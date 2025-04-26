#pragma once

#include <string>
#include <assert.h>
#include "util.h"

#define SERVER_ASSERT(x) \
    if(!(x)) { \
        SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << server::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#define SERVER_ASSERT2(x, w) \
    if(!(x)) { \
        SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << server::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }
