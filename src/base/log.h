#ifndef __LUNAR_LOG_H__

#define __LUNAR_LOG_H__

#include<memory>
#include<string>
#include<sstream>
#include<list>
#include<vector>
#include<stdint.h>
#include<iostream>
#include<fstream>
#include<ostream>
#include<functional>
#include<unordered_map>
#include<algorithm>
#include<time.h>
#include<map>


#include"base/util.h"
#include"base/singleton.h"
#include"base/mutex.h"
#include"base/thread.h"

#define LUNAR_LOG_LEVEL(logger, level) \
    if((logger)->getLevel() <= level) \
        (lunar::LogEventWrap(lunar::LogEvent::ptr(new lunar::LogEvent(__FILE__, __LINE__,\
        lunar::GetElapse(), lunar::Thread::GetTid(),\
        lunar::GetFiberId(), ::time(nullptr),\
        lunar::Thread::GetName(), level, (logger))))).getMsg()

#define LUNAR_LOG_DEBUG(logger) LUNAR_LOG_LEVEL(logger, lunar::LogLevel::Level::DEBUG)

#define LUNAR_LOG_INFO(logger) LUNAR_LOG_LEVEL(logger, lunar::LogLevel::Level::INFO)

#define LUNAR_LOG_WRONG(logger) LUNAR_LOG_LEVEL(logger, lunar::LogLevel::Level::WRONG)

#define LUNAR_LOG_ERROR(logger) LUNAR_LOG_LEVEL(logger, lunar::LogLevel::Level::ERROR)

#define LUNAR_LOG_FATAL(logger) LUNAR_LOG_LEVEL(logger, lunar::LogLevel::Level::FATAL)

#define LUNAR_ROLL_MAX_COUNT (1024 * 1024)

#define LUNAR_LOG_ROOT() lunar::LoggerMgr::GetInstance()->getRoot()

#define LUNAR_LOG_NAME(name) lunar::LoggerMgr::GetInstance()->getLoggerByName(name)
namespace lunar{

    class Logger;

    class LogLevel{
    public:
      enum Level{
          UNKNOW  = 0,
          DEBUG   = 1,
          INFO    = 2,
          WRONG  = 3,
          ERROR   = 4,
          FATAL   = 5
      };
    public:
      static const std::string ToString(LogLevel::Level level);
      static const LogLevel::Level FromString(const std::string& level);
    };
    
    class LogEvent{
    public:
      typedef std::shared_ptr<LogEvent> ptr;
    public:
      LogEvent(
          const char* file,
          uint32_t line,
          uint32_t elapse,
          uint32_t threadId,
          uint32_t fiberId,
          uint64_t timeStamp,
          const std::string& threadName,
          LogLevel::Level level,
          std::shared_ptr<Logger> logger
          );
      const char* getFile()const { return m_file; }
      uint32_t getLine()const { return m_line; }
      uint32_t getElapse()const { return m_elapse; }
      uint32_t getThreadId()const { return m_threadId; }
      uint32_t getFiberId()const { return m_fiberId; }
      uint64_t getTimeStamp()const { return m_timeStamp; }
      const std::string& getThreadName()const { return m_threadName; }
      std::stringstream& getMsg() { return m_msg; }
      LogLevel::Level getLevel()const { return m_level; }
      std::shared_ptr<Logger> getLogger() { return m_logger; }
    private:
      //文件名
      const char* m_file;
      //行号
      uint32_t m_line;
      //程序启动到现在的秒数
      uint32_t m_elapse;
      //线程号
      uint32_t m_threadId;
      //协程号
      uint32_t m_fiberId;
      //时间戳
      uint64_t m_timeStamp;
      //线程名
      std::string m_threadName;
      //日志消息
      std::stringstream m_msg;
      //日志级别
      LogLevel::Level m_level;
    
      std::shared_ptr<Logger> m_logger;
    };
    

    class LogEventWrap{
    public:
        typedef std::shared_ptr<LogEventWrap> ptr;
    public:
        LogEventWrap(LogEvent::ptr event);
        virtual ~LogEventWrap();
        std::stringstream& getMsg();
    private:
        LogEvent::ptr m_event;
    };

    class LogFormatter{
    public:
        typedef std::shared_ptr<LogFormatter>ptr;
    public:
        class FormatItem{
        public:
            typedef std::shared_ptr<FormatItem> ptr;
        public:
            virtual void format(LogEvent::ptr event, std::ostream& os) = 0;
        };
    public:
        LogFormatter(const std::string &pattern = "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%f%T[%p]%T[%c]%T%F:%l%T%m%n");
        int format(LogEvent::ptr event, std::ostream& os);
    private:
        void init();
    private:
        std::vector<FormatItem::ptr> m_items;
        std::string m_pattern;
    };
    
    class LogAppender{
    public:
        typedef std::shared_ptr<LogAppender> ptr;
        typedef Mutex MutexType;
    public:
        virtual ~LogAppender() {  };
        virtual void log(LogEvent::ptr event) = 0;
    public:
        //返回值使用期间，m_formatter可能被更改
        LogFormatter::ptr getFormatter();
        void setFormatter(LogFormatter::ptr fmt);
        void setFormatter(const std::string& fmt);
        LogLevel::Level getLevel()const { return m_level; }
        void setLevel(LogLevel::Level level) { m_level = level; }

        // for test
        size_t getSize();
    protected:
        LogLevel::Level m_level = LogLevel::Level::DEBUG;
        LogFormatter::ptr m_formatter;
        size_t m_size = 0;  // 记录输出到该LogAppender的日志的字节数。
        MutexType m_mutex;
    };

    class FileLogAppender : public LogAppender{
    public:
        FileLogAppender(const char* prefix = "log");
        virtual ~FileLogAppender() override;
        virtual void log(LogEvent::ptr event) override;
    private:
        void reopen();
    private:
        std::ofstream m_of;
        std::string m_prefixName;
        std::string m_fileName;
        uint32_t m_logCount;
        time_t m_lastFlush;
    };

    class StdoutLogAppender : public LogAppender{
    public:
        virtual void log(LogEvent::ptr event) override;
        virtual ~StdoutLogAppender() override{  };
    };

    class Logger{
    public:
        typedef std::shared_ptr<Logger> ptr;
        typedef RWMutex MutexType;
    public:
        void log(LogEvent::ptr event);
    public:
        Logger(const std::string& name = "system");
        LogLevel::Level getLevel()const { return m_level; }
        void setLevel(LogLevel::Level level) { m_level = level; }
        void addAppender(LogAppender::ptr apd);
        void clearAppender();
        void delAppender(LogAppender::ptr apd);

        LogFormatter::ptr getFormatter();
        void setFormatter(LogFormatter::ptr fmt);
        void setFormatter(const std::string& fmt);
        std::string getName();
        void setName(const std::string& name);

        size_t getTotalSize(size_t& out);
    private:
        std::list<LogAppender::ptr> m_appenders;
        LogLevel::Level m_level = LogLevel::Level::DEBUG;
        LogFormatter::ptr m_formatter;
        std::string m_name;
        MutexType m_mutex;
    };

    class LoggerManager{
    public:
        typedef std::shared_ptr<LoggerManager> ptr;
        typedef RWMutex MutexType;
    public:
        LoggerManager();
        Logger::ptr getRoot();
        Logger::ptr getLoggerByName(const std::string& name);
        void addLogger(const std::string& name, Logger::ptr logger);
        void delLogger(const std::string& name);
    private:
        Logger::ptr m_root;
        std::map<std::string, Logger::ptr> m_loggers;
        MutexType m_mutex;
    };
    struct LogIniter{
        LogIniter(){
            init();
        }

        void init();

    };
    
    typedef Singleton<LoggerManager> LoggerMgr;

}

#endif

