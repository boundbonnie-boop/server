#include "hook.h"
#include "fiber.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "log.h"
#include "config.h"

#include <iostream>
#include <dlfcn.h>
#include <sys/uio.h>

server::Logger::ptr g_logger = SERVER_LOG_NAME("system");

namespace server {

static server::ConfigVar<int>::ptr g_tcp_connect_timeout = 
    server::Config::Lookup<int>("tcp.connect.timeout", (int)5000, "tcp connect timeout");
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

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const uint64_t& old_value, const uint64_t& new_value) {
            SERVER_LOG_INFO(g_logger) << "tcp connect timeout changed, old_value="
                << old_value << ", new_value=" << new_value;
            s_connect_timeout = new_value;
        });
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

struct timer_info {
    int cancelled = 0;
};

template<typename OriginFun, typename ... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name, uint32_t event, int timeout_so, Args&&... args) {
    if(!server::t_hook_enable) {
        return fun(fd, std::forward<Args>(args)...);
    }
    
    server::FdCtx::ptr ctx = server::FdMgr::GetInstance()->get(fd);
    if(!ctx) {
        return fun(fd, std::forward<Args>(args)...);
    }

    if(ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    if(!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    while(n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }
    SERVER_LOG_INFO(g_logger) << "do_io<" << hook_fun_name << " >";
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

        int rt = iom->addEvent(fd, (server::IOManager::Event)(event));
        if(rt) {
            SERVER_LOG_ERROR(g_logger) << hook_fun_name << "addEvent(" << fd
                << ", " << event << ")";
            if(timer) {
                timer->cancel();
            }
            return -1;
        } else {
            server::Fiber::YieldToHold();
            if(timer) {
                timer->cancel();
            }
            if(tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }
            goto retry;
        }
    }
    return n;
}

extern "C" {

#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
    if(!server::t_hook_enable) {
        return sleep_f(seconds);
    }

    server::Fiber::ptr fiber = server::Fiber::GetThis();
    server::IOManager* iom = server::IOManager::GetThis();
    iom->addTimer(seconds * 1000, std::bind((void(server::Scheduler::*)(server::Fiber::ptr, int thread))&server::IOManager::schedule, iom, fiber, -1));
    server::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec) {
    if(!server::t_hook_enable) {
        return usleep_f(usec);
    }

    server::Fiber::ptr fiber = server::Fiber::GetThis();
    server::IOManager* iom = server::IOManager::GetThis();
    iom->addTimer(usec / 1000, std::bind((void(server::Scheduler::*)(server::Fiber::ptr, int thread))&server::IOManager::schedule, iom, fiber, -1));
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
    iom->addTimer(timeout_ms, std::bind((void(server::Scheduler::*)(server::Fiber::ptr, int thread))&server::IOManager::schedule, iom, fiber, -1));
    server::Fiber::YieldToHold();
    return 0;
}

int socket(int domain, int type, int protocol) {
    if(!server::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }

    int fd = socket_f(domain, type, protocol);
    if(fd == -1) {
        return fd;
    }
    server::FdMgr::GetInstance()->get(fd, true);
    return fd;
}

int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms) {
    if(!server::t_hook_enable) {
        return connect_f(sockfd, addr, addrlen);
    }

    server::FdCtx::ptr ctx = server::FdMgr::GetInstance()->get(sockfd);
    if(!ctx || ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    if(!ctx->isSocket()) {
        return connect_f(sockfd, addr, addrlen);
    }

    if(ctx->getUserNonblock()) {
        return connect_f(sockfd, addr, addrlen);
    }

    int n = connect_f(sockfd, addr, addrlen);
    if(n == 0) {
        return n;
    } else if(n != -1 || errno != EINPROGRESS) {
        return n;
    }

    server::IOManager* iom = server::IOManager::GetThis();
    server::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    if(timeout_ms != (uint64_t)-1) {
        timer = iom->addConditionTimer(timeout_ms, [winfo, sockfd, iom]() {
            auto t = winfo.lock();
            if(!t || t->cancelled) {
                return ;
            }
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(sockfd, server::IOManager::WRITE);
        }, winfo);
    }
    int rt = iom->addEvent(sockfd, server::IOManager::WRITE);
    if(rt == 0) {
        server::Fiber::YieldToHold();
        if(timer) {
            timer->cancel();
        }
        if(tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    } else {
        if(timer) {
            timer->cancel();
        }
        SERVER_LOG_ERROR(g_logger) << "connect addEvent(" << sockfd << ", WRITE) error";
    }

    int error = 0;
    socklen_t len = sizeof(int);
    if(-1 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    if(!error) {
        return 0;
    } else {
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, server::s_connect_timeout);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(sockfd, accept_f, "accept", server::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if(fd >= 0) {
        server::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}

int close(int fd) {
    if(!server::t_hook_enable) {
        return close_f(fd);
    }

    server::FdCtx::ptr ctx = server::FdMgr::GetInstance()->get(fd);
    if(ctx) {
        auto iom = server::IOManager::GetThis();
        if(iom) {
            iom->cancelAll(fd);
        }
        server::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

int fcntl(int fd, int cmd, ...) {
    va_list va;
    va_start(va, cmd);
    switch(cmd) {
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                server::FdCtx::ptr ctx = server::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setUserNonblock(arg & O_NONBLOCK);
                if(ctx->getSysNonblock()) {
                    arg |= O_NONBLOCK;
                } else {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                server::FdCtx::ptr ctx = server::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return arg;
                }
                if(ctx->getUserNonblock()) {
                    return arg | O_NONBLOCK;
                } else {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_NOTIFY:
        case F_SETPIPE_SZ:
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
        case F_GETPIPE_SZ:
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_ex* arg = va_arg(va, struct f_owner_ex*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        default:
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
    }
}

int ioctl(int fd, unsigned long int request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if(FIONBIO == request) {
        bool user_nonblock = !!*(int*)arg;
        server::FdCtx::ptr ctx = server::FdMgr::GetInstance()->get(fd);
        if(!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(fd, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(fd, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if(!server::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }

    if(level == SOL_SOCKET) {
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            server::FdCtx::ptr ctx = server::FdMgr::GetInstance()->get(sockfd);
            if(ctx) {
                const timeval* v = (const timeval*)optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", server::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", server::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", server::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", server::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", server::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", server::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", server::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    return do_io(sockfd, send_f, "send", server::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    return do_io(sockfd, sendto_f, "sendto", server::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    return do_io(sockfd, sendmsg_f, "sendmsg", server::IOManager::WRITE, SO_SNDTIMEO, msg, flags);  
}

}
