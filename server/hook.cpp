#include "hook.h"
#include "fiber.h"
#include "iomanager.h"
#include "fd_manager.h"

#include <iostream>
#include <dlfcn.h>
#include <sys/uio.h>

namespace server {

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg)


void hook_init() {
    static bool is_inited = false;
    if(is_inited) {
        return ;
    }
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX

}

struct _HookIniter {
    _HookIniter() {
        hook_init();
    }
};

static _HookIniter s_hook_initer;

bool is_hook_enable() {
    return t_hook_enable;
}

void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

}

extern "C" {

#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

struct timer_info {
    int cancelled = 0;
};

template<typename OriginFun, typename ... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name, uint32_t event, int timeout_so, ssize_t buflen, Args&&... args) {
    if(!server::t_hook_enable) {
        return fun(fd, std::forward<Args>(args)...);
    }

    server::FdCtx::ptr ctx = server::FdMgr::GetInstance()->get(fd);
    if(!ctx) {
        return fun(fd, std::forward<Args>(args)...);
    }

    if(ctx->isClose()) {
        errno == EBADF;
        return -1;
    }

    if(!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

    ssize_t n = fun(fd, std::forward<Args>(args)...);
    while(n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }
    if(n == -1 && errno == EAGAIN) {
        server::IOManager* iom = server::IOManager::GetThis();
        server::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);

        if(to != (uint64_t)-1) {
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event]() {
                auto t = winfo.lock();
                if(!t || t->cancelled) {
                    return ;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, (server::IOManager::Event)(event));
            }, winfo);
        }

        int c = 0;
        uint64_t now = 0;

        int rt = iom->addTimer(fd, (server::IOManager::Event)(event));
    }

}

unsigned int sleep(unsigned int seconds) {
    if(!server::t_hook_enable) {
        return sleep_f(seconds);
    }

    server::Fiber::ptr fiber = server::Fiber::GetThis();
    server::IOManager* iom = server::IOManager::GetThis();
    // iom->addTimer(seconds * 1000, std::bind(&server::IOManager::schedule<server::Fiber>, iom, fiber));
    iom->addTimer(seconds * 1000, [iom, fiber]() {
        iom->schedule(fiber);
    });
    server::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec) {
    if(!server::t_hook_enable) {
        return usleep_f(usec);
    }

    server::Fiber::ptr fiber = server::Fiber::GetThis();
    server::IOManager* iom = server::IOManager::GetThis();
    // iom->addTimer(usec / 1000, std::bind(&server::IOManager::schedule<server::Fiber>, iom, fiber));
    iom->addTimer(usec / 1000, [iom, fiber]() {
        iom->schedule(fiber);
    });
    server::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if(!server::t_hook_enable) {
        return nanosleep_f(req, rem);
    }

    server::Fiber::ptr fiber = server::Fiber::GetThis();
    server::IOManager* iom = server::IOManager::GetThis();
    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
    iom->addTimer(timeout_ms, [iom, fiber]() {
        iom->schedule(fiber);
    });
    server::Fiber::YieldToHold();
    return 0;
}

}