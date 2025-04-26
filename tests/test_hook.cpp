#include "server/hook.h"
#include "server/iomanager.h"
#include "server/log.h"

server::Logger::ptr g_logger = SERVER_LOG_ROOT();

void test_sleep() {
    server::IOManager iom(1);
    iom.schedule([]() {
        sleep(2);
        SERVER_LOG_INFO(g_logger) << "sleep 2";
    });

    iom.schedule([]() {
        sleep(3);
        SERVER_LOG_INFO(g_logger) << "sleep 3";
    });

    SERVER_LOG_INFO(g_logger) << "test_sleep";
}

int main() {
    test_sleep();

    return 0;
}