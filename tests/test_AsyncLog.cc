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
int64_t now()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t seconds = tv.tv_sec;
  return seconds * kMicroSecondsPerSecond + tv.tv_usec;
}
inline double timeDifference(int64_t high, int64_t low)
{
  int64_t diff = high - low;
  return static_cast<double>(diff) / kMicroSecondsPerSecond;
}
void bench(bool longLog)
{
  int cnt = 0;
  const int kBatch = 1000;
  std::string empty = " ";
  std::string longStr(3000, 'X');
  longStr += " ";

  for (int t = 0; t < 30; ++t)
  {
    int64_t start = now();
    for (int i = 0; i < kBatch; ++i)
    {
      LUNAR_LOG_INFO(g_logger) << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz "
               << (longLog ? longStr : empty)
               << cnt;
      ++cnt;
    }
    int64_t end = now();
    printf("%f\n", timeDifference(end, start)*1000000/kBatch);
    struct timespec ts = { 0, 500*1000*1000 };
    nanosleep(&ts, NULL);
  }
}

void testLogPerformance(int argc, char* argv[]){
    lunar::IOManager iom(2, "testLogPerformance", true);
    lunar::AsyncLog* alog = new lunar::AsyncLog("asynlog");
    g_logger->clearAppender();
    if(argc == 2){
        g_logger->addAppender(lunar::LogAppender::ptr(alog));
    }else{
        g_logger->addAppender(lunar::LogAppender::ptr(new lunar::FileLogAppender("log")));
    }

    iom.schedule(
        std::bind(
            (void (lunar::AsyncLog::*)(void))(&lunar::AsyncLog::backGroundHandler), 
            alog
        )
    );
    iom.schedule(std::bind(bench, std::string(argv[argc - 1]) == "true"));

    return; 
}

int main(int argc, char* argv[]){

    testLogPerformance(argc, argv);
    return 0;
}