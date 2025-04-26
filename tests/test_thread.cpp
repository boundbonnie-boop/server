#include "server/server.h"
#include <unistd.h>

server::Logger::ptr g_logger = SERVER_LOG_ROOT();

int count = 0;
// server::RWMutex s_mutex;
server::Mutex s_mutex;

void fun1() {
    SERVER_LOG_INFO(g_logger) << "name:" << server::Thread::GetName() 
                              << " this.name:" << server::Thread::GetThis()->getName()
                              << " id:" << server::GetThreadId()
                              << " this.id:" << server::Thread::GetThis()->getId();
    for(int i = 0; i < 100000; ++i) {
        // server::RWMutex::WriteLock lock(s_mutex);
        server::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void fun2() {
    while(true) {
        SERVER_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void fun3() {
    while(true) {
        SERVER_LOG_INFO(g_logger) << "=======================";
    }
}

int main() {
    SERVER_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/jiaozexin/workspace/c++/server/bin/conf/log.yml");
    server::Config::LoadFromYaml(root);

    std::vector<server::Thread::ptr> thrs;
    for(int i = 0; i < 2; ++i) {
        server::Thread::ptr thr(new server::Thread(&fun2, "name_" + std::to_string(i * 2)));
        thrs.push_back(thr);
        server::Thread::ptr thr2(new server::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr2);
    }

    for(size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }
    SERVER_LOG_INFO(g_logger) << "thread test end";
    SERVER_LOG_INFO(g_logger) << "count=" << count;
    
    return 0;
}