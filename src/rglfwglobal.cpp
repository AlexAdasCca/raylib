#include "rglfwglobal.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

// IMPORTANT (Win32): Do NOT include <windows.h> in this translation unit.
// Many raylib builds (and some downstream unity builds) include GLFW headers
// before Windows headers, which can trigger macro redefinition warnings such as
// APIENTRY. To keep the integration friction-free, we use a tiny set of forward
// declarations for the few Win32 APIs needed for thread naming.
#if defined(_WIN32)
    #ifndef WINAPI
        #define WINAPI __stdcall
    #endif
    typedef void* HANDLE;
    typedef void* HMODULE;
    typedef const wchar_t* PCWSTR;
    typedef const wchar_t* LPCWSTR;
    typedef wchar_t* LPWSTR;
    typedef const char* LPCSTR;
    typedef unsigned long DWORD;
    typedef unsigned long ULONG_PTR;
    typedef long HRESULT;
    typedef void* FARPROC;

    #ifndef CP_UTF8
        #define CP_UTF8 65001u
    #endif

    #if defined(_MSC_VER)
        #ifndef EXCEPTION_EXECUTE_HANDLER
            #define EXCEPTION_EXECUTE_HANDLER 1
        #endif
    #endif

    extern "C" __declspec(dllimport) HMODULE WINAPI GetModuleHandleW(LPCWSTR lpModuleName);
    extern "C" __declspec(dllimport) FARPROC WINAPI GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
    extern "C" __declspec(dllimport) HANDLE  WINAPI GetCurrentThread(void);
    extern "C" __declspec(dllimport) int     WINAPI MultiByteToWideChar(unsigned int CodePage, DWORD dwFlags,
                                                                          const char* lpMultiByteStr, int cbMultiByte,
                                                                          LPWSTR lpWideCharStr, int cchWideChar);
    #if defined(_MSC_VER)
        extern "C" __declspec(dllimport) void WINAPI RaiseException(DWORD dwExceptionCode, DWORD dwExceptionFlags,
                                                                     DWORD nNumberOfArguments, const ULONG_PTR* lpArguments);
    #endif
#endif

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

static std::atomic<uint32_t> gRlInternalThreadSeq{0};

#if defined(_WIN32) && defined(_MSC_VER)
// MSVC debugger thread name fallback (RaiseException 0x406D1388).
// IMPORTANT: Keep this helper free of C++ objects that require unwinding;
// MSVC forbids __try in functions that may need C++ stack unwinding.
static void rlWin32SetThreadNameException(const char* nameUtf8)
{
    if (nameUtf8 == nullptr || nameUtf8[0] == '\0') return;

    #pragma pack(push, 8)
    typedef struct tagTHREADNAME_INFO
    {
        DWORD dwType;     // Must be 0x1000.
        LPCSTR szName;    // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1 = caller thread).
        DWORD dwFlags;    // Reserved for future use, must be zero.
    } THREADNAME_INFO;
    #pragma pack(pop)

    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = nameUtf8;
    info.dwThreadID = (DWORD)-1;
    info.dwFlags = 0;

    __try { RaiseException(0x406D1388, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info); }
    __except(EXCEPTION_EXECUTE_HANDLER) { }
}
#endif

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

extern "C" void RLThreadSetNameCurrent(const char* nameUtf8)
{
    if (nameUtf8 == nullptr || nameUtf8[0] == '\0') return;

#if defined(_WIN32)
    // Prefer SetThreadDescription (Win10+). We load it dynamically so the binary
    // still runs on older Windows versions.
    using SetThreadDescriptionFn = HRESULT (WINAPI *)(HANDLE, PCWSTR);

    HMODULE hKernel32 = GetModuleHandleW(L"Kernel32.dll");
    if (hKernel32)
    {
        auto pSetThreadDescription = reinterpret_cast<SetThreadDescriptionFn>(
            GetProcAddress(hKernel32, "SetThreadDescription"));

        if (pSetThreadDescription)
        {
            int wlen = MultiByteToWideChar(CP_UTF8, 0, nameUtf8, -1, nullptr, 0);
            if (wlen > 0)
            {
                std::wstring wname;
                wname.resize(static_cast<size_t>(wlen));
                MultiByteToWideChar(CP_UTF8, 0, nameUtf8, -1, &wname[0], wlen);
                (void)pSetThreadDescription(GetCurrentThread(), wname.c_str());
                return;
            }
        }
    }

    // Fallback for older debuggers: MSVC-compatible thread naming exception.
    // This is best-effort and only useful when a debugger is attached.
	#if defined(_MSC_VER)
		rlWin32SetThreadNameException(nameUtf8);
	#endif
#else
    (void)nameUtf8;
#endif
}

static inline std::string rlMakeDefaultThreadName(void)
{
    const uint32_t id = gRlInternalThreadSeq.fetch_add(1, std::memory_order_relaxed) + 1;
    return std::string("raylib:thread:") + std::to_string(id);
}

extern "C" RLThread* RLThreadCreateNamed(RLThreadFn fn, void* user, const char* nameUtf8)
{
    if (!fn) return nullptr;

    std::string name = (nameUtf8 != nullptr && nameUtf8[0] != '\0')? std::string(nameUtf8) : rlMakeDefaultThreadName();

    RLThread* th = new RLThread{
        std::thread([fn, user, name]() {
            RLThreadSetNameCurrent(name.c_str());
            fn(user);
        })
    };
    return th;
}

extern "C" RLThread* RLThreadCreate(RLThreadFn fn, void* user)
{
    return RLThreadCreateNamed(fn, user, nullptr);
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
