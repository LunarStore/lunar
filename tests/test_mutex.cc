#include "base/mutex.h"
#include "base/iomanager.h"
#include "base/log.h"
#include <functional>
#include <unistd.h>
lunar::Logger::ptr g_logger = LUNAR_LOG_NAME("system");
class FiberSemCallBack{
public:
    FiberSemCallBack():m_fs(0){
        //LUNAR_LOG_INFO(g_logger) << "FiberSemCallBack()";
    }
    ~FiberSemCallBack(){
        //LUNAR_LOG_INFO(g_logger) << "~FiberSemCallBack()";
    }

    void waitCB(){
        LUNAR_LOG_INFO(g_logger) << "entry wait";
        if(m_fs.waitForSeconds(5) == true){
            LUNAR_LOG_INFO(g_logger) << "time out, " 
                << "sem = "
                << (int)m_fs.getSem();
        }else{
            LUNAR_LOG_INFO(g_logger) << "get sem,"
                << "sem = "
                << (int)m_fs.getSem();
        }
        LUNAR_LOG_INFO(g_logger) << "exit wait";

        sleep(3); 
        LUNAR_LOG_INFO(g_logger) << "waitCB return";
    }

    void postCB(){
        LUNAR_LOG_INFO(g_logger) << "entry sleep";
        sleep(3);
        LUNAR_LOG_INFO(g_logger) << "exit sleep";

        LUNAR_LOG_INFO(g_logger) << "entry post";
        m_fs.post();
        LUNAR_LOG_INFO(g_logger) << "exit post";

        sleep(5);

        LUNAR_LOG_INFO(g_logger) << "postCB return";

    }
private:
    lunar::FiberSemaphore m_fs;
};

static FiberSemCallBack fsc;
int main(){
    //g_logger->setLevel(lunar::LogLevel::INFO);
    lunar::IOManager iom(1, "test_mutex", true);

    iom.schedule(std::bind((void (FiberSemCallBack::*)(void))&FiberSemCallBack::waitCB, &fsc));

    iom.schedule(std::bind((void (FiberSemCallBack::*)(void))&FiberSemCallBack::postCB, &fsc));
    return 0;
}