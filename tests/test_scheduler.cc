#include"base/scheduler.h"
#include"base/hook.h"
#include <memory>
lunar::Logger::ptr g_logger = LUNAR_LOG_NAME("system");
void fun(){

    lunar::set_hook_enable(false);
    static int count = 0;
    LUNAR_LOG_DEBUG(g_logger) << "fun:"<<count;
    if(count < 5){
        lunar::Scheduler::GetThis()->schedule(fun);
        count++;
    }
    sleep(1);
}

void test1(){
    lunar::Scheduler::ptr sch(new lunar::Scheduler(2, "test_sche"));
    sch->schedule(fun);
    sch->start();
    sch->stop();
    LUNAR_LOG_DEBUG(g_logger) << "end";
}

void print(){
    
}
void test2(){
    std::unique_ptr<lunar::Scheduler> sch(new lunar::Scheduler(3, "schedule", false));
    sch->schedule([](){
        LUNAR_LOG_DEBUG(g_logger) << "fun begin";


        while(true){
            static int count = 0;
            if(count < 100){
                lunar::Fiber::YieldToReady();
                LUNAR_LOG_DEBUG(g_logger) << "fun ready back :" << count++;
            }else{
                break;
            }
        }
        LUNAR_LOG_DEBUG(g_logger) << "fun end";
    });

    sch->start();

    sch->stop();
    LUNAR_LOG_DEBUG(g_logger) << "test end";
}
int main(){
    test1();
    // test2();
}