#include "server/server.h"

static server::Logger::ptr g_logger = SERVER_LOG_ROOT();

void test_fiber() {
    static int s_count = 5;
    SERVER_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    sleep(1);
    if(--s_count >= 0) {
        server::Scheduler::GetThis()->schedule(&test_fiber);
    }
    
}

int main() {
    SERVER_LOG_INFO(g_logger) << "main";
    server::Scheduler sc(1, true, "test");
    sc.start();
    SERVER_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    SERVER_LOG_INFO(g_logger) << "stop";
    sc.stop();
    SERVER_LOG_INFO(g_logger) << "over";
    return 0;
}