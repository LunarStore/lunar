#include "base/AsyncLog.h"
#include "base/iomanager.h"
#include "base/log.h"
#include <functional>
#include <string>
#include <time.h>
static lunar::Logger::ptr g_logger = LUNAR_LOG_NAME("system");

void testOneLog(){
    lunar::IOManager iom(1, "testOneLog", true);
    lunar::AsyncLog* alog = new lunar::AsyncLog("asynlog");
    g_logger->addAppender(lunar::LogAppender::ptr(alog));
    iom.schedule(
        std::bind(
            (void (lunar::AsyncLog::*)(void))(&lunar::AsyncLog::backGroundHandler), 
            alog
        )
    );
    iom.schedule([](){
        LUNAR_LOG_INFO(g_logger) << "ok";
    });

    return;
}

static const int kMicroSecondsPerSecond = 1000 * 1000;
int64_t now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return seconds * kMicroSecondsPerSecond + tv.tv_usec;
}

/*
* 计算两个微妙差值的，秒数
*/
inline double timeDifference(int64_t high, int64_t low){
    int64_t diff = high - low;
    return static_cast<double>(diff) / kMicroSecondsPerSecond;
}

void bench(bool longLog) {
    int cnt = 0;
    const int kBatch = 1000;
    std::string empty = " ";
    std::string longStr(3000, 'X');
    longStr += " ";
    size_t msgPerS = 0;
    size_t mibPerS = 0;
    for (int t = 0; t < 30; ++t) {
        int64_t start = now();
        for (int i = 0; i < kBatch; ++i) {
            LUNAR_LOG_INFO(g_logger) << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz "
                    << (longLog ? longStr : empty)
                    << cnt;
            ++cnt;
        }
        int64_t end = now();
        double seconds = timeDifference(end, start);
        size_t total;
        LUNAR_ASSERT(g_logger->getTotalSize(total) == 1);

        printf("%12s: %f seconds, %d bytes, %10.2f msg/s, %.2f MiB/s, %f us/msg\n",
                longLog ? "long log" : "short log", seconds, total, kBatch / seconds, total / seconds / (1024 * 1024), seconds*kMicroSecondsPerSecond/kBatch);
        struct timespec ts = { 0, 500*1000*1000 };
        msgPerS += kBatch / seconds;
        mibPerS += total / seconds / (1024 * 1024);
        nanosleep(&ts, NULL);
    }

    printf("%12s [avg(30)]: %10.2f msg/s, %.2f MiB/s\n",
            longLog ? "long log" : "short log", msgPerS / 30.0, mibPerS / 30.0);
}

void testLogPerformance(int argc, char* argv[]){
    int num = atoi(argv[3]);
    if(num <= 0){
        // 线程数符合规矩
        return;
    }
    lunar::IOManager iom(num, "testLogPerformance", true);

    g_logger->clearAppender();
    if(std::string(argv[1]) == "asyn"){
        // 异步日志性能测试
        lunar::AsyncLog* alog = new lunar::AsyncLog("asynlog");

        g_logger->addAppender(lunar::LogAppender::ptr(alog));

        iom.schedule(
            std::bind(
                (void (lunar::AsyncLog::*)(void))(&lunar::AsyncLog::backGroundHandler), 
                alog
            )
        );
    }else{
        // 同步日志性能测试
        g_logger->addAppender(lunar::LogAppender::ptr(new lunar::FileLogAppender("log")));
    }
    iom.schedule(std::bind(bench, std::string(argv[2]) == std::string("llog")));

    return; 
}
/*
* arc == 4
* argv[1] ："asyn" 或者 "syn"
* argv[2] ： "llog"代表长日志。其他代表短日志。
* argv[3] ："线程数"
*/
int main(int argc, char* argv[]){
    if(argc == 4 && (argv[1] == std::string("asyn") || argv[1] == std::string("syn"))){

        testLogPerformance(argc, argv);
    }else{
        LUNAR_LOG_INFO(g_logger) << "The input parameters in the console do not meet the requirements!";
    }

    return 0;
}