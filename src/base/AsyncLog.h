#ifndef __LUNAR_ASYNCLOG_H__

#define __LUNAR_ASYNCLOG_H__


#include <string>
#include <sstream>
#include <atomic>
#include <vector>
#include "base/log.h"
#include "base/mutex.h"
namespace lunar{
    class AsyncLog : public LogAppender{
    public:
        AsyncLog(std::string fPrefixName = "log");
        ~AsyncLog();
        void log(LogEvent::ptr event);

        void backGroundHandler();   //后台协程。
    private:
        void stop();
    private:
        std::string m_fPrefixName;    //文件名
        std::stringstream m_buffer; //缓存
        //std::string m_buffer;
        std::vector<std::string> m_buffers;
        uint32_t m_logCount;    //日志条数
        FiberSemaphore m_fiberSem;  //到达一定数量，需要回滚文件，重新创建一个新文件

        std::atomic<bool> m_isStoping = { false }; //是否停止
        uint32_t m_logSplite;
        size_t m_rollSize;
    };
}

#endif