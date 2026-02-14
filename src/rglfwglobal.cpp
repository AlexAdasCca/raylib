#include "rglfwglobal.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

// GLFW is used by the desktop backend.
// This TU is compiled as C++ to use standard synchronization primitives.
#include "GLFW/glfw3.h"

// NOTE: Recursive to allow (rare) nested uses where platform code holds the
// global GLFW lock and then calls functions that also take it (Acquire/Release).
static std::recursive_mutex gRlGlfwMutex;
static std::atomic<int> gRlGlfwRefCount{0};

static std::thread::id gRlGlfwEventThread;
static std::atomic<bool> gRlGlfwEventThreadSet{false};

extern "C" bool RLGlfwGlobalAcquire(void)
{
    std::lock_guard<std::recursive_mutex> guard(gRlGlfwMutex);

    const int prev = gRlGlfwRefCount.fetch_add(1, std::memory_order_acq_rel);
    if (prev == 0)
    {
        if (glfwInit() != GLFW_TRUE)
        {
            (void)gRlGlfwRefCount.fetch_sub(1, std::memory_order_acq_rel);
            return false;
        }

        gRlGlfwEventThread = std::this_thread::get_id();
        gRlGlfwEventThreadSet.store(true, std::memory_order_release);
    }

    return true;
}

extern "C" void RLGlfwGlobalRelease(void)
{
    bool doTerminate = false;

    {
        std::lock_guard<std::recursive_mutex> guard(gRlGlfwMutex);

        const int cur = gRlGlfwRefCount.load(std::memory_order_acquire);
        if (cur <= 0)
            return;

        const int next = cur - 1;
        gRlGlfwRefCount.store(next, std::memory_order_release);

        if (next == 0)
        {
            // Termination can block (and may pump messages), so perform it outside the global lock.
            doTerminate = true;
            gRlGlfwEventThread = std::thread::id{};
            gRlGlfwEventThreadSet.store(false, std::memory_order_release);
        }
    }

    if (doTerminate)
    {
        glfwTerminate();
    }
}

extern "C" void RLGlfwGlobalLock(void)
{
    gRlGlfwMutex.lock();
}

extern "C" void RLGlfwGlobalUnlock(void)
{
    gRlGlfwMutex.unlock();
}

extern "C" bool RLGlfwIsEventPumpThread(void)
{
    if (!gRlGlfwEventThreadSet.load(std::memory_order_acquire)) return true;
    return std::this_thread::get_id() == gRlGlfwEventThread;
}

extern "C" void RLGlfwSetEventPumpThreadToCurrent(void)
{
    std::lock_guard<std::recursive_mutex> guard(gRlGlfwMutex);
    gRlGlfwEventThread = std::this_thread::get_id();
    gRlGlfwEventThreadSet.store(true, std::memory_order_release);
}

// ---------------------------------------------------------------------------------
// Thread/sync primitives (C ABI)
// ---------------------------------------------------------------------------------

struct RLThread
{
    std::thread t;
};

struct RLMutex
{
    std::mutex m;
};

struct RLEvent
{
    std::mutex m;
    std::condition_variable cv;
    bool signaled{false};
};

extern "C" RLThread* RLThreadCreate(RLThreadFn fn, void* user)
{
    if (!fn) return nullptr;
    RLThread* th = new RLThread{
        std::thread([fn, user]() { fn(user); })
    };
    return th;
}

extern "C" void RLThreadJoin(RLThread* t)
{
    if (!t) return;
    if (t->t.joinable()) t->t.join();
}

extern "C" void RLThreadDestroy(RLThread* t)
{
    if (!t) return;
    if (t->t.joinable()) t->t.join();
    delete t;
}

extern "C" RLMutex* RLMutexCreate(void)
{
    return new RLMutex{};
}

extern "C" void RLMutexLock(RLMutex* m)
{
    if (!m) return;
    m->m.lock();
}

extern "C" void RLMutexUnlock(RLMutex* m)
{
    if (!m) return;
    m->m.unlock();
}

extern "C" void RLMutexDestroy(RLMutex* m)
{
    delete m;
}

extern "C" RLEvent* RLEventCreate(bool initialSignaled)
{
    RLEvent* e = new RLEvent{};
    e->signaled = initialSignaled;
    return e;
}

extern "C" void RLEventSignal(RLEvent* e)
{
    if (!e) return;
    {
        std::lock_guard<std::mutex> lock(e->m);
        e->signaled = true;
    }
    e->cv.notify_one();
}

extern "C" void RLEventReset(RLEvent* e)
{
    if (!e) return;
    std::lock_guard<std::mutex> lock(e->m);
    e->signaled = false;
}

static inline void rlEventWaitImpl(RLEvent* e)
{
    std::unique_lock<std::mutex> lock(e->m);
    e->cv.wait(lock, [e]() { return e->signaled; });
    // Auto-reset
    e->signaled = false;
}

extern "C" void RLEventWait(RLEvent* e)
{
    if (!e) return;
    rlEventWaitImpl(e);
}

extern "C" bool RLEventWaitTimeout(RLEvent* e, uint32_t timeoutMs)
{
    if (!e) return false;
    std::unique_lock<std::mutex> lock(e->m);
    const bool ok = e->cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [e]() { return e->signaled; });
    if (ok)
    {
        // Auto-reset
        e->signaled = false;
    }
    return ok;
}

extern "C" void RLEventDestroy(RLEvent* e)
{
    delete e;
}
