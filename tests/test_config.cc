#include"base/log.h"
#include"base/config.h"
#include<vector>
static lunar::Logger::ptr g_logger = LUNAR_LOG_NAME("system");

void test_config(){
    lunar::ConfigVar<int>::ptr _int = lunar::ConfigVarMgr::GetInstance()->lookUp("a", 17, "int test");
    lunar::ConfigVar<std::vector<int>>::ptr _vec = lunar::ConfigVarMgr::GetInstance()->lookUp("test", std::vector<int>(), "int vec");
    static lunar::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
    lunar::ConfigVarMgr::GetInstance()->lookUp("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2),
            "tcp server read timeout");
    _vec->addListener([](const std::vector<int>& o, const std::vector<int>& n){
        LUNAR_LOG_DEBUG(LUNAR_LOG_ROOT()) << "hello";
    });
    LUNAR_LOG_DEBUG(LUNAR_LOG_ROOT()) << _vec->getValue().size()
    <<_vec->getDescription();
    lunar::ConfigVarMgr::GetInstance()->loadFromFile("test.yml");
    LUNAR_LOG_DEBUG(LUNAR_LOG_ROOT()) << _int->getValue()
        << _int->getDescription();

    LUNAR_LOG_DEBUG(LUNAR_LOG_ROOT()) << "--------------";

    for(size_t i = 0; i < _vec->getValue().size(); i++){
        LUNAR_LOG_DEBUG(LUNAR_LOG_ROOT())<< _vec->getValue()[i];
    }

    LUNAR_LOG_DEBUG(g_logger) << "-------------------------------------";

    LUNAR_LOG_DEBUG(g_logger) << g_tcp_server_read_timeout->getValue();
}

void test_yaml(){
    YAML::Node node(YAML::NodeType::Map);

    LUNAR_LOG_DEBUG(g_logger) << node["test"].IsDefined();

    node["name"] = YAML::Load("123");
    LUNAR_LOG_DEBUG(g_logger) << node["name"].IsScalar();
}


    // LUNAR_LOG_DEBUG(g_logger) << "hello1";
    // lunar::ConfigVarMgr::GetInstance()->loadFromFile("log.yml");
    
    // LUNAR_LOG_INFO(g_logger) << "hello2";


void test_logConfig(){
    lunar::ConfigVar<std::vector<int>>::ptr vec = lunar::ConfigVarMgr::GetInstance()->lookUp("space.vec", std::vector<int>(), "vec test");
    lunar::ConfigVar<int>::ptr num = lunar::ConfigVarMgr::GetInstance()->lookUp("space.num", int(), "num test");
    lunar::ConfigVarMgr::GetInstance()->loadFromFile("test_config.yml");

    LUNAR_LOG_INFO(g_logger) << vec->toString();
    LUNAR_LOG_INFO(g_logger) << num->toString();
}
int main(){
    
    test_logConfig();
    return 0;
}