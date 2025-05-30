#pragma once

#include <ucontext.h>
#include <memory>
#include <functional>
#include "thread.h"

namespace server {

class Scheduler;
class Fiber : public std::enable_shared_from_this<Fiber> {
friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT
    };
private:
    Fiber();
public:
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
    ~Fiber();

    //重置协程函数，并重置状态  INIT, TERM
    void reset(std::function<void()> cb);
    //切换到当前协程执行
    void swapIn();
    //切换到后台执行
    void swapOut();

    void call();
    void back();

    uint64_t getId() { return m_id; }
    State getState() const { return m_state; }

    //返回当前协程
    static Fiber::ptr GetThis();
    //设置当前协程
    static void SetThis(Fiber* f);
    //协程切换到后台，设置为Ready状态
    static void YieldToReady();
    //协程切换到后台，设置为Hold状态
    static void YieldToHold();
    static uint64_t GetFiberId();

    //总协程数
    static uint64_t TotalFibers();

    static void MainFunc();
    static void CallerMainFunc();

private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;
    ucontext_t m_ctx;
    void* m_stack = nullptr;
    std::function<void()> m_cb;
};

}