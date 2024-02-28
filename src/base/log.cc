#include "base/log.h"
#include "base/config.h"
#include <assert.h>
namespace lunar{
    static Logger::ptr g_logger = LUNAR_LOG_ROOT();

    class FileFormatItem : public LogFormatter::FormatItem{
    public:
        FileFormatItem(const std::string& str) {  };
        virtual void format(LogEvent::ptr event, std::ostream& os) override{
            os << event->getFile();
        }
    };

    class LineFormatItem : public LogFormatter::FormatItem{
    public:
        LineFormatItem(const std::string& str) {  };
        virtual void format(LogEvent::ptr event, std::ostream& os) override{
            os << event->getLine();
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatItem{
    public:
        ElapseFormatItem(const std::string& str) {  };
        virtual void format(LogEvent::ptr event, std::ostream& os) override{
            os << event->getElapse();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem{
    public:
        ThreadIdFormatItem(const std::string& str) {  };
        virtual void format(LogEvent::ptr event, std::ostream& os) override{
            os << event->getThreadId();
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem{
    public:
        FiberIdFormatItem(const std::string& str) {  };
        virtual void format(LogEvent::ptr event, std::ostream& os) override{
            os << event->getFiberId();
        }
    };

    class TimeFormatItem : public LogFormatter::FormatItem{
    public:
        TimeFormatItem(const std::string& str = "%Y-%m-%d %H:%M:%S"): m_fmt(str){
            if(str.empty()){
                m_fmt.assign("%Y-%m-%d %H:%M:%S");
            }
        };
        virtual void format(LogEvent::ptr event, std::ostream& os) override{
            struct tm tm;
            time_t time = event->getTimeStamp();
            localtime_r(&time, &tm);
            char buf[64];
            strftime(buf, sizeof(buf), m_fmt.c_str(), &tm);
            os << buf;
        }
    private:
        std::string m_fmt;
    };

    class ThreadNameFormatItem : public LogFormatter::FormatItem{
    public:
        ThreadNameFormatItem(const std::string& str) {  };
        virtual void format(LogEvent::ptr event, std::ostream& os) override{
            os << event->getThreadName();
        }
    };

    class MessageFormatItem : public LogFormatter::FormatItem{
    public:
        MessageFormatItem(const std::string& str) {  };
        virtual void format(LogEvent::ptr event, std::ostream& os) override{
            os << event->getMsg().str();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem{
    public:
        LevelFormatItem(const std::string& str) {  };
        virtual void format(LogEvent::ptr event, std::ostream& os) override{
            os << LogLevel::ToString(event->getLevel());
        }
    };

    class TabFormatItem : public LogFormatter::FormatItem{
    public:
        TabFormatItem(const std::string& str) {  };
        virtual void format(LogEvent::ptr event, std::ostream& os) override{
            os << "\t";
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatItem{
    public:
        NewLineFormatItem(const std::string& str) {  };
        virtual void format(LogEvent::ptr event, std::ostream& os) override{
            os << std::endl;
        }
    };

    class StringFormatItem : public LogFormatter::FormatItem{
    public:
        StringFormatItem(const std::string& str): m_string(str) {  };
        virtual void format(LogEvent::ptr event, std::ostream& os) override{
            os << m_string;
        }
    private:
        std::string m_string;
    };

    class LoggerNameFormatItem : public LogFormatter::FormatItem{
    public:
        LoggerNameFormatItem(const std::string& str) {  };
        virtual void format(LogEvent::ptr event, std::ostream& os) override{
            os << event->getLogger()->getName();
        }
    };

    const std::string LogLevel::ToString(LogLevel::Level level){
        switch(level){
            #define XX(level) \
            case Level::level: \
                return #level; \
                break;

            XX(DEBUG)
            XX(INFO)
            XX(WRONG)
            XX(ERROR)
            XX(FATAL)
            #undef XX

            default: 
                return "UNKNOW";
        }
        return "UNKNOW";
    }

    const LogLevel::Level LogLevel::FromString(const std::string& level){
        #define XX(str1, str2, _level) \
        if(level == #str1 || level == #str2){ \
            return LogLevel::_level; \
        }
        XX(DEBUG, debug, DEBUG)
        XX(INFO, info, INFO)
        XX(WRONG, wrong, WRONG)
        XX(ERROR, error, ERROR)
        XX(FATAL, fatal, FATAL)
        #undef XX

        return LogLevel::UNKNOW;
    }

    LogEvent::LogEvent(
        const char* file,
        uint32_t line,
        uint32_t elapse,
        uint32_t threadId,
        uint32_t fiberId,
        uint64_t timeStamp,
        const std::string& threadName,
        LogLevel::Level level,
        Logger::ptr logger
    ):m_file(file),
    m_line(line),
    m_elapse(elapse),
    m_threadId(threadId),
    m_fiberId(fiberId),
    m_timeStamp(timeStamp),
    m_threadName(threadName),
    m_level(level),
    m_logger(logger){
    
    }

    LogEventWrap::LogEventWrap(LogEvent::ptr event):
        m_event(event){
    }

    LogEventWrap::~LogEventWrap(){
        m_event->getLogger()->log(m_event);
    }

    std::stringstream& LogEventWrap::getMsg(){
        return m_event->getMsg();
    }

    LogFormatter::LogFormatter(const std::string &pattern):
        m_pattern(pattern){
        init();
    }
    //%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n
    void LogFormatter::init(){
        std::vector<std::tuple<std::string, std::string>> res;
        int len = m_pattern.length();

        //state -- 0 普通字符部分/日志修饰字符
        //state -- 1 格式化字符部分
        //state -- 2 格式化字符参数部分
        int pLt = 0, pRt = 0, state = 0 ;

        //'\0'看成万能字符
        while(pRt <= len){
            if(state == 0){
                //状态升级
                if(pRt == len || m_pattern[pRt] == '%'){
                    if(pLt < pRt){
                        res.push_back(std::make_tuple("s", m_pattern.substr(pLt, pRt - pLt)));
                    }
                    state = 1;  //升级
                    pLt = pRt + 1;
                }
            }else if(state == 1){
                //状态还原 或 状态升级 
                //或 此时遇到非{，非%，非字母的字符，则隐式代表格式化字符部分结束
                if(pRt < len && m_pattern[pRt] == '{'){
                    if(pLt < pRt){
                        res.push_back(std::make_tuple(m_pattern.substr(pLt, pRt - pLt), ""));
                    }else{
                        //错误：没有模式字符只有选项参数
                        res.push_back(std::make_tuple("s", "<parse error> empty format character : "));
                    }
                    state = 2;
                    pLt = pRt + 1;
                }
                else if(pRt < len && m_pattern[pRt] == '%'){
                    if(pLt < pRt){
                        res.push_back(std::make_tuple(m_pattern.substr(pLt, pRt - pLt), ""));
                    }
                    state = 0;
                    pLt = pRt;
                    continue;
                }else if(pRt == len || !isalpha(m_pattern[pRt])){
                    if(pLt < pRt){
                        res.push_back(std::make_tuple(m_pattern.substr(pLt, pRt - pLt), ""));
                    }
                    state = 0;
                    pLt = pRt;
                }
            }else{  //state == 2
                //状态还原
                //缺少}，结尾（"\0"）默认为'}'
                if(pRt == len || m_pattern[pRt] == '}'){
                    std::get<1>(res.back()) = std::get<1>(res.back()) + m_pattern.substr(pLt, pRt - pLt);
                    state = 0;
                    pLt = pRt + 1;
                }
            }
            pRt++;
        }

        // for(int i = 0; i < res.size(); i++){
        //     std::cout << "(" << std::get<0>(res[i]) << ")" 
        //         << "(" << std::get<1>(res[i]) << ")" 
        //         << std::endl;
        // }

        static std::unordered_map<std::string, std::function<FormatItem::ptr(const std::string&)>> fmt_fun = {
            #define XX(fmt, item) \
                {fmt, [](const std::string& str)->FormatItem::ptr { return FormatItem::ptr(new item(str)); }},
            XX("s", StringFormatItem)
            XX("d", TimeFormatItem)
            XX("t", ThreadIdFormatItem)
            XX("N", ThreadNameFormatItem)
            XX("f", FiberIdFormatItem)
            XX("p", LevelFormatItem)
            XX("c", LoggerNameFormatItem)
            XX("F", FileFormatItem)
            XX("l", LineFormatItem)
            XX("m", MessageFormatItem)
            XX("T", TabFormatItem)
            XX("n", NewLineFormatItem)

            #undef XX
        };

        for(auto it : res){
            std::string fmt = std::get<0>(it);
            std::string str = std::get<1>(it);
            auto pos = fmt_fun.find(fmt);
            if(pos == fmt_fun.end()){
                //错误的fmt
                m_items.push_back(fmt_fun["s"]("<parse error> Unrecognized format"));
            }else{
                m_items.push_back(pos->second(str));
            }
        }
    }

    int LogFormatter::format(LogEvent::ptr event, std::ostream& os){
        std::stringstream ss;
        for(auto it : m_items){
            it->format(event, ss);
        }
        std::string msg(ss.str());
        os << msg;

        return (int)msg.length();
    }

    LogFormatter::ptr LogAppender::getFormatter() {
        MutexType::Lock lock(m_mutex);
        return m_formatter;
    }
    void LogAppender::setFormatter(LogFormatter::ptr fmt) {
        MutexType::Lock lock(m_mutex);
        m_formatter = fmt;
    }
    void LogAppender::setFormatter(const std::string& fmt){
        MutexType::Lock lock(m_mutex);
        m_formatter.reset(new LogFormatter(fmt));
    }

    size_t LogAppender::getSize(){
        MutexType::Lock lock(m_mutex);
        return  m_size;
    };
    FileLogAppender::FileLogAppender(const char* prefix):
        m_prefixName(prefix),
        m_logCount(0),
        m_lastFlush(0){
        m_fileName = m_prefixName + lunar::Time2Str() + ".txt";
        reopen();
    }
    void FileLogAppender::reopen(){
        if(m_of){
            m_of.close();
        }
        m_of.open(m_fileName, std::ios::out| std::ios::app);
    }

    void FileLogAppender::log(LogEvent::ptr event){
        MutexType::Lock lock(m_mutex);
        time_t now = event->getTimeStamp();

        if(now >= m_lastFlush + 3){
            m_lastFlush = now;
            reopen();   // 确保文件存在
        }
        if(m_logCount == LUNAR_ROLL_MAX_COUNT){    //日志回滚
            // to do
            // 是否需要考虑开一个变量记录回滚次数，防止同一时间日志洪流，
            // 导致now相同，拼接出同一文件名。
            m_fileName = m_prefixName + lunar::Time2Str(now) + ".txt";
            m_logCount = 0;
            m_lastFlush = now;
            reopen();
        }
        if(m_level <= event->getLevel()){
            LogAppender::m_size += (size_t)m_formatter->format(event, m_of);
            m_logCount++;

            //debug
            // 注释的话，日志无法即时输出到文件。
            m_of.flush();
        }
    }

    FileLogAppender::~FileLogAppender(){
        if(m_of){
            m_of.close();
        }
    }


    void StdoutLogAppender::log(LogEvent::ptr event){
        MutexType::Lock lock(m_mutex);
        if(m_level <= event->getLevel()){
            LogAppender::m_size += m_formatter->format(event, std::cout);
        }
    }

    Logger::Logger(const std::string& name):
        m_name(name){
        m_formatter.reset(new LogFormatter());
    }
    void Logger::log(LogEvent::ptr event){
        MutexType::ReadLock lock(m_mutex);
        if(m_level <= event->getLevel()){
            for(auto it : m_appenders){
                it->log(event);
            }
        }
    }
    void Logger::addAppender(LogAppender::ptr apd){
            MutexType::WriteLock lock(m_mutex);
            auto itr = std::find(m_appenders.begin(), m_appenders.end(), apd);
            if(itr == m_appenders.end()){
                if(apd->getFormatter() == nullptr){
                    apd->setFormatter(m_formatter);
                }
                m_appenders.push_back(apd);
            }
    }
    void Logger::delAppender(LogAppender::ptr apd){
        MutexType::WriteLock lock(m_mutex);
        auto itr = std::find(m_appenders.begin(), m_appenders.end(), apd);
        if(itr != m_appenders.end()){
            m_appenders.erase(itr);
        }
    }
    void Logger::clearAppender(){
        MutexType::WriteLock lock(m_mutex);
        m_appenders.clear();
    }
    LogFormatter::ptr Logger::getFormatter() {
        MutexType::ReadLock lock(m_mutex);
        return m_formatter; 
    }

    void Logger::setFormatter(LogFormatter::ptr fmt) {
        MutexType::WriteLock lock(m_mutex);
        m_formatter = fmt;
    }
    void Logger::setFormatter(const std::string& fmt){
        MutexType::WriteLock lock(m_mutex);
        m_formatter.reset(new LogFormatter(fmt));
    }
    std::string Logger::getName() {
        MutexType::ReadLock lock(m_mutex);
        return m_name;
    }

    void Logger::setName(const std::string& name) {
        MutexType::WriteLock lock(m_mutex);
        m_name = name;
    }

    size_t Logger::getTotalSize(size_t& out){
        out = 0;
        MutexType::ReadLock lock(m_mutex);
        for(auto &it : m_appenders){
            out += it->getSize();
        }
        return m_appenders.size();
    }

    LoggerManager::LoggerManager(){

        m_root.reset(new Logger("root"));
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender()));

        addLogger("root", m_root);
    }

    Logger::ptr LoggerManager::getRoot(){
        return m_root;
    }

    Logger::ptr LoggerManager::getLoggerByName(const std::string& name){
        static struct LogIniter __log_init;
        MutexType::ReadLock lock(m_mutex);
        auto pos = m_loggers.find(name);
        if(pos == m_loggers.end()){
            // 没有就返回nullptr
            return nullptr;
        }
        return pos->second;
    }

    void LoggerManager::addLogger(const std::string& name, Logger::ptr logger){
        MutexType::WriteLock lock(m_mutex);
        auto pos = m_loggers.find(name);
        if(pos != m_loggers.end()){
            return;
        }
        m_loggers.insert({name, logger});
    }
    void LoggerManager::delLogger(const std::string& name){
        MutexType::WriteLock lock(m_mutex);
        m_loggers.erase(name);
    }
    //日志配置Logger

    struct LogAppenderDefine{
        LogLevel::Level m_level;
        std::string m_formatter;
        std::string m_prefixName;
        std::string m_type;

        LogAppenderDefine():
            m_level(LogLevel::Level::DEBUG),
            m_formatter(""),
            m_prefixName(""),
            m_type(""){
        }
        bool operator==(const LogAppenderDefine& rhs)const {
            return m_type == rhs.m_type &&
                m_level == rhs.m_level &&
                m_formatter == rhs.m_formatter &&
                m_prefixName == rhs.m_prefixName;
        }
    };

    struct LoggerDefine{
        std::list<LogAppenderDefine> m_appenders;
        LogLevel::Level m_level;
        std::string m_formatter;
        std::string name;
        LoggerDefine():m_appenders(),
            m_level(LogLevel::Level::DEBUG),
            m_formatter(""),
            name(""){
        }
        bool operator<(const LoggerDefine& rhs)const {
            return name < rhs.name;
        }

        bool operator==(const LoggerDefine& rhs)const {
            // todo ....
            return name == rhs.name &&
                m_appenders == rhs.m_appenders &&
                m_level == rhs.m_level &&
                m_formatter == rhs.m_formatter;
        }

    };

    /**
     * @brief 偏特化类型转换模板类YAMLString类型转化成LoggerDefine类型
     */
    template<>
    class LexicalCast<std::string, LoggerDefine >{
    public:
        LoggerDefine operator()(const std::string& val){
            YAML::Node node = YAML::Load(val);
            LoggerDefine res;
            if(!node["name"].IsDefined()){
                //name未定义
                // 配置没解析完，此时使用日志可能太危险，是否需要等配置解析完再使用日志？
                LUNAR_LOG_ERROR(g_logger) << "log config error: name is null, " 
                    << node;
                throw std::logic_error("log config name is null");
            }else{
                res.name = node["name"].as<std::string>();

                if(node["level"].IsDefined()){
                    res.m_level = LogLevel::FromString(node["level"].as<std::string>());
                }//else     //默认m_level = DEBUG

                if(node["formatter"].IsDefined()){
                    res.m_formatter = node["formatter"].as<std::string>();
                }//else     //默认m_formatter = ""

                if(node["appenders"].IsDefined()){
                    for(int i = 0; i < (int)node["appenders"].size(); i++){
                        YAML::Node it = node["appenders"][i];
                        LogAppenderDefine lad;
                        std::string type = it["type"].IsDefined() ? it["type"].as<std::string>() : "";

                        lad.m_type = type;
                        if(type == "StdoutLogAppender"){
                            //同步输出到控制台
                        }else if(type == "FileLogAppender"){
                            //同步输出到文件

                            if(it["prefixName"].IsDefined() && it["prefixName"].as<std::string>() != ""){
                                lad.m_prefixName = it["prefixName"].as<std::string>();
                            }else{
                                //摒弃该appender
                                LUNAR_LOG_ERROR(g_logger) << "log config error: appender type is FileLogAppender, but prefixName is undefine, " 
                                    << it;

                                continue;
                            }
                        }else if(type == "AsyncAppender"){
                            //异步输出到文件

                            //to do
                            continue;
                        }else{
                            //未知类型，摒弃该appender
                            LUNAR_LOG_ERROR(g_logger) << "log config error: appender type is unknow, " 
                                << it;
                            
                            continue;
                        }
                        if(it["level"].IsDefined()){
                            lad.m_level = LogLevel::FromString(it["level"].as<std::string>());
                        }//else     //默认m_level = DEBUG
                        if(it["formatter"].IsDefined()){
                            lad.m_formatter = it["formatter"].as<std::string>();
                        }//else     //默认m_formatter = ""

                        res.m_appenders.push_back(lad);
                    }
                }

            }

            return res;
        }
    };

    /**
     * @brief 偏特化类型转换，LoggerDefine类型转化成YAMLString类型
     */
    template<>
    class LexicalCast<LoggerDefine, std::string>{
    public:
        std::string operator()(const LoggerDefine& val){
            YAML::Node node(YAML::NodeType::Map);

            node["name"] = val.name;
            node["formatter"] = val.m_formatter;
            node["level"] = LogLevel::ToString(val.m_level);

            for(auto &it : val.m_appenders){
                YAML::Node apd(YAML::NodeType::Map);
                apd["level"] = LogLevel::ToString(it.m_level);
                apd["formatter"] = it.m_formatter;

                apd["type"] = it.m_type;
                if(it.m_type == "FileLogAppender"){
                    apd["prefixName"] = it.m_prefixName;
                }

                node["appenders"].push_back(apd);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };



    void LogIniter::init(){
        ConfigVar<std::set<LoggerDefine>>::ptr loggerDefines = 
            ConfigVarMgr::GetInstance()->lookUp("log", std::set<LoggerDefine>(), "log config");
        
        // 配置一下系统默认的system logger
        #define XX(name) \
        { \
            Logger::ptr logger; \
            logger.reset(new Logger(name)); \
            logger->addAppender(LogAppender::ptr(new StdoutLogAppender())); \
            LoggerMgr::GetInstance()->addLogger(name, logger); \
        }while(0)

        XX("system");
        #undef XX

        // 安装日志配置改变的回调。
        loggerDefines->addListener([](const std::set<LoggerDefine>& oldVal, 
            const std::set<LoggerDefine>& newVal){

            LUNAR_LOG_INFO(g_logger)<< "on_logger_conf_changed";
            for(const auto &it : newVal){
                auto old = oldVal.find(it); // 通过logger.name查找
                Logger::ptr logger;
                if(old != oldVal.end()){
                    //存在
                    logger = LUNAR_LOG_NAME(old->name);

                    // oldVal所定义的logger一定是LoggerMgr的子集，不会存在LoggerMgr没有的logger。
                    LUNAR_ASSERT(logger != nullptr);

                    if((*old) == it){   // 该判断在后面的application模块是非常必要，可以确保不重复创建日志文件。
                        continue;   // 配置无变化。
                    }
                }else{
                    //oldVal不存在但LoggerMgr可能存在比如root
                    logger = LUNAR_LOG_NAME(it.name);

                    if(logger == nullptr){  // 新增
                        logger.reset(new Logger(it.name));
                        LoggerMgr::GetInstance()->addLogger(it.name, logger);
                    }
                }

                //根据newVal -- it 修改logger
                logger->clearAppender();

                if(it.m_formatter != ""){
                    logger->setFormatter(it.m_formatter);
                }//else     //默认的formatter

                logger->setLevel(it.m_level);

                //appenders
                for(const auto &apd : it.m_appenders){
                    LogAppender::ptr appender;
                    if(apd.m_type == "StdoutLogAppender"){
                        appender.reset(new StdoutLogAppender());
                        appender->setLevel(apd.m_level);
                        if(apd.m_formatter != ""){
                            appender->setFormatter(apd.m_formatter);
                        }
                    }else if(apd.m_type == "FileLogAppender"){
                        if(apd.m_prefixName == ""){
                            //never reach
                            continue;
                        }
                        appender.reset(new FileLogAppender(apd.m_prefixName.c_str()));
                        appender->setLevel(apd.m_level);
                        if(apd.m_formatter != ""){
                            appender->setFormatter(apd.m_formatter);
                        }
                    }else if(apd.m_type == "AsyncAppender"){

                    }else{
                        // never reach
                    }
                    logger->addAppender(appender);
                }
            }

            //删除LoggerMgr中newVal不存在的logger
            for(const auto &it : oldVal){
                auto rt = newVal.find(it);
                if(rt == newVal.end()){
                    LoggerMgr::GetInstance()->delLogger(it.name);
                }
            }

        });
    }
    ConfigVar<std::set<LoggerDefine>>::ptr g_logger_defines = ConfigVarMgr::GetInstance()->lookUp("log", std::set<LoggerDefine>(), "log config");

}


