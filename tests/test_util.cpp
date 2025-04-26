#include "server/server.h"
#include <assert.h>

server::Logger::ptr g_logger = SERVER_LOG_ROOT();

void test_assert() {
    SERVER_LOG_INFO(g_logger) << server::BacktraceToString(10);
    SERVER_ASSERT2(0 == 1, "asd xx");
}

int main() {
    test_assert();
    return 0;
}