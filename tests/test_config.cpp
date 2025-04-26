#include "../server/config.h"
#include "../server/log.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

server::ConfigVar<int>::ptr g_int_value_config;
server::ConfigVar<float>::ptr g_int_valuex_config;
server::ConfigVar<float>::ptr g_float_value_config;
server::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config;
server::ConfigVar<std::list<int>>::ptr g_int_list_value_config;
server::ConfigVar<std::set<int>>::ptr g_int_set_value_config;
server::ConfigVar<std::unordered_set<int>>::ptr g_int_unordered_set_value_config;
server::ConfigVar<std::map<std::string, int>>::ptr g_str_int_map_value_config;
server::ConfigVar<std::unordered_map<std::string, int>>::ptr g_str_int_unordered_map_value_config;

void print_yaml(const YAML::Node& node, int level){
    if(node.IsScalar()) {
        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type() << " - " << level;
    }
    else if(node.IsNull()) {
        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - " << node.Type() << " - " << level;
    }
    else if(node.IsMap()) {
        for(auto it = node.begin(); it != node.end(); ++it){
            SERVER_LOG_INFO(SERVER_LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    }
    else if(node.IsSequence()) {
        for(size_t i = 0; i < node.size(); ++i){
            SERVER_LOG_INFO(SERVER_LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml() {
    YAML::Node root = YAML::LoadFile("/home/jiaozexin/workspace/c++/server/bin/conf/test.yml");
    // print_yaml(root, 0);
    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << root;
}

void test_config() {

    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "before: " << g_float_value_config->toString();

#define XX(g_var, name, prefix) { \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            SERVER_LOG_INFO(SERVER_LOG_ROOT()) << #prefix " " #name ": " << i; \
        }\
        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

#define XX_M(g_var, name, prefix) { \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            SERVER_LOG_INFO(SERVER_LOG_ROOT()) << #prefix " " #name ": {" << i.first << " - " << i.second << "}"; \
        }\
        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_unordered_set_value_config, int_set, before);
    XX_M(g_str_int_map_value_config, int_map, before);
    XX_M(g_str_int_unordered_map_value_config, int_unordered_map, before);

    YAML::Node root = YAML::LoadFile("/home/jiaozexin/workspace/c++/server/bin/conf/oldtest.yml");
    server::Config::LoadFromYaml(root);

    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "after: " << g_float_value_config->toString();

    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_unordered_set_value_config, int_set, after);
    XX_M(g_str_int_map_value_config, int_map, after);
    XX_M(g_str_int_unordered_map_value_config, int_unordered_map, after);

}

class Person {
public:
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex
           << "]";
        return ss.str();
    }

    bool operator==(const Person& oth) const {
        return m_name == oth.m_name
            && m_age == oth.m_age
            && m_sex == oth.m_sex;
    }
};

namespace server {
template<>
class LexicalCast<std::string, Person> {
public:
    Person operator() (const std::string& v) {
        Person p;
        YAML::Node node = YAML::Load(v);
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};

template<>
class LexicalCast<Person, std::string> {
public:
    std::string operator() (const Person& p) {
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};
}

server::ConfigVar<Person>::ptr g_person;
server::ConfigVar<std::map<std::string, Person>>::ptr g_person_map;
server::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr g_person_vec_map;

void test_class() {
    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "before: " << g_person->getValue().toString() << " - " << g_person->toString();

#define XX_PM(g_var, prefix) \
    { \
        auto m = g_var->getValue(); \
        for(auto& i : m) { \
            SERVER_LOG_INFO(SERVER_LOG_ROOT()) << #prefix ": " << i.first << " - " << i.second.toString(); \
        } \
        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << #prefix ": size=" << m.size(); \
    }

    g_person->addListener([](const Person& old_val, const Person& new_val){
        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "old value=" << old_val.toString()
            << " new value=" << new_val.toString();
    });

    g_person_map->addListener([](const std::map<std::string, Person>& old_val, const std::map<std::string, Person>& new_val){
        for(auto& i : new_val) {
            SERVER_LOG_INFO(SERVER_LOG_ROOT()) << " new value key=" << i.first << " new value val=" << i.second.toString();
        }
    });

    XX_PM(g_person_map, class.map before);
    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "before: " << g_person_vec_map->toString();

    YAML::Node root = YAML::LoadFile("/home/jiaozexin/workspace/c++/server/bin/conf/oldtest.yml");
    server::Config::LoadFromYaml(root);

    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "after: " << g_person->getValue().toString() << " - " << g_person->toString();
    XX_PM(g_person_map, class.map after);
    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "after: " << g_person_vec_map->toString();
}

void test_log() {
    static server::Logger::ptr system_log = SERVER_LOG_NAME("system");
    SERVER_LOG_INFO(system_log) << "hello system" << std::endl;
    std::cout << server::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/jiaozexin/workspace/c++/server/bin/conf/test.yml");
    server::Config::LoadFromYaml(root);
    std::cout << "=========" << std::endl;
    std::cout << server::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout << "=========" << std::endl;
    std::cout << root << std::endl;
    
    SERVER_LOG_INFO(system_log) << "hello system after" << std::endl;

    system_log->setFormatter("%d - %m%n");
    SERVER_LOG_INFO(system_log) << "hello system new" << std::endl;
}

int main(int argc, char** argv){
    
    g_int_value_config = server::Config::Lookup("system.port", (int)8080, "system port");
    g_int_valuex_config = server::Config::Lookup("system.port", (float)8080, "system port");
    g_float_value_config = server::Config::Lookup("system.value", (float)10.2, "system value");
    g_int_vec_value_config = server::Config::Lookup("system.int_vec", std::vector<int>{1, 2}, "system int vec");
    g_int_list_value_config = server::Config::Lookup("system.int_list", std::list<int>{1, 2}, "system int list");
    g_int_set_value_config = server::Config::Lookup("system.int_set", std::set<int>{1, 2}, "system int set");
    g_int_unordered_set_value_config = server::Config::Lookup("system.int_unordered_set", std::unordered_set<int>{1, 2}, "system int unordered_set");
    g_str_int_map_value_config = server::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k", 2}}, "system str int map");
    g_str_int_unordered_map_value_config = server::Config::Lookup("system.str_int_unordered_map", std::unordered_map<std::string, int>{{"k", 2}}, "system str int unordered_map");

    g_person = server::Config::Lookup("class.person", Person(), "class person");
    g_person_map = server::Config::Lookup("class.map", std::map<std::string, Person>(), "class map");
    g_person_vec_map = server::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person>>(), "class vec_map");

    // test_yaml();
    test_config();
    // test_class();
    // test_log();

    // server::Config::Visit([](server::ConfigVarBase::ptr var){
    //     SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "name=" << var->getName() << " description=" << var->getDescription() << " type.name=" << var->getTypeName()
    //                                        << " value=" << var->toString();
    // });


    return 0;
}