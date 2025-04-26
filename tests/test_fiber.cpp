#include "server/server.h"

server::Logger::ptr g_logger = SERVER_LOG_ROOT();

void run_in_fiber() {
    SERVER_LOG_INFO(g_logger) << "run_in_fiber begin";
    server::Fiber::YieldToHold();
    SERVER_LOG_INFO(g_logger) << "run_in_fiber end";
    server::Fiber::YieldToHold();
}

void test_fiber() {
    server::Fiber::GetThis();
    SERVER_LOG_INFO(g_logger) << "main begin";
    server::Fiber::ptr fiber(new server::Fiber(run_in_fiber));
    fiber->swapIn();
    SERVER_LOG_INFO(g_logger) << "main after swap";
    fiber->swapIn();
    SERVER_LOG_INFO(g_logger) << "main after end";
    fiber->swapIn();
    SERVER_LOG_INFO(g_logger) << "main after end2";
}

int main() {
    server::Thread::SetName("main");
    
    std::vector<server::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i) {
        thrs.push_back(server::Thread::ptr(new server::Thread(&test_fiber, "name_" + std::to_string(i))));
    }

    for(auto i : thrs) {
        i->join();
    }

    return 0;
}