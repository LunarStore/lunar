#include"base/log.h"
#include"base/thread.h"
#include"base/config.h"
static lunar::Logger::ptr g_logger = LUNAR_LOG_NAME("system");
void test_lock(){
    lunar::ConfigVarMgr::GetInstance()->loadFromFile("log.yml");
    lunar::Thread::ptr t1(new lunar::Thread([](){
        while(true){
            LUNAR_LOG_DEBUG(g_logger) << "---------------------------------------------------------";
        }
    }, "t1"));

    lunar::Thread::ptr t2(new lunar::Thread([](){
        while(true){
            LUNAR_LOG_DEBUG(g_logger) << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++";
        }
    }, "t2"));

    t1->join();
    t2->join();
}
void test_log(){
    // LUNAR_LOG_DEBUG(LUNAR_LOG_ROOT()) << "hello";
    LUNAR_LOG_DEBUG(LUNAR_LOG_NAME("system")) << "system hello";
}


void debug_log(){


}
int main(){
    // lunar::Logger::ptr logger(new lunar::Logger());
    // lunar::LogFormatter::ptr formatter(new lunar::LogFormatter("[%p]%T[%N:%T%t]%T[Fiber ID :%T%f]%T%F:%l%n"));
    // logger->setFormatter(formatter);
    // logger->addAppender(lunar::LogAppender::ptr(new lunar::StdoutLogAppender()));
    // LUNAR_LOG_DEBUG(logger) << "hello logger";
    test_log();

    // test_lock();
    return 0;
}