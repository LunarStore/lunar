#include "base/AsyncLog.h"
#include "base/macro.h"
#include "base/fiber.h"
#include "base/util.h"
#include "base/config.h"
#include <time.h>
#include <fstream>
namespace lunar{
    //static uint32_t s_logSplite = 10000;
    //static size_t s_rollSize = 45*1000*1000;
    static ConfigVar<uint32_t>::ptr s_logSplite = ConfigVarMgr::GetInstance()->lookUp("log.splite", uint32_t(10000), "log splite size");

    static ConfigVar<size_t>::ptr s_rollSize = ConfigVarMgr::GetInstance()->lookUp("log.rollSize", size_t(45 * 1024 * 1024), "log roll size");

    static lunar::Logger::ptr g_logger = LUNAR_LOG_NAME("system");
    AsyncLog::AsyncLog(std::string fPrefixName): 
        m_fPrefixName(fPrefixName),
        m_logCount(0),
        m_fiberSem(0){
        m_buffers.reserve(20);
        m_logSplite = s_logSplite->getValue();
        m_rollSize = s_rollSize->getValue();
    }
    AsyncLog::~AsyncLog(){
        stop();
    }

    void AsyncLog::stop(){  //不可占锁
        m_isStoping.store(true);

        m_fiberSem.post();
    }
    void AsyncLog::log(LogEvent::ptr event){
        MutexType::Lock lock(m_mutex);

        if(m_level <= event->getLevel()){
            LogAppender::m_size += m_formatter->format(event, m_buffer);
            m_logCount++;   //日志切割

            if(m_logCount >= m_logSplite){ //过大，切换buff
                m_buffers.emplace_back(std::move(m_buffer.str()));
                m_buffer.str("");
                m_logCount = 0;

                m_fiberSem.post();
            }
        }
    }

    void AsyncLog::backGroundHandler(){
        LUNAR_ASSERT(Fiber::GetThis() != nullptr);    //协程必须初始化完毕。

        std::vector<std::string> bufferVector;
        std::ofstream of;   //文件流
        std::string fPrefixName = "log";
        std::string fName;
        size_t currentSize = 0; //当前文件大小
        time_t lastRoll = time(0);    //最后一次回滚时间戳
        {
            MutexType::Lock lock(m_mutex);
            if(m_fPrefixName.empty() == false){
                fPrefixName = m_fPrefixName;
            }
        }
        fName = fPrefixName + Time2Str(lastRoll) + ".txt";
        of.open(fName, std::ios::out| std::ios::app);

        if(of.is_open() == true){
            LUNAR_LOG_INFO(g_logger) << "roll a log file, the new file name is " + fName;
        }else{
            LUNAR_LOG_FATAL(g_logger) << "open a new file fail, the new file name is" + fName;
            LUNAR_ASSERT(false);
        }

        bufferVector.reserve(20);
        while(!m_isStoping.load()){
            {
                LUNAR_ASSERT(bufferVector.empty() == true);
                MutexType::Lock lock(m_mutex);
                if(m_buffers.empty()){
                    lock.unlock();
                    m_fiberSem.waitForSeconds(2);   //等2秒
                    //to do call m_fiberSem.reset()
                    lock.lock();
                    m_fiberSem.reset();
                }

                if(!m_buffer.str().empty()){
                    m_buffers.emplace_back(std::move(m_buffer.str()));
                    m_buffer.str("");
                    m_logCount = 0;
                }

                bufferVector.swap(m_buffers);
            }
            //当日志突然太多，需要丢弃一部分。to do

            for(auto &it : bufferVector){
                of << it;
                currentSize = currentSize + it.length();
                if(currentSize > m_rollSize){   //大于回滚大小
                    time_t now = time(0);
                    if(now > lastRoll){ //不是同一时刻，以免创建文件名重复
                        lastRoll = now;
                        fName = fPrefixName + Time2Str(lastRoll) + ".txt";
                        of.close();
                        of.open(fName, std::ios::out| std::ios::app);
                        currentSize = 0;    //重置大小

                        if(of.is_open() == true){
                            LUNAR_LOG_INFO(g_logger) << "roll a log file, the new file name is " + fName;
                        }else{
                            LUNAR_LOG_FATAL(g_logger) << "open a new file fail, the new file name is" + fName;
                            LUNAR_ASSERT(false);
                        }
                    }
                }
            }

            bufferVector.clear();
            of.flush();
        }
        of.flush();
    }
}