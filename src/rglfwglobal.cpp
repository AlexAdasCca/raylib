#include "rglfwglobal.h"

#include <atomic>
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
