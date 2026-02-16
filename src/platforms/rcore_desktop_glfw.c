/**********************************************************************************************
*
*   rcore_desktop_glfw - Functions to manage window, graphics device and inputs
*
*   PLATFORM: DESKTOP: GLFW
*       - Windows (Win32, Win64)
*       - Linux (X11/Wayland desktop mode)
*       - FreeBSD, OpenBSD, NetBSD, DragonFly (X11 desktop)
*       - OSX/macOS (x64, arm64)
*
*   LIMITATIONS:
*       - Limitation 01
*       - Limitation 02
*
*   POSSIBLE IMPROVEMENTS:
*       - Improvement 01
*       - Improvement 02
*
*   CONFIGURATION:
*       #define RCORE_PLATFORM_CUSTOM_FLAG
*           Custom flag for rcore on target platform -not used-
*
*   DEPENDENCIES:
*       - rglfw: Manage graphic device, OpenGL context and inputs (Windows, Linux, OSX/macOS, FreeBSD...)
*       - gestures: Gestures system for touch-ready devices (or simulated from mouse inputs)
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2013-2026 Ramon Santamaria (@raysan5) and contributors
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#define GLFW_INCLUDE_NONE       // Disable the standard OpenGL header inclusion on GLFW3
                                // NOTE: Already provided by rlgl implementation (on glad.h)
#include "GLFW/glfw3.h"         // GLFW3 library: Windows, OpenGL context and Input management
                                // NOTE: GLFW3 already includes gl.h (OpenGL) headers

// Support retrieving native window handlers
#if defined(_WIN32)
    #if !defined(HWND) && !defined(_MSVC_LANG)
        #define HWND void*
    #elif !defined(HWND) && defined(_MSVC_LANG)
        typedef struct HWND__ *HWND;
    #endif

    #include "../external/win32_clipboard.h" // Clipboard image copy-paste

    #define GLFW_EXPOSE_NATIVE_WIN32
    #define GLFW_NATIVE_INCLUDE_NONE // To avoid some symbols re-definition in windows.h
    #include "GLFW/glfw3native.h"

    #if defined(SUPPORT_WINMM_HIGHRES_TIMER) && !defined(SUPPORT_BUSY_WAIT_LOOP)
        // NOTE: Those functions require linking with winmm library
        //#pragma warning(disable: 4273)
        __declspec(dllimport) unsigned int __stdcall timeEndPeriod(unsigned int uPeriod);
        //#pragma warning(default: 4273)
    #endif
#endif
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    #include <sys/time.h>               // Required for: timespec, nanosleep(), select() - POSIX

    #if defined(_GLFW_X11) || defined(_GLFW_WAYLAND)
                                        // Set appropriate expose macros based on available backends
        #if defined(_GLFW_X11)
            #define GLFW_EXPOSE_NATIVE_X11
                #define RLFont X11Font    // Hack to fix 'Font' name collision
                                        // The definition and references to the X11 Font type will be replaced by 'X11Font'
                                        // Works as long as the current file consistently references any X11 Font as X11Font
                                        // Since it is never referenced (as of writing), this does not pose an issue
        #endif

        #if defined(_GLFW_WAYLAND)
            #define GLFW_EXPOSE_NATIVE_WAYLAND
        #endif

        #include "GLFW/glfw3native.h"   // Include native header only once, regardless of how many backends are defined
                                        // Required for: glfwGetX11Window() and glfwGetWaylandWindow()
        #if defined(_GLFW_X11)          // Clean up X11-specific hacks
            #undef RLFont                 // Revert hack and allow normal raylib Font usage
        #endif
    #endif
#endif
#if defined(__APPLE__)
    #include <unistd.h>                 // Required for: usleep()

    //#define GLFW_EXPOSE_NATIVE_COCOA    // WARNING: Fails due to type redefinition
    void *glfwGetCocoaWindow(GLFWwindow *handle);
    #include "GLFW/glfw3native.h"       // Required for: glfwGetCocoaWindow()
#endif

#include <stddef.h>  // Required for: size_t
#include <stdint.h>  // Required for: uintptr_t, intptr_t
#include <rl_context.h>
#include "rglfwglobal.h"

#if defined(_WIN32)
    // Assertions for the Win32 Route2/event-thread backend.
    // Enabled in Debug builds (when NDEBUG is not defined) or when RLGLFW_DIAGNOSTICS is defined.
    #if !defined(NDEBUG) || defined(RLGLFW_DIAGNOSTICS)
        #include <assert.h>
        #define RLGLFW_ASSERT(x) assert(x)
    #else
        #define RLGLFW_ASSERT(x) ((void)0)
    #endif
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct {
    GLFWwindow *handle;                 // GLFW window handle (graphic device)

#if defined(_WIN32)
    // When enabled, all GLFW event pumping and Win32 message processing happens
    // on a dedicated thread (the thread that owns the HWND). Rendering stays on
    // the caller thread.
    bool useEventThread;


    // If set (or implied by the primary window during shutdown), wake-required events
    // broadcast to all windows' render threads. Otherwise only wake the current window.
    bool broadcastWake;

    // Global registry helpers
    int isRegistered;                   // Whether this PlatformData is in gRlGlfwPdHead
    HWND win32Hwnd;                     // Cached HWND for the window (set after creation)

	RLContext *ownerCtx;                // Owning RLContext for this window (used for cross-thread render dispatch)

    // Thread handles (GLFW per-thread contexts).
    GLFWthread *renderThread;           // Thread that owns the OpenGL/Vulkan context
    GLFWthread *eventThread;            // Thread that owns the Win32 window/message queue

    // OS thread handle (C ABI wrapper) for the event thread.
    RLThread *eventThreadHandle;

    // Startup / wake coordination.
    RLEvent *createdEvent;              // Signaled after window creation + callbacks are set
    RLEvent *renderWakeEvent;           // Signaled to wake render thread when waiting for events

    volatile int eventThreadStop;       // Non-zero => event thread should exit
    volatile int closing;               // Non-zero => context/window is closing (drop non-critical tasks)

#if RL_EVENTTHREAD_COALESCE_STATE
    // Coalesced pending input/state for Win32 event-thread mode
    volatile long pendingMask;
    volatile long pendingQueued;
    // mouse move (last)
    volatile long pendingMouseXBits;
    volatile long pendingMouseYBits;
    // wheel (accumulated, fixed-point)
    volatile long pendingWheelX_fp;
    volatile long pendingWheelY_fp;
    // window pos (last)
    volatile long pendingWinX;
    volatile long pendingWinY;
    // content scale (last)
    volatile long pendingScaleXBits;
    volatile long pendingScaleYBits;
    // framebuffer size (last)
    volatile long pendingFbW;
    volatile long pendingFbH;
#endif
#endif
} PlatformData;

#if defined(_WIN32)
// ----------------------------------------------------------------------------------
// Win32: registry of event-thread platforms (for broadcast wake on shutdown)
// ----------------------------------------------------------------------------------
typedef struct RLGlfwPlatformNode
{
    PlatformData *pd;
    struct RLGlfwPlatformNode *next;
} RLGlfwPlatformNode;

static RLGlfwPlatformNode *gRlGlfwPdHead = NULL;

// Primary window tracking + global quit semantics (Win32 Route2)
// Primary is defined as the first created GLFW window in the process.
static GLFWwindow *gRlGlfwPrimaryWindow = NULL;
static long gRlGlfwWindowCount = 0;
static volatile long gRlGlfwGlobalQuitRequested = 0;

#if defined(_MSC_VER)
    #include <intrin.h>
    #pragma intrinsic(_InterlockedExchange)
    #pragma intrinsic(_InterlockedCompareExchange)
#endif

static void RLGlfwResetGlobalQuit(void)
{
#if defined(_MSC_VER)
    _InterlockedExchange(&gRlGlfwGlobalQuitRequested, 0);
#else
    __sync_lock_test_and_set(&gRlGlfwGlobalQuitRequested, 0);
#endif
}

static void RLGlfwRequestGlobalQuit(void)
{
#if defined(_MSC_VER)
    _InterlockedExchange(&gRlGlfwGlobalQuitRequested, 1);
#else
    __sync_lock_test_and_set(&gRlGlfwGlobalQuitRequested, 1);
#endif
}

static bool RLGlfwIsGlobalQuitRequested(void)
{
#if defined(_MSC_VER)
    return (_InterlockedCompareExchange(&gRlGlfwGlobalQuitRequested, 0, 0) != 0);
#else
    return (__sync_val_compare_and_swap(&gRlGlfwGlobalQuitRequested, 0, 0) != 0);
#endif
}

static bool RLGlfwIsPrimaryWindow(GLFWwindow *window)
{
    return (window != NULL) && (window == gRlGlfwPrimaryWindow);
}

static bool RLGlfwIsPrimaryPlatform(PlatformData *pd)
{
    return (pd != NULL) && RLGlfwIsPrimaryWindow(pd->handle);
}

// Track *all* windows (event-thread and non-event-thread). This keeps "primary" semantics
// consistent across modes and lets primary-close wake sleeping event-thread render loops.
static void RLGlfwTrackWindowCreated(GLFWwindow *window, bool globalLockHeld)
{
    if (window == NULL) return;
    if (!globalLockHeld) RLGlfwGlobalLock();

    if (gRlGlfwWindowCount == 0)
    {
        gRlGlfwPrimaryWindow = window;
        // Fresh run: clear stale quit so RLWindowShouldClose() does not instantly exit.
        RLGlfwResetGlobalQuit();
    }

    gRlGlfwWindowCount++;

    if (!globalLockHeld) RLGlfwGlobalUnlock();
}

static void RLGlfwTrackWindowDestroyed(GLFWwindow *window, bool globalLockHeld)
{
    if (window == NULL) return;
    if (!globalLockHeld) RLGlfwGlobalLock();

    if (gRlGlfwWindowCount > 0) gRlGlfwWindowCount--;

    // If all windows are gone, reset to allow a clean re-init.
    if (gRlGlfwWindowCount == 0)
    {
        gRlGlfwPrimaryWindow = NULL;
        RLGlfwResetGlobalQuit();
    }
    else if (gRlGlfwPrimaryWindow == window)
    {
        // Primary is being destroyed while others remain. At this point we are effectively
        // in global-shutdown semantics already; keep primary unset.
        gRlGlfwPrimaryWindow = NULL;
    }

    if (!globalLockHeld) RLGlfwGlobalUnlock();
}

// NOTE: This registry is accessed concurrently by multiple window threads.
// Use the global GLFW recursive lock (rglfwglobal.cpp) to avoid double-init races.
// This also avoids having multiple distinct mutex instances guarding the same list.

static void RLGlfwPlatformRegister(PlatformData *pd)
{
    if (pd == NULL) return;
    if (pd->isRegistered) return;
    RLGlfwGlobalLock();

    // Prevent duplicates.
    for (RLGlfwPlatformNode *it = gRlGlfwPdHead; it != NULL; it = it->next)
    {
        if (it->pd == pd) { pd->isRegistered = 1; RLGlfwGlobalUnlock(); return; }
    }

    RLGlfwPlatformNode *n = (RLGlfwPlatformNode *)RL_CALLOC(1, sizeof(RLGlfwPlatformNode));
    n->pd = pd;
    n->next = gRlGlfwPdHead;
    gRlGlfwPdHead = n;
    pd->isRegistered = 1;
    RLGlfwGlobalUnlock();
}

static void RLGlfwPlatformUnregister(PlatformData *pd)
{
    if (pd == NULL) return;
    if (!pd->isRegistered) return;
    RLGlfwGlobalLock();

    RLGlfwPlatformNode **pp = &gRlGlfwPdHead;
    while (*pp)
    {
        RLGlfwPlatformNode *cur = *pp;
        if (cur->pd == pd)
        {
            *pp = cur->next;
            RL_FREE(cur);
            pd->isRegistered = 0;
            break;
        }
        pp = &cur->next;
    }

    RLGlfwGlobalUnlock();
}

static void RLGlfwSignalAllRenderWake(void)
{
    RLGlfwGlobalLock();
    for (RLGlfwPlatformNode *it = gRlGlfwPdHead; it != NULL; it = it->next)
    {
        PlatformData *pd = it->pd;
        if (pd == NULL) continue;
        if (pd->renderWakeEvent) RLEventSignal(pd->renderWakeEvent);
        if (pd->renderThread) glfwWakeThread(pd->renderThread);
    }
    RLGlfwGlobalUnlock();
}

static void RLGlfwSignalOneRenderWake(PlatformData *pd)
{
    if (pd == NULL) return;
    if (pd->renderWakeEvent) RLEventSignal(pd->renderWakeEvent);
    if (pd->renderThread) glfwWakeThread(pd->renderThread);
}

static bool RLGlfwShouldBroadcastWake(PlatformData *pd, bool isShutdownOrClose)
{
    // During global quit, always wake every render thread to ensure shutdown completes.
    if (RLGlfwIsGlobalQuitRequested()) return true;

    // Explicit opt-in flag on this window enables broadcast wake behavior.
    if ((pd != NULL) && pd->broadcastWake) return true;

    // Primary window implies broadcast only for shutdown/close paths (not for normal refresh).
    if (isShutdownOrClose && RLGlfwIsPrimaryPlatform(pd)) return true;

    return false;
}

static void RLGlfwSignalWakeByPolicy(PlatformData *pd, bool isShutdownOrClose)
{
    // Always wake the current window render thread at least once.
    RLGlfwSignalOneRenderWake(pd);

    if (RLGlfwShouldBroadcastWake(pd, isShutdownOrClose))
    {
        RLGlfwSignalAllRenderWake();
    }
}

#endif

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
// Route2 Stage-A: platform becomes context-scoped (selected per-thread)
static inline PlatformData *RLGetPlatformDataPtr(void)
{
    RLContext *ctx = RLGetCurrentContext();
    if ((ctx != NULL) && (ctx->platformData == NULL)) ctx->platformData = RL_CALLOC(1, sizeof(PlatformData));
    return (PlatformData *)((ctx != NULL) ? ctx->platformData : NULL);
}
#define platform (*RLGetPlatformDataPtr())


//----------------------------------------------------------------------------------
// GPU resource sharing between contexts/windows
//----------------------------------------------------------------------------------
static GLFWwindow *RLGlfwGetPrimaryShareWindow(void)
{
#if defined(_WIN32)
    return gRlGlfwPrimaryWindow;
#else
    return NULL;
#endif
}

static GLFWwindow *RLGlfwResolveShareWindowForContext(RLContext *ctx)
{
    if (ctx == NULL) return NULL;

    RLContextResourceShareMode mode = (RLContextResourceShareMode)ctx->resourceShareMode;
    if (mode == RL_CONTEXT_SHARE_WITH_PRIMARY)
    {
        GLFWwindow *w = RLGlfwGetPrimaryShareWindow();
        return (w != NULL) ? w : NULL;
    }
    else if (mode == RL_CONTEXT_SHARE_WITH_CONTEXT)
    {
        RLContext *other = (RLContext *)ctx->resourceShareWith;
        if (other == NULL) return NULL;
        PlatformData *opd = (PlatformData *)other->platformData;
        if ((opd == NULL) || (opd->handle == NULL)) return NULL;
        return opd->handle;
    }

    return NULL;
}

//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------
int InitPlatform(void);          // Initialize platform (graphics, inputs and more)
void ClosePlatform(void);        // Close platform

#if defined(_WIN32)
//----------------------------------------------------------------------------------
// Win32: message/event thread separation helpers
//----------------------------------------------------------------------------------


#if RL_EVENTTHREAD_COALESCE_STATE
//----------------------------------------------------------------------------------
// Win32: coalesced pending state for event-thread mode
//----------------------------------------------------------------------------------
#define RL_PENDING_MOUSE_MOVE   (1L<<0)
#define RL_PENDING_WHEEL        (1L<<1)
#define RL_PENDING_WIN_POS      (1L<<2)
#define RL_PENDING_SCALE        (1L<<3)
#define RL_PENDING_FB_SIZE      (1L<<4)

// Accumulate wheel deltas as fixed-point integers, then convert back in the drain task.
#ifndef RL_WHEEL_FP_SCALE
    #define RL_WHEEL_FP_SCALE 1000L
#endif

#if defined(_MSC_VER)
    #include <intrin.h>
    static inline long RLAtomicExchangeLong(volatile long *p, long v) { return _InterlockedExchange(p, v); }
    static inline long RLAtomicCompareExchangeLong(volatile long *p, long desired, long expected) { return _InterlockedCompareExchange(p, desired, expected); }
    static inline long RLAtomicOrLong(volatile long *p, long v) { return _InterlockedOr(p, v); }
    static inline long RLAtomicAddLong(volatile long *p, long v) { return _InterlockedExchangeAdd(p, v) + v; }
    static inline long RLAtomicLoadLong(volatile long *p) { return _InterlockedCompareExchange(p, 0, 0); }
#else
    static inline long RLAtomicExchangeLong(volatile long *p, long v) { return __sync_lock_test_and_set(p, v); }
    static inline long RLAtomicCompareExchangeLong(volatile long *p, long desired, long expected) { return __sync_val_compare_and_swap(p, expected, desired); }
    static inline long RLAtomicOrLong(volatile long *p, long v) { return __sync_fetch_and_or(p, v); }
    static inline long RLAtomicAddLong(volatile long *p, long v) { return __sync_add_and_fetch(p, v); }
    static inline long RLAtomicLoadLong(volatile long *p) { return __sync_add_and_fetch(p, 0); }
#endif

static inline bool RLAtomicCASLong(volatile long *p, long expected, long desired)
{
    return (RLAtomicCompareExchangeLong(p, desired, expected) == expected);
}

static inline long RLFloatBitsFromFloat(float f)
{
    union { float f; uint32_t u; } cv;
    cv.f = f;
    return (long)cv.u;
}

static inline float RLFloatFromBits(long bits)
{
    union { float f; uint32_t u; } cv;
    cv.u = (uint32_t)bits;
    return cv.f;
}

static inline long RLWheelToFixed(double v)
{
    const double s = v*(double)RL_WHEEL_FP_SCALE;
    // round to nearest integer (ties away from zero)
    if (s >= 0.0) return (long)(s + 0.5);
    else return (long)(s - 0.5);
}
#endif // RL_EVENTTHREAD_COALESCE_STATE

typedef struct
{
    void (*fn)(void *user);
    void *user;
    RLEvent *done;
} RLGlfwThreadCall;

typedef struct
{
    RLContext *ctx;
    void (*fn)(void *user);
    void *user;
} RLGlfwRenderCall;

// Forward declaration: avoid implicit int prototype in C (MSVC) before definition.
static bool RLGlfwIsThread(GLFWthread* thr);

static void RLGlfwThreadCallTrampoline(void *p)
{
    RLGlfwThreadCall *call = (RLGlfwThreadCall *)p;
    if (call && call->fn) call->fn(call->user);
    if (call && call->done) RLEventSignal(call->done);
    if (call) RL_FREE(call);
}

static void RLGlfwRenderCallTrampoline(void *p)
{
    RLGlfwRenderCall *call = (RLGlfwRenderCall *)p;
    RL_DIAG_TASK_EXECUTED();
    if (call && call->ctx)
    {
        PlatformData *pd = (PlatformData *)call->ctx->platformData;
        if ((pd != NULL) && pd->useEventThread)
        {
            // Render-thread tasks must execute on the owning render thread.
            RLGLFW_ASSERT(pd->renderThread != NULL);
            RLGLFW_ASSERT(RLGlfwIsThread(pd->renderThread));

            // If the GLFWwindow exists, its user pointer must match the target RLContext.
            if (pd->handle != NULL) RLGLFW_ASSERT(glfwGetWindowUserPointer(pd->handle) == call->ctx);
        }

        RLSetCurrentContext(call->ctx);
    }
    if (call && call->fn) call->fn(call->user);
    if (call) { RL_DIAG_RENDERCALL_FREE(sizeof(RLGlfwRenderCall)); RL_FREE(call); }
}

static bool RLGlfwIsThread(GLFWthread *thr)
{
    if (!thr) return false;
    return (glfwGetCurrentThread() == thr);
}

static void RLGlfwWakeEventThread(void)
{
    if (platform.eventThread) glfwWakeThread(platform.eventThread);
}

static void RLGlfwWakeRenderThread(void)
{
    if (platform.renderWakeEvent) RLEventSignal(platform.renderWakeEvent);
    // Also set the GLFW wake-event, in case the render thread is blocked in GLFW.
    if (platform.renderThread) glfwWakeThread(platform.renderThread);
}

static void RLGlfwBarrierSignalTask(void *user)
{
    if (user) RLEventSignal((RLEvent *)user);
}

static void RLGlfwPumpThreadTasksWithDiag(void)
{
#if RL_EVENT_DIAG_STATS
    double t0 = RLGetTime();
    RL_DIAG_PUMP_BEGIN();
    glfwPumpThreadTasks();
    unsigned int n = RL_DIAG_PUMP_END();
    RL_DIAG_ON_PUMP(RLGetTime() - t0, n);
#else
    glfwPumpThreadTasks();
#endif
}

// Drain pending tasks posted to the current render thread. Used during shutdown to avoid
// executing tasks after the RLContext/Core are freed.
static void RLGlfwDrainRenderThreadTasks(void)
{
    // Only meaningful when the render thread exists and we are on it.
    if (!platform.renderThread || !RLGlfwIsThread(platform.renderThread))
    {
        // Best-effort: execute any tasks queued for the current thread.
        RLGlfwPumpThreadTasksWithDiag();
        return;
    }

    // Post a barrier task to the end of the queue and pump until it runs.
    RLEvent *done = RLEventCreate(false);
    if (done == NULL)
    {
        RLGlfwPumpThreadTasksWithDiag();
        return;
    }

    glfwPostTask(platform.renderThread, RLGlfwBarrierSignalTask, done);
    RLGlfwWakeRenderThread();

    // Pump until the barrier is observed.
    for (int spin = 0; spin < 100000; spin++)
    {
        RLGlfwPumpThreadTasksWithDiag();
        if (RLEventWaitTimeout(done, 0)) break;
    }

    RLEventDestroy(done);
}

static void RLGlfwRunOnEventThread(void (*fn)(void *user), void *user, bool wait)
{
    if (!platform.useEventThread || RLGlfwIsThread(platform.eventThread))
    {
        if (fn) fn(user);
        return;
    }

    // If the event thread isn't ready yet, execute synchronously (initialization fallback).
    if (!platform.eventThread)
    {
        if (fn) fn(user);
        return;
    }

    RLEvent *done = wait ? RLEventCreate(false) : NULL;

    RLGlfwThreadCall *call = (RLGlfwThreadCall *)RL_CALLOC(1, sizeof(RLGlfwThreadCall));
    call->fn = fn;
    call->user = user;
    call->done = done;

    glfwPostTask(platform.eventThread, RLGlfwThreadCallTrampoline, call);
    RLGlfwWakeEventThread();

    if (done)
    {
        RLEventWait(done);
        RLEventDestroy(done);
    }
}

static void RLGlfwRunOnRenderThread(RLContext *ctx, void (*fn)(void *user), void *user)
{
    // Render thread tasks should be idempotent and short.
    // If called on the render thread, execute immediately.
    if (!platform.useEventThread || RLGlfwIsThread(platform.renderThread))
    {
        RLSetCurrentContext(ctx);
        if (fn) fn(user);
        return;
    }

    // In event-thread mode we expect a dedicated render thread.
    // If it is missing (and we're not in shutdown), something is inconsistent.
    if (platform.useEventThread && !platform.closing)
    {
        RLGLFW_ASSERT(platform.renderThread != NULL);
    }

    // If render thread handle is missing, fall back to direct execution.
    if (!platform.renderThread)
    {
        RLSetCurrentContext(ctx);
        if (fn) fn(user);
        return;
    }

    RLGlfwRenderCall *call = (RLGlfwRenderCall *)RL_CALLOC(1, sizeof(RLGlfwRenderCall));
    RL_DIAG_RENDERCALL_ALLOC(sizeof(RLGlfwRenderCall));
    RL_DIAG_TASK_POSTED();
    call->ctx = ctx;
    call->fn = fn;
    call->user = user;

    glfwPostTask(platform.renderThread, RLGlfwRenderCallTrampoline, call);
    RLGlfwWakeRenderThread();
}

#if RL_EVENTTHREAD_COALESCE_STATE
// Forward declaration required because RLGlfwQueuePendingDrain() may be used before the
// task declarations section further down in this translation unit.
static void RLGlfwTask_DrainPendingInput(void *user);

static inline void RLGlfwQueuePendingDrain(RLContext *ctx, PlatformData *pd)
{
    if ((ctx == NULL) || (pd == NULL)) return;
    if (pd->closing) return;

    // Only queue one drain task at a time
    if (!RLAtomicCASLong(&pd->pendingQueued, 0, 1)) return;

    RLSetCurrentContext(ctx);
    RLGlfwRunOnRenderThread(ctx, RLGlfwTask_DrainPendingInput, pd);
}
#endif // RL_EVENTTHREAD_COALESCE_STATE


typedef struct
{
    RLContext *ctx;
} RLGlfwEventThreadStart;

static void RLGlfwEventThreadMain(void *p);

#endif // _WIN32

// Error callback event
static void ErrorCallback(int error, const char *description);                          // GLFW3 Error Callback, runs on GLFW3 error

// Window callbacks events
static void WindowSizeCallback(GLFWwindow *window, int width, int height);              // GLFW3 WindowSize Callback, runs when window is resized
static void FramebufferSizeCallback(GLFWwindow *window, int width, int height);         // GLFW3 FramebufferSize Callback, runs when window is resized
static void WindowContentScaleCallback(GLFWwindow *window, float scalex, float scaley); // GLFW3 Window Content Scale Callback, runs when window changes scale
static void WindowPosCallback(GLFWwindow *window, int x, int y);                        // GLFW3 WindowPos Callback, runs when window is moved
static void WindowIconifyCallback(GLFWwindow *window, int iconified);                   // GLFW3 WindowIconify Callback, runs when window is minimized/restored
static void WindowMaximizeCallback(GLFWwindow *window, int maximized);                  // GLFW3 Window Maximize Callback, runs when window is maximized
static void WindowRefreshCallback(GLFWwindow *window);                                   // GLFW3 Window Refresh Callback, runs when the OS requests a repaint
static void WindowCloseCallback(GLFWwindow *window);                                     // GLFW3 Window Close Callback, runs when the OS/user requests window close
static void WindowFocusCallback(GLFWwindow *window, int focused);                       // GLFW3 WindowFocus Callback, runs when window get/lose focus
static void WindowDropCallback(GLFWwindow *window, int count, const char **paths);      // GLFW3 Window Drop Callback, runs when drop files into window

// Input callbacks events
static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods); // GLFW3 Keyboard Callback, runs on key pressed
static void CharCallback(GLFWwindow *window, unsigned int codepoint);                   // GLFW3 Char Callback, runs on key pressed (get codepoint value)
static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);  // GLFW3 Mouse Button Callback, runs on mouse button pressed
static void MouseCursorPosCallback(GLFWwindow *window, double x, double y);             // GLFW3 Cursor Position Callback, runs on mouse move
static void MouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset);    // GLFW3 Scrolling Callback, runs on mouse wheel
static void CursorEnterCallback(GLFWwindow *window, int enter);                         // GLFW3 Cursor Enter Callback, cursor enters client area
static void JoystickCallback(int jid, int event);                                       // GLFW3 Joystick Connected/Disconnected Callback

// Monitor query bundle (can be filled by direct GLFW calls or by a Win32 event-thread query)
typedef struct
{
    int index;              // input
    int monitorCount;       // output
    GLFWmonitor *monitor;   // output
    const char *name;       // output (GLFW-owned)

    int posX; int posY;     // output
    int workX; int workY;   // output
    int workW; int workH;   // output

    int modeW; int modeH;   // output
    int refresh;            // output

    int physW; int physH;   // output (mm)
    int ok;                 // output: 1 if index valid
} RLGlfwMonitorInfo;

#if defined(_WIN32)
static void RLGlfwTask_DestroyWindow(void *user);

// Thread-affine window operations (Win32 event-thread mode)
static void RLGlfwTask_SetWindowShouldCloseTrue(void *user);
static void RLGlfwTask_SetWindowPos(void *user);
static void RLGlfwTask_SetWindowSize(void *user);
static void RLGlfwTask_SetWindowTitle(void *user);
static void RLGlfwTask_SetWindowAttrib(void *user);
static void RLGlfwTask_SetWindowRefreshCallback(void *user);
static void RLGlfwTask_SetWindowSizeLimits(void *user);
static void RLGlfwTask_SetWindowOpacity(void *user);
static void RLGlfwTask_SetWindowMonitor(void *user);
static void RLGlfwTask_ShowWindow(void *user);
static void RLGlfwTask_HideWindow(void *user);
static void RLGlfwTask_FocusWindow(void *user);
static void RLGlfwTask_IconifyWindow(void *user);
static void RLGlfwTask_MaximizeWindow(void *user);
static void RLGlfwTask_RestoreWindow(void *user);
static void RLGlfwTask_SetWindowIcon(void *user);
static void RLGlfwTask_QueryMonitorCount(void *user);
static void RLGlfwTask_QueryMonitorInfo(void *user);
static void RLGlfwTask_QueryCurrentMonitorIndex(void *user);
// Clipboard + content scale helpers (Win32 event-thread mode)
static void RLGlfwTask_SetClipboardText(void *user);
static void RLGlfwTask_GetClipboardText(void *user);
static void RLGlfwTask_GetWindowContentScale(void *user);

typedef struct { GLFWmonitor *monitor; int xpos; int ypos; int width; int height; int refreshRate; } RLGlfwMonitorTask;
typedef struct { int count; GLFWimage *icons; } RLGlfwIconTask;
typedef struct { const char *out; } RLGlfwClipboardGetTask;
typedef struct { float x; float y; } RLGlfwContentScaleTask;

// Small helpers to keep window-affine GLFW calls on the Win32 event thread.
static void RLGlfwSetWindowAttribThreadAware(int attrib, int value)
{
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        int av[2] = { attrib, value };
        RLGlfwRunOnEventThread(RLGlfwTask_SetWindowAttrib, av, true);
        return;
    }

    glfwSetWindowAttrib(platform.handle, attrib, value);
}

static void RLGlfwSetWindowRefreshCallbackThreadAware(int enable)
{
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        int v = enable;
        RLGlfwRunOnEventThread(RLGlfwTask_SetWindowRefreshCallback, &v, true);
        return;
    }

    glfwSetWindowRefreshCallback(platform.handle, enable ? WindowRefreshCallback : NULL);
}

static void RLGlfwHideWindowThreadAware(void)
{
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwRunOnEventThread(RLGlfwTask_HideWindow, NULL, true);
        return;
    }

    glfwHideWindow(platform.handle);
}

static void RLGlfwShowWindowThreadAware(void)
{
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwRunOnEventThread(RLGlfwTask_ShowWindow, NULL, true);
        return;
    }

    glfwShowWindow(platform.handle);
}

static void RLGlfwFocusWindowThreadAware(void)
{
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwRunOnEventThread(RLGlfwTask_FocusWindow, NULL, true);
        return;
    }

    glfwFocusWindow(platform.handle);
}

static void RLGlfwSetWindowMonitorThreadAware(GLFWmonitor *monitor, int xpos, int ypos, int width, int height, int refreshRate)
{
    RLGlfwMonitorTask task = { monitor, xpos, ypos, width, height, refreshRate };
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwRunOnEventThread(RLGlfwTask_SetWindowMonitor, &task, true);
        return;
    }

    glfwSetWindowMonitor(platform.handle, monitor, xpos, ypos, width, height, refreshRate);
}
#endif

// Memory allocator wrappers [used by glfwInitAllocator()]
static void *AllocateWrapper(size_t size, void *user);                                  // GLFW3 GLFWallocatefun, wrapps around RL_CALLOC macro
static void *ReallocateWrapper(void *block, size_t size, void *user);                   // GLFW3 GLFWreallocatefun, wrapps around RL_REALLOC macro
static void DeallocateWrapper(void *block, void *user);                                 // GLFW3 GLFWdeallocatefun, wraps around RL_FREE macro

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
// NOTE: Functions declaration is provided by raylib.h

//----------------------------------------------------------------------------------
// Module Functions Definition: Window and Graphics Device
//----------------------------------------------------------------------------------

// Check if application should close
// NOTE: By default, if KEY_ESCAPE pressed or window close icon clicked
bool RLWindowShouldClose(void)
{
#if defined(_WIN32)
    // Process-wide quit latch: closing the primary window requests global quit.
    if (RLGlfwIsGlobalQuitRequested()) return true;
#endif
    if (CORE.Window.ready) return CORE.Window.shouldClose;
    else return true;
}

// Toggle fullscreen mode
void RLToggleFullscreen(void)
{
    RLGlfwMonitorInfo info = { 0 };

    if (!FLAG_IS_SET(CORE.Window.flags, FLAG_FULLSCREEN_MODE))
    {
        // Store previous screen data (in case exiting fullscreen)
        CORE.Window.previousPosition = CORE.Window.position;
        CORE.Window.previousScreen = CORE.Window.screen;

        // Use current monitor the window is on to get fullscreen required size
        int monitorIndex = RLGetCurrentMonitor();
        info.index = monitorIndex;

#if defined(_WIN32)
        if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
        {
            RLGlfwRunOnEventThread(RLGlfwTask_QueryMonitorInfo, &info, true);
        }
        else
#endif
        {
            int monitorCount = 0;
            GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);
            info.monitorCount = monitorCount;

            if ((monitorIndex >= 0) && (monitorIndex < monitorCount))
            {
                GLFWmonitor *monitor = monitors[monitorIndex];
                const GLFWvidmode *mode = glfwGetVideoMode(monitor);

                info.monitor = monitor;
                info.name = glfwGetMonitorName(monitor);
                glfwGetMonitorPos(monitor, &info.posX, &info.posY);
                glfwGetMonitorWorkarea(monitor, &info.workX, &info.workY, &info.workW, &info.workH);
                glfwGetMonitorPhysicalSize(monitor, &info.physW, &info.physH);

                if (mode != NULL)
                {
                    info.modeW = mode->width;
                    info.modeH = mode->height;
                    info.refresh = mode->refreshRate;
                    info.ok = 1;
                }
            }
        }

        if (info.ok)
        {
            CORE.Window.display.width = info.modeW;
            CORE.Window.display.height = info.modeH;

            CORE.Window.position = (Point){ 0, 0 };
            CORE.Window.screen = CORE.Window.display;

            // Set fullscreen flag to be processed on FramebufferSizeCallback() accordingly
            FLAG_SET(CORE.Window.flags, FLAG_FULLSCREEN_MODE);

#if defined(_GLFW_X11) || defined(_GLFW_WAYLAND)
            // NOTE: X11 requires undecorating the window before switching to
            // fullscreen to avoid issues with framebuffer scaling
            glfwSetWindowAttrib(platform.handle, GLFW_DECORATED, GLFW_FALSE);
            FLAG_SET(CORE.Window.flags, FLAG_WINDOW_UNDECORATED);
#endif

            // WARNING: This function launches FramebufferSizeCallback()
#if defined(_WIN32)
            RLGlfwSetWindowMonitorThreadAware(info.monitor, 0, 0, CORE.Window.screen.width, CORE.Window.screen.height, GLFW_DONT_CARE);
#else
            glfwSetWindowMonitor(platform.handle, info.monitor, 0, 0, CORE.Window.screen.width, CORE.Window.screen.height, GLFW_DONT_CARE);
#endif
        }
        else TRACELOG(LOG_WARNING, "GLFW: Failed to get monitor");
    }
    else
    {
        // Restore previous window position and size
        CORE.Window.position = CORE.Window.previousPosition;
        CORE.Window.screen = CORE.Window.previousScreen;

        // Set fullscreen flag to be processed on FramebufferSizeCallback() accordingly
        // and considered by GetWindowScaleDPI()
        FLAG_CLEAR(CORE.Window.flags, FLAG_FULLSCREEN_MODE);

#if !defined(__APPLE__)
        // Make sure to restore render size considering HighDPI scaling
        if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_HIGHDPI))
        {
            RLVector2 scaleDpi = RLGetWindowScaleDPI();
            CORE.Window.screen.width = (unsigned int)(CORE.Window.screen.width*scaleDpi.x);
            CORE.Window.screen.height = (unsigned int)(CORE.Window.screen.height*scaleDpi.y);
        }
#endif

        // WARNING: This function launches FramebufferSizeCallback()
#if defined(_WIN32)
        RLGlfwSetWindowMonitorThreadAware(NULL, CORE.Window.position.x, CORE.Window.position.y,
            CORE.Window.screen.width, CORE.Window.screen.height, GLFW_DONT_CARE);
#else
        glfwSetWindowMonitor(platform.handle, NULL, CORE.Window.position.x, CORE.Window.position.y,
            CORE.Window.screen.width, CORE.Window.screen.height, GLFW_DONT_CARE);
#endif

#if defined(_GLFW_X11) || defined(_GLFW_WAYLAND)
        // NOTE: X11 requires restoring the decorated window after switching from
        // fullscreen to avoid issues with framebuffer scaling
        glfwSetWindowAttrib(platform.handle, GLFW_DECORATED, GLFW_TRUE);
        FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_UNDECORATED);
#endif
    }

    // Try to enable GPU V-Sync, so frames are limited to screen refresh rate (60Hz -> 60 FPS)
    // NOTE: V-Sync can be enabled by graphic driver configuration
    if (FLAG_IS_SET(CORE.Window.flags, FLAG_VSYNC_HINT)) glfwSwapInterval(1);
}

// Toggle borderless windowed mode
void RLToggleBorderlessWindowed(void)
{
    // Leave fullscreen before attempting to set borderless windowed mode
    // NOTE: Fullscreen already saves the previous position so it does not need to be set again later
    if (FLAG_IS_SET(CORE.Window.flags, FLAG_FULLSCREEN_MODE)) RLToggleFullscreen();

    RLGlfwMonitorInfo info = { 0 };
    const int monitorIndex = RLGetCurrentMonitor();
    info.index = monitorIndex;

#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwRunOnEventThread(RLGlfwTask_QueryMonitorInfo, &info, true);
    }
    else
#endif
    {
        int monitorCount = 0;
        GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);
        info.monitorCount = monitorCount;

        if ((monitorIndex >= 0) && (monitorIndex < monitorCount))
        {
            GLFWmonitor *monitor = monitors[monitorIndex];
            const GLFWvidmode *mode = glfwGetVideoMode(monitor);

            info.monitor = monitor;
            info.name = glfwGetMonitorName(monitor);
            glfwGetMonitorPos(monitor, &info.posX, &info.posY);
            glfwGetMonitorWorkarea(monitor, &info.workX, &info.workY, &info.workW, &info.workH);
            glfwGetMonitorPhysicalSize(monitor, &info.physW, &info.physH);

            if (mode != NULL)
            {
                info.modeW = mode->width;
                info.modeH = mode->height;
                info.refresh = mode->refreshRate;
                info.ok = 1;
            }
        }
    }

    if (!info.ok)
    {
        TRACELOG(LOG_WARNING, "GLFW: Failed to find selected monitor");
        return;
    }

    if (!FLAG_IS_SET(CORE.Window.flags, FLAG_BORDERLESS_WINDOWED_MODE))
    {
        // Store screen position and size
        // NOTE: If it was on fullscreen, screen position was already stored, so skip setting it here
        CORE.Window.previousPosition = CORE.Window.position;
        CORE.Window.previousScreen = CORE.Window.screen;

        // Set undecorated flag
#if defined(_WIN32)
        RLGlfwSetWindowAttribThreadAware(GLFW_DECORATED, GLFW_FALSE);
#else
        glfwSetWindowAttrib(platform.handle, GLFW_DECORATED, GLFW_FALSE);
#endif
        FLAG_SET(CORE.Window.flags, FLAG_WINDOW_UNDECORATED);

        // Get monitor position and size
        CORE.Window.position.x = info.posX;
        CORE.Window.position.y = info.posY;
        CORE.Window.screen.width = info.modeW;
        CORE.Window.screen.height = info.modeH;

        // Set screen position and size
#if defined(_WIN32)
        RLGlfwSetWindowMonitorThreadAware(info.monitor, CORE.Window.position.x, CORE.Window.position.y,
            CORE.Window.screen.width, CORE.Window.screen.height, info.refresh);
#else
        glfwSetWindowMonitor(platform.handle, info.monitor, CORE.Window.position.x, CORE.Window.position.y,
            CORE.Window.screen.width, CORE.Window.screen.height, info.refresh);
#endif

        // Refocus window
#if !defined(__APPLE__)
    #if defined(_WIN32)
        RLGlfwFocusWindowThreadAware();
    #else
        glfwFocusWindow(platform.handle);
    #endif
#endif

        FLAG_SET(CORE.Window.flags, FLAG_BORDERLESS_WINDOWED_MODE);
    }
    else
    {
        // Restore previous screen values
        CORE.Window.position = CORE.Window.previousPosition;
        CORE.Window.screen = CORE.Window.previousScreen;

        // Remove undecorated flag
#if defined(_WIN32)
        RLGlfwSetWindowAttribThreadAware(GLFW_DECORATED, GLFW_TRUE);
#else
        glfwSetWindowAttrib(platform.handle, GLFW_DECORATED, GLFW_TRUE);
#endif
        FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_UNDECORATED);

    #if !defined(__APPLE__)
        // Make sure to restore size considering HighDPI scaling
        if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_HIGHDPI))
        {
            RLVector2 scaleDpi = RLGetWindowScaleDPI();
            CORE.Window.screen.width = (unsigned int)(CORE.Window.screen.width*scaleDpi.x);
            CORE.Window.screen.height = (unsigned int)(CORE.Window.screen.height*scaleDpi.y);
        }
    #endif

        // Return to previous screen size and position
#if defined(_WIN32)
        RLGlfwSetWindowMonitorThreadAware(NULL, CORE.Window.position.x, CORE.Window.position.y,
            CORE.Window.screen.width, CORE.Window.screen.height, info.refresh);
#else
        glfwSetWindowMonitor(platform.handle, NULL, CORE.Window.position.x, CORE.Window.position.y,
            CORE.Window.screen.width, CORE.Window.screen.height, info.refresh);
#endif

        // Refocus window
#if !defined(__APPLE__)
    #if defined(_WIN32)
        RLGlfwFocusWindowThreadAware();
    #else
        glfwFocusWindow(platform.handle);
    #endif
#endif

        FLAG_CLEAR(CORE.Window.flags, FLAG_BORDERLESS_WINDOWED_MODE);
    }
}

// Set window state: maximized, if resizable
void RLMaximizeWindow(void)
{
#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_RESIZABLE))
        {
            RLGlfwRunOnEventThread(RLGlfwTask_MaximizeWindow, NULL, true);
            FLAG_SET(CORE.Window.flags, FLAG_WINDOW_MAXIMIZED);
        }
        return;
    }
#endif
    if (glfwGetWindowAttrib(platform.handle, GLFW_RESIZABLE) == GLFW_TRUE)
    {
        glfwMaximizeWindow(platform.handle);
        FLAG_SET(CORE.Window.flags, FLAG_WINDOW_MAXIMIZED);
    }
}

// Set window state: minimized
void RLMinimizeWindow(void)
{
    // NOTE: Following function launches callback that sets appropriate flag!
#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwRunOnEventThread(RLGlfwTask_IconifyWindow, NULL, true);
        return;
    }
#endif
    glfwIconifyWindow(platform.handle);
}

// Restore window from being minimized/maximized
void RLRestoreWindow(void)
{
#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_RESIZABLE))
        {
            RLGlfwRunOnEventThread(RLGlfwTask_RestoreWindow, NULL, true);
            FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_MINIMIZED);
            FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_MAXIMIZED);
        }
        return;
    }
#endif
    if (glfwGetWindowAttrib(platform.handle, GLFW_RESIZABLE) == GLFW_TRUE)
    {
        // Restores the specified window if it was previously iconified (minimized) or maximized
        glfwRestoreWindow(platform.handle);
        FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_MINIMIZED);
        FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_MAXIMIZED);
    }
}

// Set window configuration state using flags
void RLSetWindowState(unsigned int flags)
{
    // NOTE: SetWindowState() is meant to be used after InitWindow().
    // For pre-init configuration, route to SetConfigFlags() to avoid touching platform handles.
    if (!CORE.Window.ready)
    {
        TRACELOG(LOG_WARNING, "WINDOW: SetWindowState called before window initialization, routing to SetConfigFlags");
        RLSetConfigFlags(flags);
        return;
    }

    // Check previous state and requested state to apply required changes
    // NOTE: In most cases the functions already change the flags internally

    // State change: FLAG_VSYNC_HINT
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_VSYNC_HINT) != FLAG_IS_SET(flags, FLAG_VSYNC_HINT)) && FLAG_IS_SET(flags, FLAG_VSYNC_HINT))
    {
        glfwSwapInterval(1);
        FLAG_SET(CORE.Window.flags, FLAG_VSYNC_HINT);
    }

    // State change: FLAG_BORDERLESS_WINDOWED_MODE
    // NOTE: This must be handled before FLAG_FULLSCREEN_MODE because ToggleBorderlessWindowed() needs to get some fullscreen values if fullscreen is running
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_BORDERLESS_WINDOWED_MODE) != FLAG_IS_SET(flags, FLAG_BORDERLESS_WINDOWED_MODE)) && FLAG_IS_SET(flags, FLAG_BORDERLESS_WINDOWED_MODE))
    {
        RLToggleBorderlessWindowed();     // NOTE: Window state flag updated inside function
    }

    // State change: FLAG_FULLSCREEN_MODE
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_FULLSCREEN_MODE) != FLAG_IS_SET(flags, FLAG_FULLSCREEN_MODE)) && FLAG_IS_SET(flags, FLAG_FULLSCREEN_MODE))
    {
        RLToggleFullscreen();     // NOTE: Window state flag updated inside function
    }

    // State change: FLAG_WINDOW_RESIZABLE
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_RESIZABLE) != FLAG_IS_SET(flags, FLAG_WINDOW_RESIZABLE)) && FLAG_IS_SET(flags, FLAG_WINDOW_RESIZABLE))
    {
#if defined(_WIN32)
        RLGlfwSetWindowAttribThreadAware(GLFW_RESIZABLE, GLFW_TRUE);
#else
        glfwSetWindowAttrib(platform.handle, GLFW_RESIZABLE, GLFW_TRUE);
#endif
        FLAG_SET(CORE.Window.flags, FLAG_WINDOW_RESIZABLE);
    }

    // State change: FLAG_WINDOW_UNDECORATED
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_UNDECORATED) != FLAG_IS_SET(flags, FLAG_WINDOW_UNDECORATED)) && FLAG_IS_SET(flags, FLAG_WINDOW_UNDECORATED))
    {
#if defined(_WIN32)
        RLGlfwSetWindowAttribThreadAware(GLFW_DECORATED, GLFW_FALSE);
#else
        glfwSetWindowAttrib(platform.handle, GLFW_DECORATED, GLFW_FALSE);
#endif
        FLAG_SET(CORE.Window.flags, FLAG_WINDOW_UNDECORATED);
    }

    // State change: FLAG_WINDOW_HIDDEN
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_HIDDEN) != FLAG_IS_SET(flags, FLAG_WINDOW_HIDDEN)) && FLAG_IS_SET(flags, FLAG_WINDOW_HIDDEN))
    {
#if defined(_WIN32)
        RLGlfwHideWindowThreadAware();
#else
        glfwHideWindow(platform.handle);
#endif
        FLAG_SET(CORE.Window.flags, FLAG_WINDOW_HIDDEN);
    }

    // State change: FLAG_WINDOW_MINIMIZED
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_MINIMIZED) != FLAG_IS_SET(flags, FLAG_WINDOW_MINIMIZED)) && FLAG_IS_SET(flags, FLAG_WINDOW_MINIMIZED))
    {
        //GLFW_ICONIFIED
        RLMinimizeWindow();       // NOTE: Window state flag updated inside function
    }

    // State change: FLAG_WINDOW_MAXIMIZED
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_MAXIMIZED) != FLAG_IS_SET(flags, FLAG_WINDOW_MAXIMIZED)) && FLAG_IS_SET(flags, FLAG_WINDOW_MAXIMIZED))
    {
        //GLFW_MAXIMIZED
        RLMaximizeWindow();       // NOTE: Window state flag updated inside function
    }

    // State change: FLAG_WINDOW_UNFOCUSED
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_UNFOCUSED) != FLAG_IS_SET(flags, FLAG_WINDOW_UNFOCUSED)) && FLAG_IS_SET(flags, FLAG_WINDOW_UNFOCUSED))
    {
#if defined(_WIN32)
        RLGlfwSetWindowAttribThreadAware(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
#else
        glfwSetWindowAttrib(platform.handle, GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
#endif
        FLAG_SET(CORE.Window.flags, FLAG_WINDOW_UNFOCUSED);
    }

    // State change: FLAG_WINDOW_TOPMOST
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_TOPMOST) != FLAG_IS_SET(flags, FLAG_WINDOW_TOPMOST)) && FLAG_IS_SET(flags, FLAG_WINDOW_TOPMOST))
    {
#if defined(_WIN32)
        RLGlfwSetWindowAttribThreadAware(GLFW_FLOATING, GLFW_TRUE);
#else
        glfwSetWindowAttrib(platform.handle, GLFW_FLOATING, GLFW_TRUE);
#endif
        FLAG_SET(CORE.Window.flags, FLAG_WINDOW_TOPMOST);
    }

    // State change: FLAG_WINDOW_ALWAYS_RUN
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_ALWAYS_RUN) != FLAG_IS_SET(flags, FLAG_WINDOW_ALWAYS_RUN)) && FLAG_IS_SET(flags, FLAG_WINDOW_ALWAYS_RUN))
    {
        FLAG_SET(CORE.Window.flags, FLAG_WINDOW_ALWAYS_RUN);
    }

    
    // State change: FLAG_WINDOW_BROADCAST_WAKE
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_BROADCAST_WAKE) != FLAG_IS_SET(flags, FLAG_WINDOW_BROADCAST_WAKE)) && FLAG_IS_SET(flags, FLAG_WINDOW_BROADCAST_WAKE))
    {
#if defined(_WIN32)
        platform.broadcastWake = true;
#endif
        FLAG_SET(CORE.Window.flags, FLAG_WINDOW_BROADCAST_WAKE);
    }

    // State change: FLAG_WINDOW_REFRESH_CALLBACK
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_REFRESH_CALLBACK) != FLAG_IS_SET(flags, FLAG_WINDOW_REFRESH_CALLBACK)) && FLAG_IS_SET(flags, FLAG_WINDOW_REFRESH_CALLBACK))
    {
#if defined(_WIN32)
        RLGlfwSetWindowRefreshCallbackThreadAware(1);
#else
        glfwSetWindowRefreshCallback(platform.handle, WindowRefreshCallback);
#endif
        FLAG_SET(CORE.Window.flags, FLAG_WINDOW_REFRESH_CALLBACK);
    }

    // State change: FLAG_WINDOW_SNAP_LAYOUT
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_SNAP_LAYOUT) != FLAG_IS_SET(flags, FLAG_WINDOW_SNAP_LAYOUT)) && FLAG_IS_SET(flags, FLAG_WINDOW_SNAP_LAYOUT))
    {
#if defined(_WIN32)
        RLGlfwSetWindowAttribThreadAware(GLFW_WIN32_SNAP_LAYOUT, GLFW_TRUE);
#endif
        FLAG_SET(CORE.Window.flags, FLAG_WINDOW_SNAP_LAYOUT);
    }

    /* RL_DYNAMIC_SET_WIN32_FLAGS */

    // The following states can not be changed after window creation

    // State change: FLAG_WINDOW_TRANSPARENT
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_TRANSPARENT) != FLAG_IS_SET(flags, FLAG_WINDOW_TRANSPARENT)) && FLAG_IS_SET(flags, FLAG_WINDOW_TRANSPARENT))
    {
        TRACELOG(LOG_WARNING, "WINDOW: Framebuffer transparency can only be configured before window initialization");
    }

    // State change: FLAG_WINDOW_HIGHDPI
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_HIGHDPI) != FLAG_IS_SET(flags, FLAG_WINDOW_HIGHDPI)) && FLAG_IS_SET(flags, FLAG_WINDOW_HIGHDPI))
    {
        TRACELOG(LOG_WARNING, "WINDOW: High DPI can only be configured before window initialization");
    }

    // State change: FLAG_WINDOW_MOUSE_PASSTHROUGH
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_MOUSE_PASSTHROUGH) != FLAG_IS_SET(flags, FLAG_WINDOW_MOUSE_PASSTHROUGH)) && FLAG_IS_SET(flags, FLAG_WINDOW_MOUSE_PASSTHROUGH))
    {
#if defined(_WIN32)
        RLGlfwSetWindowAttribThreadAware(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
#else
        glfwSetWindowAttrib(platform.handle, GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
#endif
        FLAG_SET(CORE.Window.flags, FLAG_WINDOW_MOUSE_PASSTHROUGH);
    }

    // State change: FLAG_MSAA_4X_HINT
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_MSAA_4X_HINT) != FLAG_IS_SET(flags, FLAG_MSAA_4X_HINT)) && FLAG_IS_SET(flags, FLAG_MSAA_4X_HINT))
    {
        TRACELOG(LOG_WARNING, "WINDOW: MSAA can only be configured before window initialization");
    }

    // State change: FLAG_INTERLACED_HINT
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_INTERLACED_HINT) != FLAG_IS_SET(flags, FLAG_INTERLACED_HINT)) && FLAG_IS_SET(flags, FLAG_INTERLACED_HINT))
    {
        TRACELOG(LOG_WARNING, "WINDOW: Interlaced mode can only be configured before window initialization");
    }
}

// Clear window configuration state flags
void RLClearWindowState(unsigned int flags)
{
    // NOTE: ClearWindowState() is meant to be used after InitWindow().
    // If called pre-init, just clear the pending config flags and return.
    if (!CORE.Window.ready)
    {
        TRACELOG(LOG_WARNING, "WINDOW: ClearWindowState called before window initialization, clearing pending config flags");
        FLAG_CLEAR(CORE.Window.flags, flags);
        return;
    }

    // Check previous state and requested state to apply required changes
    // NOTE: In most cases the functions already change the flags internally

    // State change: FLAG_VSYNC_HINT
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_VSYNC_HINT)) && (FLAG_IS_SET(flags, FLAG_VSYNC_HINT)))
    {
        glfwSwapInterval(0);
        FLAG_CLEAR(CORE.Window.flags, FLAG_VSYNC_HINT);
    }

    // State change: FLAG_BORDERLESS_WINDOWED_MODE
    // NOTE: This must be handled before FLAG_FULLSCREEN_MODE because ToggleBorderlessWindowed() needs to get some fullscreen values if fullscreen is running
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_BORDERLESS_WINDOWED_MODE)) && (FLAG_IS_SET(flags, FLAG_BORDERLESS_WINDOWED_MODE)))
    {
        RLToggleBorderlessWindowed(); // NOTE: Window state flag updated inside function
    }

    // State change: FLAG_FULLSCREEN_MODE
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_FULLSCREEN_MODE)) && (FLAG_IS_SET(flags, FLAG_FULLSCREEN_MODE)))
    {
        RLToggleFullscreen(); // NOTE: Window state flag updated inside function
    }

    // State change: FLAG_WINDOW_RESIZABLE
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_RESIZABLE)) && (FLAG_IS_SET(flags, FLAG_WINDOW_RESIZABLE)))
    {
        #if defined(_WIN32)
        RLGlfwSetWindowAttribThreadAware(GLFW_RESIZABLE, GLFW_FALSE);
        #else
        glfwSetWindowAttrib(platform.handle, GLFW_RESIZABLE, GLFW_FALSE);
        #endif
        FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_RESIZABLE);
    }

    // State change: FLAG_WINDOW_HIDDEN
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_HIDDEN)) && (FLAG_IS_SET(flags, FLAG_WINDOW_HIDDEN)))
    {
        #if defined(_WIN32)
        RLGlfwShowWindowThreadAware();
        #else
        glfwShowWindow(platform.handle);
        #endif
        FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_HIDDEN);
    }

    // State change: FLAG_WINDOW_MINIMIZED
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_MINIMIZED)) && (FLAG_IS_SET(flags, FLAG_WINDOW_MINIMIZED)))
    {
        RLRestoreWindow();       // NOTE: Window state flag updated inside function
    }

    // State change: FLAG_WINDOW_MAXIMIZED
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_MAXIMIZED)) && (FLAG_IS_SET(flags, FLAG_WINDOW_MAXIMIZED)))
    {
        RLRestoreWindow();       // NOTE: Window state flag updated inside function
    }

    // State change: FLAG_WINDOW_UNDECORATED
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_UNDECORATED)) && (FLAG_IS_SET(flags, FLAG_WINDOW_UNDECORATED)))
    {
        #if defined(_WIN32)
        RLGlfwSetWindowAttribThreadAware(GLFW_DECORATED, GLFW_TRUE);
        #else
        glfwSetWindowAttrib(platform.handle, GLFW_DECORATED, GLFW_TRUE);
        #endif
        FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_UNDECORATED);
    }

    // State change: FLAG_WINDOW_UNFOCUSED
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_UNFOCUSED)) && (FLAG_IS_SET(flags, FLAG_WINDOW_UNFOCUSED)))
    {
        #if defined(_WIN32)
        RLGlfwSetWindowAttribThreadAware(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);
        #else
        glfwSetWindowAttrib(platform.handle, GLFW_FOCUS_ON_SHOW, GLFW_TRUE);
        #endif
        FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_UNFOCUSED);
    }

    // State change: FLAG_WINDOW_TOPMOST
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_TOPMOST)) && (FLAG_IS_SET(flags, FLAG_WINDOW_TOPMOST)))
    {
        #if defined(_WIN32)
        RLGlfwSetWindowAttribThreadAware(GLFW_FLOATING, GLFW_FALSE);
        #else
        glfwSetWindowAttrib(platform.handle, GLFW_FLOATING, GLFW_FALSE);
        #endif
        FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_TOPMOST);
    }

    // State change: FLAG_WINDOW_ALWAYS_RUN
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_ALWAYS_RUN)) && (FLAG_IS_SET(flags, FLAG_WINDOW_ALWAYS_RUN)))
    {
        FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_ALWAYS_RUN);
    }

    
    // State change: FLAG_WINDOW_BROADCAST_WAKE
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_BROADCAST_WAKE)) && (FLAG_IS_SET(flags, FLAG_WINDOW_BROADCAST_WAKE)))
    {
#if defined(_WIN32)
        platform.broadcastWake = false;
#endif
        FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_BROADCAST_WAKE);
    }

    // State change: FLAG_WINDOW_REFRESH_CALLBACK
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_REFRESH_CALLBACK)) && (FLAG_IS_SET(flags, FLAG_WINDOW_REFRESH_CALLBACK)))
    {
#if defined(_WIN32)
        RLGlfwSetWindowRefreshCallbackThreadAware(0);
#else
        glfwSetWindowRefreshCallback(platform.handle, NULL);
#endif
        FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_REFRESH_CALLBACK);
    }

    // State change: FLAG_WINDOW_SNAP_LAYOUT
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_SNAP_LAYOUT)) && (FLAG_IS_SET(flags, FLAG_WINDOW_SNAP_LAYOUT)))
    {
#if defined(_WIN32)
        RLGlfwSetWindowAttribThreadAware(GLFW_WIN32_SNAP_LAYOUT, GLFW_FALSE);
#endif
        FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_SNAP_LAYOUT);
    }

    /* RL_DYNAMIC_CLEAR_WIN32_FLAGS */

    // The following states can not be changed after window creation

    // State change: FLAG_WINDOW_TRANSPARENT
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_TRANSPARENT)) && (FLAG_IS_SET(flags, FLAG_WINDOW_TRANSPARENT)))
    {
        TRACELOG(LOG_WARNING, "WINDOW: Framebuffer transparency can only be configured before window initialization");
    }

    // State change: FLAG_WINDOW_HIGHDPI
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_HIGHDPI)) && (FLAG_IS_SET(flags, FLAG_WINDOW_HIGHDPI)))
    {
        TRACELOG(LOG_WARNING, "WINDOW: High DPI can only be configured before window initialization");
    }

    // State change: FLAG_WINDOW_MOUSE_PASSTHROUGH
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_MOUSE_PASSTHROUGH)) && (FLAG_IS_SET(flags, FLAG_WINDOW_MOUSE_PASSTHROUGH)))
    {
#if defined(_WIN32)
        RLGlfwSetWindowAttribThreadAware(GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);
#else
        glfwSetWindowAttrib(platform.handle, GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);
#endif
        FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_MOUSE_PASSTHROUGH);
    }

    // State change: FLAG_MSAA_4X_HINT
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_MSAA_4X_HINT)) && (FLAG_IS_SET(flags, FLAG_MSAA_4X_HINT)))
    {
        TRACELOG(LOG_WARNING, "WINDOW: MSAA can only be configured before window initialization");
    }

    // State change: FLAG_INTERLACED_HINT
    if ((FLAG_IS_SET(CORE.Window.flags, FLAG_INTERLACED_HINT)) && (FLAG_IS_SET(flags, FLAG_INTERLACED_HINT)))
    {
        TRACELOG(LOG_WARNING, "RPI: Interlaced mode can only be configured before window initialization");
    }
}

// Set icon for window
// NOTE 1: Image must be in RGBA format, 8bit per channel
// NOTE 2: Image is scaled by the OS for all required sizes
void RLSetWindowIcon(RLImage image)
{
    if (image.data == NULL)
    {
        // Revert to the default window icon, pass in an empty image array
#if defined(_WIN32)
        if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
        {
            RLGlfwIconTask t = { 0, NULL };
            RLGlfwRunOnEventThread(RLGlfwTask_SetWindowIcon, &t, true);
            return;
        }
#endif
        glfwSetWindowIcon(platform.handle, 0, NULL);
    }
    else
    {
        if (image.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8)
        {
            GLFWimage icon[1] = { 0 };

            icon[0].width = image.width;
            icon[0].height = image.height;
            icon[0].pixels = (unsigned char *)image.data;

            // NOTE 1: Only one image icon supported
            // NOTE 2: The specified image data is copied before this function returns
#if defined(_WIN32)
            if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
            {
                RLGlfwIconTask t = { 1, icon };
                RLGlfwRunOnEventThread(RLGlfwTask_SetWindowIcon, &t, true);
                return;
            }
#endif
            glfwSetWindowIcon(platform.handle, 1, icon);
        }
        else TRACELOG(LOG_WARNING, "GLFW: Window icon image must be in R8G8B8A8 pixel format");
    }
}

// Set icon for window, multiple images
// NOTE 1: Images must be in RGBA format, 8bit per channel
// NOTE 2: The multiple images are used depending on provided sizes
// Standard Windows icon sizes: 256, 128, 96, 64, 48, 32, 24, 16
void RLSetWindowIcons(RLImage *images, int count)
{
    if ((images == NULL) || (count <= 0))
    {
        // Revert to the default window icon, pass in an empty image array
#if defined(_WIN32)
        if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
        {
            RLGlfwIconTask t = { 0, NULL };
            RLGlfwRunOnEventThread(RLGlfwTask_SetWindowIcon, &t, true);
            return;
        }
#endif
        glfwSetWindowIcon(platform.handle, 0, NULL);
    }
    else
    {
        int valid = 0;
        GLFWimage *icons = (GLFWimage *)RL_CALLOC(count, sizeof(GLFWimage));

        for (int i = 0; i < count; i++)
        {
            if (images[i].format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8)
            {
                icons[valid].width = images[i].width;
                icons[valid].height = images[i].height;
                icons[valid].pixels = (unsigned char *)images[i].data;

                valid++;
            }
            else TRACELOG(LOG_WARNING, "GLFW: Window icon image must be in R8G8B8A8 pixel format");
        }
        // NOTE: Images data is copied internally before this function returns
#if defined(_WIN32)
        if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
        {
            RLGlfwIconTask t = { valid, icons };
            RLGlfwRunOnEventThread(RLGlfwTask_SetWindowIcon, &t, true);
        }
        else
#endif
        glfwSetWindowIcon(platform.handle, valid, icons);

        RL_FREE(icons);
    }
}

// Set title for window
void RLSetWindowTitle(const char *title)
{
    CORE.Window.title = title;
#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        // NOTE: title pointer is expected to be valid for the duration of this synchronous call.
        RLGlfwRunOnEventThread(RLGlfwTask_SetWindowTitle, (void *)title, true);
        return;
    }
#endif

    glfwSetWindowTitle(platform.handle, title);
}

// Set window position on screen (windowed mode)
void RLSetWindowPosition(int x, int y)
{
    // Update CORE.Window.position as well
    CORE.Window.position.x = x;
    CORE.Window.position.y = y;
#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        int xy[2] = { x, y };
        RLGlfwRunOnEventThread(RLGlfwTask_SetWindowPos, xy, true);
        return;
    }
#endif
    glfwSetWindowPos(platform.handle, x, y);
}

// Set monitor for the current window
void RLSetWindowMonitor(int monitor)
{
    int monitorCount = 0;

#if defined(_WIN32)
    // In event-thread mode, all GLFW monitor/window queries must run on the Win32 message thread.
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwMonitorInfo info = { 0 };
        info.index = monitor;
        RLGlfwRunOnEventThread(RLGlfwTask_QueryMonitorInfo, &info, true);
        monitorCount = info.monitorCount;

        if (!info.ok)
        {
            TRACELOG(LOG_WARNING, "GLFW: Failed to find selected monitor");
            return;
        }

        if (FLAG_IS_SET(CORE.Window.flags, FLAG_FULLSCREEN_MODE))
        {
            TRACELOG(LOG_INFO, "GLFW: Selected fullscreen monitor: [%i] %s", monitor, info.name);
            RLGlfwSetWindowMonitorThreadAware(info.monitor, 0, 0, info.modeW, info.modeH, info.refresh);
        }
        else
        {
            TRACELOG(LOG_INFO, "GLFW: Selected monitor: [%i] %s", monitor, info.name);

            // Here the render width has to be used again in case high dpi flag is enabled.
            const int screenWidth = CORE.Window.render.width;
            const int screenHeight = CORE.Window.render.height;
            const int monitorWorkareaX = info.workX;
            const int monitorWorkareaY = info.workY;
            const int monitorWorkareaWidth = info.workW;
            const int monitorWorkareaHeight = info.workH;

            if ((screenWidth >= monitorWorkareaWidth) || (screenHeight >= monitorWorkareaHeight))
            {
                int xy[2] = { monitorWorkareaX, monitorWorkareaY };
                RLGlfwRunOnEventThread(RLGlfwTask_SetWindowPos, xy, true);
            }
            else
            {
                const int x = monitorWorkareaX + (monitorWorkareaWidth/2) - (screenWidth/2);
                const int y = monitorWorkareaY + (monitorWorkareaHeight/2) - (screenHeight/2);
                int xy[2] = { x, y };
                RLGlfwRunOnEventThread(RLGlfwTask_SetWindowPos, xy, true);
            }
        }
        return;
    }
#endif

    GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

    if ((monitor >= 0) && (monitor < monitorCount))
    {
        if (FLAG_IS_SET(CORE.Window.flags, FLAG_FULLSCREEN_MODE))
        {
            TRACELOG(LOG_INFO, "GLFW: Selected fullscreen monitor: [%i] %s", monitor, glfwGetMonitorName(monitors[monitor]));

            const GLFWvidmode *mode = glfwGetVideoMode(monitors[monitor]);
            if (mode != NULL) glfwSetWindowMonitor(platform.handle, monitors[monitor], 0, 0, mode->width, mode->height, mode->refreshRate);
        }
        else
        {
            TRACELOG(LOG_INFO, "GLFW: Selected monitor: [%i] %s", monitor, glfwGetMonitorName(monitors[monitor]));

            // Here the render width has to be used again in case high dpi flag is enabled
            const int screenWidth = CORE.Window.render.width;
            const int screenHeight = CORE.Window.render.height;
            int monitorWorkareaX = 0;
            int monitorWorkareaY = 0;
            int monitorWorkareaWidth = 0;
            int monitorWorkareaHeight = 0;
            glfwGetMonitorWorkarea(monitors[monitor], &monitorWorkareaX, &monitorWorkareaY, &monitorWorkareaWidth, &monitorWorkareaHeight);

            // If the screen size is larger than the monitor workarea, anchor it on the top left corner, otherwise, center it
            if ((screenWidth >= monitorWorkareaWidth) || (screenHeight >= monitorWorkareaHeight))
            {
                glfwSetWindowPos(platform.handle, monitorWorkareaX, monitorWorkareaY);
            }
            else
            {
                const int x = monitorWorkareaX + (monitorWorkareaWidth/2) - (screenWidth/2);
                const int y = monitorWorkareaY + (monitorWorkareaHeight/2) - (screenHeight/2);
                glfwSetWindowPos(platform.handle, x, y);
            }
        }
    }
    else TRACELOG(LOG_WARNING, "GLFW: Failed to find selected monitor");
}

// Set window minimum dimensions (FLAG_WINDOW_RESIZABLE)
void RLSetWindowMinSize(int width, int height)
{
    CORE.Window.screenMin.width = width;
    CORE.Window.screenMin.height = height;

    int minWidth  = (CORE.Window.screenMin.width  == 0)? GLFW_DONT_CARE : (int)CORE.Window.screenMin.width;
    int minHeight = (CORE.Window.screenMin.height == 0)? GLFW_DONT_CARE : (int)CORE.Window.screenMin.height;
    int maxWidth  = (CORE.Window.screenMax.width  == 0)? GLFW_DONT_CARE : (int)CORE.Window.screenMax.width;
    int maxHeight = (CORE.Window.screenMax.height == 0)? GLFW_DONT_CARE : (int)CORE.Window.screenMax.height;

#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        int lim[4] = { minWidth, minHeight, maxWidth, maxHeight };
        RLGlfwRunOnEventThread(RLGlfwTask_SetWindowSizeLimits, lim, true);
        return;
    }
#endif
    glfwSetWindowSizeLimits(platform.handle, minWidth, minHeight, maxWidth, maxHeight);
}

// Set window maximum dimensions (FLAG_WINDOW_RESIZABLE)
void RLSetWindowMaxSize(int width, int height)
{
    CORE.Window.screenMax.width = width;
    CORE.Window.screenMax.height = height;

    int minWidth  = (CORE.Window.screenMin.width  == 0)? GLFW_DONT_CARE : (int)CORE.Window.screenMin.width;
    int minHeight = (CORE.Window.screenMin.height == 0)? GLFW_DONT_CARE : (int)CORE.Window.screenMin.height;
    int maxWidth  = (CORE.Window.screenMax.width  == 0)? GLFW_DONT_CARE : (int)CORE.Window.screenMax.width;
    int maxHeight = (CORE.Window.screenMax.height == 0)? GLFW_DONT_CARE : (int)CORE.Window.screenMax.height;

#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        int lim[4] = { minWidth, minHeight, maxWidth, maxHeight };
        RLGlfwRunOnEventThread(RLGlfwTask_SetWindowSizeLimits, lim, true);
        return;
    }
#endif
    glfwSetWindowSizeLimits(platform.handle, minWidth, minHeight, maxWidth, maxHeight);
}

// Set window dimensions
void RLSetWindowSize(int width, int height)
{
    CORE.Window.screen.width = width;
    CORE.Window.screen.height = height;

#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        int wh[2] = { width, height };
        RLGlfwRunOnEventThread(RLGlfwTask_SetWindowSize, wh, true);
        return;
    }
#endif
    glfwSetWindowSize(platform.handle, width, height);
}

// Set window opacity, value opacity is between 0.0 and 1.0
void RLSetWindowOpacity(float opacity)
{
    if (opacity >= 1.0f) opacity = 1.0f;
    else if (opacity <= 0.0f) opacity = 0.0f;

#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        float op = opacity;
        RLGlfwRunOnEventThread(RLGlfwTask_SetWindowOpacity, &op, true);
        return;
    }
#endif
    glfwSetWindowOpacity(platform.handle, opacity);
}

// Set window focused
void RLSetWindowFocused(void)
{
#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwRunOnEventThread(RLGlfwTask_FocusWindow, NULL, true);
        return;
    }
#endif
    glfwFocusWindow(platform.handle);
}

#if defined(__linux__) && defined(_GLFW_X11)
// Local storage for the window handle returned by glfwGetX11Window
// This is needed as X11 handles are integers and may not fit inside a pointer depending on platform
// Storing the handle locally and returning a pointer in GetWindowHandle allows the code to work regardless of pointer width
static XID X11WindowHandle;
#endif
// Get native window handle
void *RLGetWindowHandle(void)
{
#if defined(_WIN32)
    // NOTE: Returned handle is: void *HWND (windows.h)
    return glfwGetWin32Window(platform.handle);
#endif
#if defined(__linux__)
    #if defined(_GLFW_WAYLAND)
        #if defined(_GLFW_X11)
            int platformID = glfwGetPlatform();
            if (platformID == GLFW_PLATFORM_WAYLAND)
            {
                return glfwGetWaylandWindow(platform.handle);
            }
            else
            {
                X11WindowHandle = glfwGetX11Window(platform.handle);
                return &X11WindowHandle;
            }
        #else
            return glfwGetWaylandWindow(platform.handle);
        #endif
    #elif defined(_GLFW_X11)
        // Store the window handle localy and return a pointer to the variable instead
        // Reasoning detailed in the declaration of X11WindowHandle
        X11WindowHandle = glfwGetX11Window(platform.handle);
        return &X11WindowHandle;
    #endif
#endif
#if defined(__APPLE__)
    // NOTE: Returned handle is: (objc_object *)
    return (void *)glfwGetCocoaWindow(platform.handle);
#endif

    return NULL;
}

#if defined(_WIN32)

//----------------------------------------------------------------------------------
// Win32 helpers (property bag + message hooks) - raylib wrappers

#define RL_WIN32_DISPATCH_MSG_NAME L"GLFW_RAYLIB_DISPATCH_V1_{3A2C1E22-6B43-4E67-A8F2-5E2D1E04F9A8}"

// Minimal Win32 message dispatch surface (no windows.h)
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef intptr_t LRESULT;
typedef unsigned long DWORD;

__declspec(dllimport) DWORD __stdcall GetCurrentThreadId(void);
__declspec(dllimport) DWORD __stdcall GetWindowThreadProcessId(HWND hWnd, DWORD* lpdwProcessId);
__declspec(dllimport) int __stdcall PostMessageW(HWND hWnd, unsigned int Msg, WPARAM wParam, LPARAM lParam);

__declspec(dllimport) unsigned int __stdcall RegisterWindowMessageW(const wchar_t* lpString);
__declspec(dllimport) LRESULT __stdcall SendMessageW(HWND hWnd, unsigned int Msg, WPARAM wParam, LPARAM lParam);

typedef LRESULT (*RLWin32DispatchFn)(GLFWwindow* window, HWND hWnd, void* user);

static unsigned int RLWin32GetDispatchMessageId(void)
{
    static unsigned int msg = 0;
    if (msg == 0) msg = RegisterWindowMessageW(RL_WIN32_DISPATCH_MSG_NAME);
    return msg;
}

static LRESULT RLWin32DispatchToHwnd(HWND hWnd, RLWin32DispatchFn fnDispatch, void* user)
{
    if (hWnd == NULL || fnDispatch == NULL) return 0;
    const unsigned int msg = RLWin32GetDispatchMessageId();
    if (msg == 0) return 0;
    return SendMessageW(hWnd, msg, (WPARAM) fnDispatch, (LPARAM) user);
}

static PlatformData* RLWin32FindPlatformByHwnd(HWND hwnd)
{
    if (hwnd == NULL) return NULL;
    PlatformData* out = NULL;
    RLGlfwGlobalLock();
    for (RLGlfwPlatformNode* it = gRlGlfwPdHead; it; it = it->next)
    {
        PlatformData* pd = it->pd;
        if (pd && pd->handle && pd->win32Hwnd == hwnd) { out = pd; break; }
    }
    RLGlfwGlobalUnlock();
    return out;
}

static int RLWin32IsKnownWindowHandle_Internal(HWND hwnd)
{
    return (RLWin32FindPlatformByHwnd(hwnd) != NULL);
}

// --- Dispatch handlers (run on the HWND owner thread) ---
typedef struct { const char* name; void* value; int ok; } RLWin32PropSetCall;
typedef struct { const char* name; void* out; } RLWin32PropGetCall;

static LRESULT RLWin32Dispatch_SetProp(GLFWwindow* window, HWND hWnd, void* user)
{
    (void)hWnd;
    RLWin32PropSetCall* callData = (RLWin32PropSetCall*)user;
    if (!callData) return 0;
    callData->ok = glfwWin32SetWindowProp(window, callData->name, callData->value);
    return (LRESULT)callData->ok;
}

static LRESULT RLWin32Dispatch_GetProp(GLFWwindow* window, HWND hWnd, void* user)
{
    (void)hWnd;
    RLWin32PropGetCall* callData = (RLWin32PropGetCall*)user;
    if (!callData) return 0;
    callData->out = glfwWin32GetWindowProp(window, callData->name);
    return (LRESULT)(uintptr_t)callData->out;
}

static LRESULT RLWin32Dispatch_RemoveProp(GLFWwindow* window, HWND hWnd, void* user)
{
    (void)hWnd;
    RLWin32PropGetCall* callData = (RLWin32PropGetCall*)user;
    if (!callData) return 0;
    callData->out = glfwWin32RemoveWindowProp(window, callData->name);
    return (LRESULT)(uintptr_t)callData->out;
}

typedef struct {
    RLWin32MessageHook hook;
    void* user;
    void* glfwToken;
    HWND hwnd;
} RLWin32HookWrapper;

static int RLWin32HookAdapter(GLFWwindow* window, HWND hWnd, unsigned int uMsg, uintptr_t wParam, intptr_t lParam, intptr_t* result, void* user)
{
    (void)window;
    RLWin32HookWrapper* wrapper = (RLWin32HookWrapper*)user;
    if (!wrapper || !wrapper->hook) return 0;
    return wrapper->hook((void*)hWnd, uMsg, wParam, lParam, result, wrapper->user);
}

typedef struct { RLWin32HookWrapper* wrapper; void* outToken; } RLWin32HookAddCall;
static LRESULT RLWin32Dispatch_AddHook(GLFWwindow* window, HWND hWnd, void* user)
{
    RLWin32HookAddCall* callData = (RLWin32HookAddCall*)user;
    if (!callData || !callData->wrapper) return 0;
    callData->wrapper->hwnd = hWnd;
    callData->outToken = glfwWin32AddMessageHook(window, RLWin32HookAdapter, callData->wrapper);
    callData->wrapper->glfwToken = callData->outToken;
    return (LRESULT)(uintptr_t)callData->outToken;
}

static LRESULT RLWin32Dispatch_RemoveHook(GLFWwindow* window, HWND hWnd, void* user)
{
    (void)hWnd;
    RLWin32HookWrapper* wrapper = (RLWin32HookWrapper*)user;
    if (!wrapper || !wrapper->glfwToken) return 0;
    const int isRemoveOk = glfwWin32RemoveMessageHook(window, wrapper->glfwToken);
    if (isRemoveOk) wrapper->glfwToken = NULL;
    return (LRESULT)isRemoveOk;
}
//----------------------------------------------------------------------------------

typedef struct RLWin32PropTask
{
    const char* name;
    void* value;
    void* out;
    int ok;
} RLWin32PropTask;

static void RLGlfwTask_Win32SetWindowProp(void* user)
{
    RLWin32PropTask* t = (RLWin32PropTask*) user;
    t->ok = glfwWin32SetWindowProp(platform.handle, t->name, t->value);
}

static void RLGlfwTask_Win32GetWindowProp(void* user)
{
    RLWin32PropTask* t = (RLWin32PropTask*) user;
    t->out = glfwWin32GetWindowProp(platform.handle, t->name);
}

static void RLGlfwTask_Win32RemoveWindowProp(void* user)
{
    RLWin32PropTask* t = (RLWin32PropTask*) user;
    t->out = glfwWin32RemoveWindowProp(platform.handle, t->name);
}

typedef struct RLWin32HookToken
{
    void* glfwToken;
    RLWin32MessageHook hook;
    void* user;
} RLWin32HookToken;

static int RLWin32MessageHookTrampoline(GLFWwindow* window,
                                       HWND hWnd, unsigned int uMsg, uintptr_t wParam, intptr_t lParam,
                                       intptr_t* result, void* user)
{
    (void) window;
    RLWin32HookToken* tok = (RLWin32HookToken*) user;
    if (!tok || !tok->hook) return 0;
    return tok->hook((void*) hWnd, uMsg, wParam, lParam, result, tok->user) ? 1 : 0;
}

typedef struct RLWin32HookTask
{
    RLWin32HookToken* tok;
    int ok;
} RLWin32HookTask;

static void RLGlfwTask_Win32AddMessageHook(void* user)
{
    RLWin32HookTask* t = (RLWin32HookTask*) user;
    if (!t || !t->tok) { t->ok = 0; return; }
    t->tok->glfwToken = glfwWin32AddMessageHook(platform.handle, RLWin32MessageHookTrampoline, t->tok);
    t->ok = (t->tok->glfwToken != NULL);
}

static void RLGlfwTask_Win32RemoveMessageHook(void* user)
{
    RLWin32HookTask* t = (RLWin32HookTask*) user;
    if (!t || !t->tok || !t->tok->glfwToken) { t->ok = 0; return; }
    t->ok = glfwWin32RemoveMessageHook(platform.handle, t->tok->glfwToken);
}

int RLWin32SetWindowProp(const char* name, void* value)
{
    RLWin32PropTask t = { name, value, NULL, 0 };
    RLGlfwRunOnEventThread(RLGlfwTask_Win32SetWindowProp, &t, true);
    return t.ok;
}

void* RLWin32GetWindowProp(const char* name)
{
    RLWin32PropTask t = { name, NULL, NULL, 0 };
    RLGlfwRunOnEventThread(RLGlfwTask_Win32GetWindowProp, &t, true);
    return t.out;
}

void* RLWin32RemoveWindowProp(const char* name)
{
    RLWin32PropTask t = { name, NULL, NULL, 0 };
    RLGlfwRunOnEventThread(RLGlfwTask_Win32RemoveWindowProp, &t, true);
    return t.out;
}

void* RLWin32AddMessageHook(RLWin32MessageHook hook, void* user)
{
    if (!hook) return NULL;

    RLWin32HookToken* tok = (RLWin32HookToken*) RL_CALLOC(1, sizeof(RLWin32HookToken));
    if (!tok) return NULL;

    tok->hook = hook;
    tok->user = user;

    RLWin32HookTask task = { tok, 0 };
    RLGlfwRunOnEventThread(RLGlfwTask_Win32AddMessageHook, &task, true);

    if (!task.ok)
    {
        RL_FREE(tok);
        return NULL;
    }

    return (void*) tok;
}

int RLWin32RemoveMessageHook(void* token)
{
    RLWin32HookToken* tok = (RLWin32HookToken*) token;
    if (!tok) return 0;

    RLWin32HookTask task = { tok, 0 };
    RLGlfwRunOnEventThread(RLGlfwTask_Win32RemoveMessageHook, &task, true);

	// Only free the token if removal succeeded; otherwise it may still be referenced
	// by the underlying GLFW hook trampoline.
	if (task.ok) RL_FREE(tok);
    return task.ok;
}

// ------------------------------------------------------------
// Global window management + cross-thread helpers (HWND based)
// ------------------------------------------------------------

int RLWin32GetAllWindowHandles(void** outHwnds, int maxCount)
{
    int count = 0;
    RLGlfwGlobalLock();
    for (RLGlfwPlatformNode* it = gRlGlfwPdHead; it; it = it->next)
    {
        PlatformData* pd = it->pd;
        if (!pd || !pd->handle || !pd->win32Hwnd) continue;
        if (outHwnds && maxCount > 0 && count < maxCount) outHwnds[count] = (void*)pd->win32Hwnd;
        count++;
    }
    RLGlfwGlobalUnlock();
    return count;
}

void* RLWin32GetPrimaryWindowHandle(void)
{
    void* out = NULL;
    RLGlfwGlobalLock();
    // Primary is tracked by GLFWwindow* (first created window in the process).
    // Resolve to cached HWND via the global platform list to avoid calling into GLFW under unknown thread state.
    if (gRlGlfwPrimaryWindow)
    {
        for (RLGlfwPlatformNode* it = gRlGlfwPdHead; it; it = it->next)
        {
            PlatformData* pd = it->pd;
            if (pd && pd->handle == gRlGlfwPrimaryWindow && pd->win32Hwnd)
            {
                out = (void*)pd->win32Hwnd;
                break;
            }
        }
    }
    RLGlfwGlobalUnlock();
    return out;
}

int RLWin32IsKnownWindowHandle(void* hwnd)
{
    return RLWin32IsKnownWindowHandle_Internal((HWND)hwnd);
}

int RLWin32SetWindowPropByHandle(void* hwnd, const char* name, void* value)
{
    HWND hNativeWindowHandle = (HWND)hwnd;
    if (hNativeWindowHandle == NULL || name == NULL) return 0;
    if (!RLWin32IsKnownWindowHandle_Internal(hNativeWindowHandle)) return 0;
    RLWin32PropSetCall callData = { name, value, 0 };
    (void)RLWin32DispatchToHwnd(hNativeWindowHandle, RLWin32Dispatch_SetProp, &callData);
    return callData.ok;
}

void* RLWin32GetWindowPropByHandle(void* hwnd, const char* name)
{
    HWND hNativeWindowHandle = (HWND)hwnd;
    if (hNativeWindowHandle == NULL || name == NULL) return NULL;
    if (!RLWin32IsKnownWindowHandle_Internal(hNativeWindowHandle)) return NULL;
    RLWin32PropGetCall callData = { name, NULL };
    (void)RLWin32DispatchToHwnd(hNativeWindowHandle, RLWin32Dispatch_GetProp, &callData);
    return callData.out;
}

void* RLWin32RemoveWindowPropByHandle(void* hwnd, const char* name)
{
    HWND hNativeWindowHandle = (HWND)hwnd;
    if (hNativeWindowHandle == NULL || name == NULL) return NULL;
    if (!RLWin32IsKnownWindowHandle_Internal(hNativeWindowHandle)) return NULL;
    RLWin32PropGetCall callData = { name, NULL };
    (void)RLWin32DispatchToHwnd(hNativeWindowHandle, RLWin32Dispatch_RemoveProp, &callData);
    return callData.out;
}

void* RLWin32AddMessageHookByHandle(void* hwnd, RLWin32MessageHook hook, void* user)
{
    HWND hNativeWindowHandle = (HWND)hwnd;
    if (hNativeWindowHandle == NULL || hook == NULL) return NULL;
    if (!RLWin32IsKnownWindowHandle_Internal(hNativeWindowHandle)) return NULL;

    RLWin32HookWrapper* wrapper = (RLWin32HookWrapper*)RL_CALLOC(1, sizeof(RLWin32HookWrapper));
    if (!wrapper) return NULL;
    wrapper->hook = hook;
    wrapper->user = user;

    RLWin32HookAddCall call = { 0 };
    call.wrapper = wrapper;
    (void)RLWin32DispatchToHwnd(hNativeWindowHandle, RLWin32Dispatch_AddHook, &call);

    if (call.outToken == NULL)
    {
        RL_FREE(wrapper);
        return NULL;
    }

    return (void*)wrapper;
}

int RLWin32RemoveMessageHookByHandle(void* hwnd, void* token)
{
    HWND hNativeWindowHandle = (HWND)hwnd;
    if (hNativeWindowHandle == NULL || token == NULL) return 0;
    if (!RLWin32IsKnownWindowHandle_Internal(hNativeWindowHandle)) return 0;

    RLWin32HookWrapper* wrapper = (RLWin32HookWrapper*)token;
    if (wrapper->hwnd && wrapper->hwnd != hNativeWindowHandle) return 0;

    const int isDispatchOk = (int)RLWin32DispatchToHwnd(hNativeWindowHandle, RLWin32Dispatch_RemoveHook, wrapper);
    if (isDispatchOk) RL_FREE(wrapper);
    return isDispatchOk;
}

// Generic cross-thread invoke helpers (Win32)
//
// These are low-level primitives intended for advanced integrations.
// - Window-thread invoke: runs on the Win32 GUI thread that owns the HWND (safe for Win32 UI ops).
// - Render-thread invoke: runs on the render thread associated with that window (safe for raylib/GL for that window).
//
// NOTE: In non-event-thread mode, render-thread invoke only works when called from the same thread
//       that currently owns the target OpenGL context.

#ifndef RL_WIN32_WINDOW_THREAD_INVOKE_DEFINED
#define RL_WIN32_WINDOW_THREAD_INVOKE_DEFINED
typedef intptr_t (*RLWin32WindowThreadInvoke)(void* hwnd, void* user);
#endif


typedef struct RLWin32UserInvokeCall
{
    RLWin32WindowThreadInvoke fn;
    void* hwnd;
    void* user;
    int autoFree;
} RLWin32UserInvokeCall;

static LRESULT RLWin32Dispatch_InvokeUser(GLFWwindow* window, HWND hWnd, void* user)
{
    (void)window;
    RLWin32UserInvokeCall* c = (RLWin32UserInvokeCall*)user;
    if (!c || !c->fn)
    {
        if (c && c->autoFree) RL_FREE(c);
        return (LRESULT)0;
    }

    const intptr_t r = c->fn(c->hwnd ? c->hwnd : (void*)hWnd, c->user);

    if (c->autoFree) RL_FREE(c);
    return (LRESULT)r;
}

intptr_t RLWin32InvokeOnWindowThreadByHandle(void* hwnd, RLWin32WindowThreadInvoke fn, void* user, int wait)
{
    if (!hwnd || !fn) return (intptr_t)0;

    HWND hNativeWindowHandle = (HWND)hwnd;

    // If already on the owning window thread, run inline.
    const DWORD ownerTid = GetWindowThreadProcessId(hNativeWindowHandle, NULL);
    if (ownerTid != 0 && ownerTid == GetCurrentThreadId())
    {
        return fn(hwnd, user);
    }

    if (wait)
    {
        RLWin32UserInvokeCall call = { fn, hwnd, user, 0 };
        return (intptr_t)RLWin32DispatchToHwnd(hNativeWindowHandle, RLWin32Dispatch_InvokeUser, &call);
    }

    RLWin32UserInvokeCall* call = (RLWin32UserInvokeCall*)RL_CALLOC(1, sizeof(RLWin32UserInvokeCall));
    if (!call) return (intptr_t)0;

    call->fn = fn;
    call->hwnd = hwnd;
    call->user = user;
    call->autoFree = 1;

    return PostMessageW(hNativeWindowHandle,                        /* hWnd */
                        RLWin32GetDispatchMessageId(),              /* Msg */
                        (WPARAM)RLWin32Dispatch_InvokeUser,         /* wParam */
                        (LPARAM)call) ? (intptr_t)1 : (intptr_t)0;  /* lParam */
}

#ifndef RL_WINDOW_RENDER_THREAD_INVOKE_DEFINED
#define RL_WINDOW_RENDER_THREAD_INVOKE_DEFINED
typedef intptr_t (*RLWindowRenderThreadInvoke)(void* hwnd, void* user);
#endif


typedef struct RLRenderUserInvokeCall
{
    RLWindowRenderThreadInvoke fn;
    void* hwnd;
    void* user;
    intptr_t result;
    RLEvent* done;
    int autoFree;
} RLRenderUserInvokeCall;

static void RLGlfwTask_InvokeUserOnRenderThread(void* user)
{
    RLRenderUserInvokeCall* c = (RLRenderUserInvokeCall*)user;
    if (!c || !c->fn)
    {
        if (c && c->done) RLEventSignal(c->done);
        if (c && c->autoFree) RL_FREE(c);
        return;
    }

    c->result = c->fn(c->hwnd, c->user);

    if (c->done) RLEventSignal(c->done);
    if (c->autoFree) RL_FREE(c);
}

intptr_t RLInvokeOnWindowRenderThreadByHandle(void* hwnd, RLWindowRenderThreadInvoke fn, void* user, int wait)
{
    if (!hwnd || !fn) return (intptr_t)0;

    HWND h = (HWND)hwnd;
    PlatformData* pd = RLWin32FindPlatformByHwnd(h);
    if (!pd || !pd->ownerCtx) return (intptr_t)0;

    // Non-event-thread mode: only safe from the thread that currently owns this GL context.
    if (!pd->useEventThread)
    {
        if (glfwGetCurrentContext() != pd->handle) return (intptr_t)0;

        RLContext* prev = RLGetCurrentContext();
        if (prev != pd->ownerCtx) RLSetCurrentContext(pd->ownerCtx);
        const intptr_t r = fn(hwnd, user);
        if (prev != pd->ownerCtx) RLSetCurrentContext(prev);
        return r;
    }

    if (!pd->renderThread || !pd->renderWakeEvent) return (intptr_t)0;

    RLRenderUserInvokeCall* call = (RLRenderUserInvokeCall*)RL_CALLOC(1, sizeof(RLRenderUserInvokeCall));
    if (!call) return (intptr_t)0;

    call->fn = fn;
    call->hwnd = hwnd;
    call->user = user;
    call->done = wait ? RLEventCreate(false) : NULL;
    call->autoFree = wait ? 0 : 1;

    RLGlfwRenderCall* rc = (RLGlfwRenderCall*)RL_CALLOC(1, sizeof(RLGlfwRenderCall));

    if (!rc)
    {
        if (call->done) RLEventDestroy(call->done);
        RL_FREE(call);
        return (intptr_t)0;
    }

    RL_DIAG_RENDERCALL_ALLOC(sizeof(RLGlfwRenderCall));
    RL_DIAG_TASK_POSTED();

    rc->ctx = pd->ownerCtx;
    rc->fn = RLGlfwTask_InvokeUserOnRenderThread;
    rc->user = call;

    glfwPostTask(pd->renderThread, RLGlfwRenderCallTrampoline, rc);
    RLGlfwSignalOneRenderWake(pd);

    if (wait)
    {
        RLEventWait(call->done);
        const intptr_t r = call->result;
        RLEventDestroy(call->done);
        RL_FREE(call);
        return r;
    }

    return (intptr_t)1;
}
#endif


// Get number of monitors
int RLGetMonitorCount(void)
{
    int monitorCount = 0;

#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwRunOnEventThread(RLGlfwTask_QueryMonitorCount, &monitorCount, true);
        return monitorCount;
    }
#endif

    glfwGetMonitors(&monitorCount);

    return monitorCount;
}

// Get current monitor where window is placed
int RLGetCurrentMonitor(void)
{
#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        int idx = 0;
        RLGlfwRunOnEventThread(RLGlfwTask_QueryCurrentMonitorIndex, &idx, true);
        return idx;
    }
#endif

    int index = 0;
    int monitorCount = 0;
    GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);
    GLFWmonitor *monitor = NULL;

    if (monitorCount >= 1)
    {
        if (RLIsWindowFullscreen())
        {
            // Get the handle of the monitor that the specified window is in full screen on
            monitor = glfwGetWindowMonitor(platform.handle);

            for (int i = 0; i < monitorCount; i++)
            {
                if (monitors[i] == monitor)
                {
                    index = i;
                    break;
                }
            }
        }
        else
        {
            // In case the window is between two monitors, below logic is used
            // to try to detect the "current monitor" for that window, note that
            // this is probably an overengineered solution for a very side case
            // trying to match SDL behaviour

            int closestDist = 0x7FFFFFFF;

            // Window center position
            int wcx = 0;
            int wcy = 0;

            glfwGetWindowPos(platform.handle, &wcx, &wcy);
            wcx += (int)CORE.Window.screen.width/2;
            wcy += (int)CORE.Window.screen.height/2;

            for (int i = 0; i < monitorCount; i++)
            {
                // Monitor top-left position
                int mx = 0;
                int my = 0;

                monitor = monitors[i];
                glfwGetMonitorPos(monitor, &mx, &my);
                const GLFWvidmode *mode = glfwGetVideoMode(monitor);

                if (mode)
                {
                    const int right = mx + mode->width - 1;
                    const int bottom = my + mode->height - 1;

                    if ((wcx >= mx) &&
                        (wcx <= right) &&
                        (wcy >= my) &&
                        (wcy <= bottom))
                    {
                        index = i;
                        break;
                    }

                    int xclosest = wcx;
                    if (wcx < mx) xclosest = mx;
                    else if (wcx > right) xclosest = right;

                    int yclosest = wcy;
                    if (wcy < my) yclosest = my;
                    else if (wcy > bottom) yclosest = bottom;

                    int dx = wcx - xclosest;
                    int dy = wcy - yclosest;
                    int dist = (dx*dx) + (dy*dy);
                    if (dist < closestDist)
                    {
                        index = i;
                        closestDist = dist;
                    }
                }
                else TRACELOG(LOG_WARNING, "GLFW: Failed to find video mode for selected monitor");
            }
        }
    }

    return index;
}

// Get selected monitor position
RLVector2 RLGetMonitorPosition(int monitor)
{
    int x = 0;
    int y = 0;

#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwMonitorInfo info = { 0 };
        info.index = monitor;
        RLGlfwRunOnEventThread(RLGlfwTask_QueryMonitorInfo, &info, true);
        if (info.ok)
        {
            x = info.posX;
            y = info.posY;
            return (RLVector2){ (float)x, (float)y };
        }

        TRACELOG(LOG_WARNING, "GLFW: Failed to find selected monitor");
        return (RLVector2){ 0, 0 };
    }
#endif

    int monitorCount = 0;
    GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

    if ((monitor >= 0) && (monitor < monitorCount))
    {
        glfwGetMonitorPos(monitors[monitor], &x, &y);
        return (RLVector2){ (float)x, (float)y };
    }
    else TRACELOG(LOG_WARNING, "GLFW: Failed to find selected monitor");
    return (RLVector2){ 0, 0 };
}

// Get selected monitor width (currently used by monitor)
int RLGetMonitorWidth(int monitor)
{
    int width = 0;

#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwMonitorInfo info = { 0 };
        info.index = monitor;
        RLGlfwRunOnEventThread(RLGlfwTask_QueryMonitorInfo, &info, true);
        if (info.ok) width = info.modeW;
        else TRACELOG(LOG_WARNING, "GLFW: Failed to find selected monitor");
        return width;
    }
#endif

    int monitorCount = 0;
    GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

    if ((monitor >= 0) && (monitor < monitorCount))
    {
        const GLFWvidmode *mode = glfwGetVideoMode(monitors[monitor]);
        if (mode) width = mode->width;
        else TRACELOG(LOG_WARNING, "GLFW: Failed to find video mode for selected monitor");
    }
    else TRACELOG(LOG_WARNING, "GLFW: Failed to find selected monitor");

    return width;
}

// Get selected monitor height (currently used by monitor)
int RLGetMonitorHeight(int monitor)
{
    int height = 0;

#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwMonitorInfo info = { 0 };
        info.index = monitor;
        RLGlfwRunOnEventThread(RLGlfwTask_QueryMonitorInfo, &info, true);
        if (info.ok) height = info.modeH;
        else TRACELOG(LOG_WARNING, "GLFW: Failed to find selected monitor");
        return height;
    }
#endif

    int monitorCount = 0;
    GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

    if ((monitor >= 0) && (monitor < monitorCount))
    {
        const GLFWvidmode *mode = glfwGetVideoMode(monitors[monitor]);

        if (mode) height = mode->height;
        else TRACELOG(LOG_WARNING, "GLFW: Failed to find video mode for selected monitor");
    }
    else TRACELOG(LOG_WARNING, "GLFW: Failed to find selected monitor");

    return height;
}

// Get selected monitor physical width in millimetres
int RLGetMonitorPhysicalWidth(int monitor)
{
    int width = 0;

#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwMonitorInfo info = { 0 };
        info.index = monitor;
        RLGlfwRunOnEventThread(RLGlfwTask_QueryMonitorInfo, &info, true);
        if (info.ok) width = info.physW;
        else TRACELOG(LOG_WARNING, "GLFW: Failed to find selected monitor");
        return width;
    }
#endif

    int monitorCount = 0;
    GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

    if ((monitor >= 0) && (monitor < monitorCount)) glfwGetMonitorPhysicalSize(monitors[monitor], &width, NULL);
    else TRACELOG(LOG_WARNING, "GLFW: Failed to find selected monitor");

    return width;
}

// Get selected monitor physical height in millimetres
int RLGetMonitorPhysicalHeight(int monitor)
{
    int height = 0;

#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwMonitorInfo info = { 0 };
        info.index = monitor;
        RLGlfwRunOnEventThread(RLGlfwTask_QueryMonitorInfo, &info, true);
        if (info.ok) height = info.physH;
        else TRACELOG(LOG_WARNING, "GLFW: Failed to find selected monitor");
        return height;
    }
#endif

    int monitorCount = 0;
    GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

    if ((monitor >= 0) && (monitor < monitorCount)) glfwGetMonitorPhysicalSize(monitors[monitor], NULL, &height);
    else TRACELOG(LOG_WARNING, "GLFW: Failed to find selected monitor");

    return height;
}

// Get selected monitor refresh rate
int RLGetMonitorRefreshRate(int monitor)
{
    int refresh = 0;

#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwMonitorInfo info = { 0 };
        info.index = monitor;
        RLGlfwRunOnEventThread(RLGlfwTask_QueryMonitorInfo, &info, true);
        if (info.ok) refresh = info.refresh;
        else TRACELOG(LOG_WARNING, "GLFW: Failed to find selected monitor");
        return refresh;
    }
#endif

    int monitorCount = 0;
    GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

    if ((monitor >= 0) && (monitor < monitorCount))
    {
        const GLFWvidmode *mode = glfwGetVideoMode(monitors[monitor]);

        if (mode) refresh = mode->refreshRate;
        else TRACELOG(LOG_WARNING, "GLFW: Failed to find video mode for selected monitor");

    }
    else TRACELOG(LOG_WARNING, "GLFW: Failed to find selected monitor");

    return refresh;
}

// Get the human-readable, UTF-8 encoded name of the selected monitor
const char *RLGetMonitorName(int monitor)
{
    const char *name = "";

#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwMonitorInfo info = { 0 };
        info.index = monitor;
        RLGlfwRunOnEventThread(RLGlfwTask_QueryMonitorInfo, &info, true);
        if (info.ok) name = (info.name != NULL)? info.name : "";
        else TRACELOG(LOG_WARNING, "GLFW: Failed to find selected monitor");
        return name;
    }
#endif

    int monitorCount = 0;
    GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

    if ((monitor >= 0) && (monitor < monitorCount))
    {
        return glfwGetMonitorName(monitors[monitor]);
    }
    else TRACELOG(LOG_WARNING, "GLFW: Failed to find selected monitor");
    return "";
}

// Get window position XY on monitor
RLVector2 RLGetWindowPosition(void)
{
    return (RLVector2){ (float)CORE.Window.position.x, (float)CORE.Window.position.y };
}

// Get window scale DPI factor for current monitor
RLVector2 RLGetWindowScaleDPI(void)
{
    RLVector2 scale = { 1.0f, 1.0f };
    if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_HIGHDPI) && !FLAG_IS_SET(CORE.Window.flags, FLAG_FULLSCREEN_MODE))
    {
#if defined(_WIN32)
        if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
        {
            RLGlfwContentScaleTask t = { 1.0f, 1.0f };
            RLGlfwRunOnEventThread(RLGlfwTask_GetWindowContentScale, &t, true);
            scale.x = t.x;
            scale.y = t.y;
        }
        else glfwGetWindowContentScale(platform.handle, &scale.x, &scale.y);
#else
        glfwGetWindowContentScale(platform.handle, &scale.x, &scale.y);
#endif
    }
    return scale;
}

// Set clipboard text content
void RLSetClipboardText(const char *text)
{
#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwRunOnEventThread(RLGlfwTask_SetClipboardText, (void *)text, true);
        return;
    }
#endif
    glfwSetClipboardString(platform.handle, (text != NULL)? text : "");
}

// Get clipboard text content
// NOTE: returned string is allocated and freed by GLFW
const char *RLGetClipboardText(void)
{
#if defined(_WIN32)
    if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
    {
        RLGlfwClipboardGetTask t = { 0 };
        RLGlfwRunOnEventThread(RLGlfwTask_GetClipboardText, &t, true);
        return t.out;
    }
#endif
    return glfwGetClipboardString(platform.handle);
}

// Get clipboard image
RLImage RLGetClipboardImage(void)
{
    RLImage image = { 0 };

#if defined(SUPPORT_CLIPBOARD_IMAGE)
#if defined(_WIN32)
    unsigned long long int dataSize = 0;
    void *bmpData = NULL;
    int width = 0;
    int height = 0;

    bmpData  = (void *)Win32GetClipboardImageData(&width, &height, &dataSize);

    if (bmpData == NULL) TRACELOG(LOG_WARNING, "Clipboard image: Couldn't get clipboard data.");
    else image = RLLoadImageFromMemory(".bmp", (const unsigned char *)bmpData, (int)dataSize);
#else
    TRACELOG(LOG_WARNING, "GetClipboardImage() not implemented on target platform");
#endif
#endif // SUPPORT_CLIPBOARD_IMAGE

    return image;
}

// Show mouse cursor
void RLShowCursor(void)
{
    glfwSetInputMode(platform.handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    CORE.Input.Mouse.cursorHidden = false;
}

// Hides mouse cursor
void RLHideCursor(void)
{
    glfwSetInputMode(platform.handle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    CORE.Input.Mouse.cursorHidden = true;
}

// Enables cursor (unlock cursor)
void RLEnableCursor(void)
{
    glfwSetInputMode(platform.handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Set cursor position in the middle
    RLSetMousePosition(CORE.Window.screen.width/2, CORE.Window.screen.height/2);

    if (glfwRawMouseMotionSupported()) glfwSetInputMode(platform.handle, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

    CORE.Input.Mouse.cursorHidden = false;
    CORE.Input.Mouse.cursorLocked = false;
}

// Disables cursor (lock cursor)
void RLDisableCursor(void)
{
    // Reset mouse position within the window area before disabling cursor
    RLSetMousePosition(CORE.Window.screen.width/2, CORE.Window.screen.height/2);

    glfwSetInputMode(platform.handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (glfwRawMouseMotionSupported()) glfwSetInputMode(platform.handle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    CORE.Input.Mouse.cursorHidden = true;
    CORE.Input.Mouse.cursorLocked = true;
}

// Swap back buffer with front buffer (screen drawing)
void RLSwapScreenBuffer(void)
{
    glfwSwapBuffers(platform.handle);
}

//----------------------------------------------------------------------------------
// Module Functions Definition: Misc
//----------------------------------------------------------------------------------

// Get elapsed time measure in seconds since InitTimer()
double RLGetTime(void)
{
    double time = glfwGetTime();   // Elapsed time since glfwInit()
    return time;
}

// Open URL with default system browser (if available)
// NOTE: This function is only safe to use if you control the URL given
// A user could craft a malicious string performing another action
// Only call this function yourself not with user input or make sure to check the string yourself
// REF: https://github.com/raysan5/raylib/issues/686
void RLOpenURL(const char *url)
{
    // Security check to (partially) avoid malicious code
    if (strchr(url, '\'') != NULL) TRACELOG(LOG_WARNING, "SYSTEM: Provided URL could be potentially malicious, avoid [\'] character");
    else
    {
        char *cmd = (char *)RL_CALLOC(strlen(url) + 32, sizeof(char));
#if defined(_WIN32)
        sprintf(cmd, "explorer \"%s\"", url);
#endif
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__)
        sprintf(cmd, "xdg-open '%s'", url); // Alternatives: firefox, x-www-browser
#endif
#if defined(__APPLE__)
        sprintf(cmd, "open '%s'", url);
#endif
        int result = system(cmd);
        if (result == -1) TRACELOG(LOG_WARNING, "OpenURL() child process could not be created");
        RL_FREE(cmd);
    }
}

//----------------------------------------------------------------------------------
// Module Functions Definition: Inputs
//----------------------------------------------------------------------------------

// Set internal gamepad mappings
int RLSetGamepadMappings(const char *mappings)
{
    return glfwUpdateGamepadMappings(mappings);
}

// Set gamepad vibration
void RLSetGamepadVibration(int gamepad, float leftMotor, float rightMotor, float duration)
{
    TRACELOG(LOG_WARNING, "SetGamepadVibration() not available on target platform");
}

// Set mouse position XY
void RLSetMousePosition(int x, int y)
{
    CORE.Input.Mouse.currentPosition = (RLVector2){ (float)x, (float)y };
    CORE.Input.Mouse.previousPosition = CORE.Input.Mouse.currentPosition;

    // NOTE: emscripten not implemented
    glfwSetCursorPos(platform.handle, CORE.Input.Mouse.currentPosition.x, CORE.Input.Mouse.currentPosition.y);
}

// Set mouse cursor
void RLSetMouseCursor(int cursor)
{
    CORE.Input.Mouse.cursor = cursor;
    if (cursor == MOUSE_CURSOR_DEFAULT) glfwSetCursor(platform.handle, NULL);
    else
    {
        // NOTE: Mapping internal GLFW enum values to MouseCursor enum values
        glfwSetCursor(platform.handle, glfwCreateStandardCursor(0x00036000 + cursor));
    }
}

// Get physical key name
const char *RLGetKeyName(int key)
{
    return glfwGetKeyName(key, glfwGetKeyScancode(key));
}

// Register all input events
void RLPollInputEvents(void)
{
#if defined(SUPPORT_GESTURES_SYSTEM)
    // NOTE: Gestures update must be called every frame to reset gestures correctly
    // because ProcessGestureEvent() is just called on an event, not every frame
    UpdateGestures();
#endif

    // Reset keys/chars pressed registered
    CORE.Input.Keyboard.keyPressedQueueCount = 0;
    CORE.Input.Keyboard.charPressedQueueCount = 0;

    // Reset last gamepad button/axis registered state
    CORE.Input.Gamepad.lastButtonPressed = GAMEPAD_BUTTON_UNKNOWN;
    //CORE.Input.Gamepad.axisCount = 0;

    // Keyboard/Mouse input polling (automatically managed by GLFW3 through callback)

    // Register previous keys states
    for (int i = 0; i < MAX_KEYBOARD_KEYS; i++)
    {
        CORE.Input.Keyboard.previousKeyState[i] = CORE.Input.Keyboard.currentKeyState[i];
        CORE.Input.Keyboard.keyRepeatInFrame[i] = 0;
    }

    // Register previous mouse states
    for (int i = 0; i < MAX_MOUSE_BUTTONS; i++) CORE.Input.Mouse.previousButtonState[i] = CORE.Input.Mouse.currentButtonState[i];

    // Register previous mouse wheel state
    CORE.Input.Mouse.previousWheelMove = CORE.Input.Mouse.currentWheelMove;
    CORE.Input.Mouse.currentWheelMove = (RLVector2){ 0.0f, 0.0f };

    // Register previous mouse position
    CORE.Input.Mouse.previousPosition = CORE.Input.Mouse.currentPosition;

    // Register previous touch states
    for (int i = 0; i < MAX_TOUCH_POINTS; i++) CORE.Input.Touch.previousTouchState[i] = CORE.Input.Touch.currentTouchState[i];

    // Reset touch positions
    //for (int i = 0; i < MAX_TOUCH_POINTS; i++) CORE.Input.Touch.position[i] = (Vector2){ 0, 0 };

    // Map touch position to mouse position for convenience
    // WARNING: If the target desktop device supports touch screen, this behaviour should be reviewed!
    // TODO: GLFW does not support multi-touch input yet
    // REF: https://www.codeproject.com/Articles/668404/Programming-for-Multi-Touch
    // REF: https://docs.microsoft.com/en-us/windows/win32/wintouch/getting-started-with-multi-touch-messages
    CORE.Input.Touch.position[0] = CORE.Input.Mouse.currentPosition;

    // Check if gamepads are ready
    // NOTE: Doing it here in case of disconnection
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (glfwJoystickPresent(i)) CORE.Input.Gamepad.ready[i] = true;
        else CORE.Input.Gamepad.ready[i] = false;
    }

    // Register gamepads buttons events
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (CORE.Input.Gamepad.ready[i])     // Check if gamepad is available
        {
            // Register previous gamepad states
            for (int k = 0; k < MAX_GAMEPAD_BUTTONS; k++) CORE.Input.Gamepad.previousButtonState[i][k] = CORE.Input.Gamepad.currentButtonState[i][k];

            // Get current gamepad state
            // NOTE: There is no callback available, getting it manually
            GLFWgamepadstate state = { 0 };
            int result = glfwGetGamepadState(i, &state); // This remaps all gamepads so they have their buttons mapped like an xbox controller
            if (result == GLFW_FALSE) // No joystick is connected, no gamepad mapping or an error occurred
            {
                // Setting axes to expected resting value instead of GLFW 0.0f default when gamepad is not connected
                state.axes[GAMEPAD_AXIS_LEFT_TRIGGER] = -1.0f;
                state.axes[GAMEPAD_AXIS_RIGHT_TRIGGER] = -1.0f;
            }

            const unsigned char *buttons = state.buttons;

            for (int k = 0; (buttons != NULL) && (k < MAX_GAMEPAD_BUTTONS); k++)
            {
                int button = -1;        // GamepadButton enum values assigned

                switch (k)
                {
                    case GLFW_GAMEPAD_BUTTON_Y: button = GAMEPAD_BUTTON_RIGHT_FACE_UP; break;
                    case GLFW_GAMEPAD_BUTTON_B: button = GAMEPAD_BUTTON_RIGHT_FACE_RIGHT; break;
                    case GLFW_GAMEPAD_BUTTON_A: button = GAMEPAD_BUTTON_RIGHT_FACE_DOWN; break;
                    case GLFW_GAMEPAD_BUTTON_X: button = GAMEPAD_BUTTON_RIGHT_FACE_LEFT; break;

                    case GLFW_GAMEPAD_BUTTON_LEFT_BUMPER: button = GAMEPAD_BUTTON_LEFT_TRIGGER_1; break;
                    case GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER: button = GAMEPAD_BUTTON_RIGHT_TRIGGER_1; break;

                    case GLFW_GAMEPAD_BUTTON_BACK: button = GAMEPAD_BUTTON_MIDDLE_LEFT; break;
                    case GLFW_GAMEPAD_BUTTON_GUIDE: button = GAMEPAD_BUTTON_MIDDLE; break;
                    case GLFW_GAMEPAD_BUTTON_START: button = GAMEPAD_BUTTON_MIDDLE_RIGHT; break;

                    case GLFW_GAMEPAD_BUTTON_DPAD_UP: button = GAMEPAD_BUTTON_LEFT_FACE_UP; break;
                    case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT: button = GAMEPAD_BUTTON_LEFT_FACE_RIGHT; break;
                    case GLFW_GAMEPAD_BUTTON_DPAD_DOWN: button = GAMEPAD_BUTTON_LEFT_FACE_DOWN; break;
                    case GLFW_GAMEPAD_BUTTON_DPAD_LEFT: button = GAMEPAD_BUTTON_LEFT_FACE_LEFT; break;

                    case GLFW_GAMEPAD_BUTTON_LEFT_THUMB: button = GAMEPAD_BUTTON_LEFT_THUMB; break;
                    case GLFW_GAMEPAD_BUTTON_RIGHT_THUMB: button = GAMEPAD_BUTTON_RIGHT_THUMB; break;
                    default: break;
                }

                if (button != -1)   // Check for valid button
                {
                    if (buttons[k] == GLFW_PRESS)
                    {
                        CORE.Input.Gamepad.currentButtonState[i][button] = 1;
                        CORE.Input.Gamepad.lastButtonPressed = button;
                    }
                    else CORE.Input.Gamepad.currentButtonState[i][button] = 0;
                }
            }

            // Get current state of axes
            const float *axes = state.axes;

            for (int k = 0; (axes != NULL) && (k < GLFW_GAMEPAD_AXIS_LAST + 1); k++)
            {
                CORE.Input.Gamepad.axisState[i][k] = axes[k];
            }

            // Register buttons for 2nd triggers (because GLFW doesn't count these as buttons but rather as axes)
            if (CORE.Input.Gamepad.axisState[i][GAMEPAD_AXIS_LEFT_TRIGGER] > 0.1f)
            {
                CORE.Input.Gamepad.currentButtonState[i][GAMEPAD_BUTTON_LEFT_TRIGGER_2] = 1;
                CORE.Input.Gamepad.lastButtonPressed = GAMEPAD_BUTTON_LEFT_TRIGGER_2;
            }
            else CORE.Input.Gamepad.currentButtonState[i][GAMEPAD_BUTTON_LEFT_TRIGGER_2] = 0;
            if (CORE.Input.Gamepad.axisState[i][GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.1f)
            {
                CORE.Input.Gamepad.currentButtonState[i][GAMEPAD_BUTTON_RIGHT_TRIGGER_2] = 1;
                CORE.Input.Gamepad.lastButtonPressed = GAMEPAD_BUTTON_RIGHT_TRIGGER_2;
            }
            else CORE.Input.Gamepad.currentButtonState[i][GAMEPAD_BUTTON_RIGHT_TRIGGER_2] = 0;

            CORE.Input.Gamepad.axisCount[i] = GLFW_GAMEPAD_AXIS_LAST + 1;
        }
    }
    CORE.Window.resizedLastFrame = false;

    // Drain tasks posted to this thread (thread-aware GLFW extensions; no-op on non-Win32 builds)
    RLGlfwPumpThreadTasksWithDiag();

#if defined(_WIN32)
    if (platform.useEventThread)
    {
        // In event-thread mode, the Win32 message thread performs glfwWaitEvents/glfwPollEvents.
        // The render thread only blocks on a dedicated wake event.
        if ((CORE.Window.eventWaiting) ||
            (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_MINIMIZED) && !FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_ALWAYS_RUN)))
        {
            // Pause semantics: block the render thread while minimized/eventWaiting (like glfwWaitEvents()).
            // NOTE: We only enable the timeout safety-net during shutdown (platform.closing), otherwise
            // the periodic wake would let the main loop tick (render/audio) while minimized.
            if (platform.renderWakeEvent != NULL)
            {
                if (platform.closing) (void)RLEventWaitTimeout(platform.renderWakeEvent, 250);
                else (void)RLEventWait(platform.renderWakeEvent);
            }
            CORE.Time.previous = RLGetTime();
        }

        // Close intent is forwarded through WindowCloseCallback -> RLGlfwTask_WindowClose.
        if (platform.handle == NULL) CORE.Window.shouldClose = true;
        return;
    }
#endif

    if ((CORE.Window.eventWaiting) ||
        (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_MINIMIZED) && !FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_ALWAYS_RUN)))
    {
        // NOTE(Route2): glfwWaitEvents() blocks this calling thread until an event arrives.
        // It must not be wrapped by a process-wide lock, otherwise other windows/threads can stall.
        glfwWaitEvents();     // Wait for input events before continue (drawing is paused)
        CORE.Time.previous = RLGetTime();
    }
    else
    {
        // NOTE(Route2): glfwPollEvents() can enter modal loops on Windows (drag/resize) via DefWindowProc,
        // so it must not be wrapped by a process-wide lock (otherwise other windows/threads will stall).
        glfwPollEvents();      // Poll input events: keyboard/mouse/window events (callbacks) -> Update keys state
    }

    CORE.Window.shouldClose = glfwWindowShouldClose(platform.handle);

    // Reset close status for next frame
}

//----------------------------------------------------------------------------------
// Module Internal Functions Definition
//----------------------------------------------------------------------------------
// Function wrappers around RL_*ALLOC macros, used by glfwInitAllocator() inside of InitPlatform()
// GLFWallocator expects function pointers with specific signatures to be provided
// REF: https://www.glfw.org/docs/latest/intro_guide.html#init_allocator
static void *AllocateWrapper(size_t size, void *user)
{
    (void)user;
    return RL_CALLOC(size, 1);
}
static void *ReallocateWrapper(void *block, size_t size, void *user)
{
    (void)user;
    return RL_REALLOC(block, size);
}
static void DeallocateWrapper(void *block, void *user)
{
    (void)user;
    RL_FREE(block);
}

// Initialize platform: graphics, inputs and more
int InitPlatform(void)
{
    glfwSetErrorCallback(ErrorCallback);

    // NOTE (Route2): glfwInit/glfwTerminate are process-global.
    // We guard lifetime with a global refcount and serialize critical calls.
    // Custom GLFW allocators/init-hints are intentionally not used here (they must
    // be configured before the first glfwInit, which becomes hard to coordinate
    // across multiple threads). If you want them back, do it in the Stage-B plan.
    if (!RLGlfwGlobalAcquire()) { TRACELOG(LOG_WARNING, "GLFW: Failed to initialize GLFW"); return -1; }

    bool holdGlobalLock = false;

    // Initialize graphic device: display/window and graphic context
    //----------------------------------------------------------------------------
    glfwDefaultWindowHints();                       // Set default windows hints
    //glfwWindowHint(GLFW_RED_BITS, 8);             // Framebuffer red color component bits
    //glfwWindowHint(GLFW_GREEN_BITS, 8);           // Framebuffer green color component bits
    //glfwWindowHint(GLFW_BLUE_BITS, 8);            // Framebuffer blue color component bits
    //glfwWindowHint(GLFW_ALPHA_BITS, 8);           // Framebuffer alpha color component bits
    //glfwWindowHint(GLFW_DEPTH_BITS, 24);          // Depthbuffer bits
    //glfwWindowHint(GLFW_REFRESH_RATE, 0);         // Refresh rate for fullscreen window
    //glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API); // OpenGL API to use. Alternative: GLFW_OPENGL_ES_API
    //glfwWindowHint(GLFW_AUX_BUFFERS, 0);          // Number of auxiliar buffers

    // Disable GlFW auto iconify behaviour
    // Auto Iconify automatically minimizes (iconifies) the window if the window loses focus
    // additionally auto iconify restores the hardware resolution of the monitor if the window that loses focus is a fullscreen window
    glfwWindowHint(GLFW_AUTO_ICONIFY, 0);

    // Window flags requested before initialization to be applied after initialization
    unsigned int requestedWindowFlags = CORE.Window.flags;

#if defined(_WIN32)
    // Optional per-window Win32 class name override (one-shot)
    RLContext* ctxHint = RLGetCurrentContext();
    if (ctxHint && ctxHint->win32ClassName[0])
    {
        glfwWindowHintString(GLFW_WIN32_CLASS_NAME, ctxHint->win32ClassName);
        ctxHint->win32ClassName[0] = '\0';
    }
#endif

#if defined(_WIN32)
    // Win32 optional event-thread mode: run the GLFW event/message pump on a dedicated
    // thread while keeping rendering on the caller thread.
    platform.useEventThread = FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_EVENT_THREAD);
    platform.ownerCtx = RLGetCurrentContext();
    platform.broadcastWake = FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_BROADCAST_WAKE);
    platform.renderThread = platform.useEventThread? glfwGetCurrentThread() : NULL;
    platform.eventThread = NULL;
    platform.createdEvent = NULL;
    platform.renderWakeEvent = NULL;
    platform.eventThreadHandle = NULL;
    platform.eventThreadStop = 0;
#endif

    // Check window creation flags
    if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_HIDDEN)) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Visible window
    else glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);     // Window initially hidden

    if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_UNDECORATED)) glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // Border and buttons on Window
    else glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);   // Decorated window

    if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_RESIZABLE)) glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); // Resizable window
    else glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);  // Avoid window being resizable

#if defined(_WIN32)
    // Keep Win32 Snap Layout affordances even when the window is not user-resizable
    glfwWindowHint(GLFW_WIN32_SNAP_LAYOUT, FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_SNAP_LAYOUT)? GLFW_TRUE : GLFW_FALSE);
#endif

    // Disable FLAG_WINDOW_MINIMIZED, not supported on initialization
    if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_MINIMIZED)) FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_MINIMIZED);

    // Disable FLAG_WINDOW_MAXIMIZED, not supported on initialization
    if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_MAXIMIZED)) FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_MAXIMIZED);

    if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_UNFOCUSED)) glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
    else glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);

    if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_TOPMOST)) glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    else glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);

    // NOTE: Some GLFW flags are not supported on HTML5
    if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_TRANSPARENT)) glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);     // Transparent framebuffer
    else glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);  // Opaque framebuffer

    if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_HIGHDPI))
    {
#if defined(__APPLE__)
        glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_FALSE);
#endif
        // Resize window content area based on the monitor content scale
        // NOTE: This hint only has an effect on platforms where screen coordinates and
        // pixels always map 1:1 such as Windows and X11
        // On platforms like macOS the resolution of the framebuffer is changed independently of the window size
        glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
#if defined(__APPLE__)
        glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_TRUE);
#endif
    }
    else
    {
        glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
#if defined(__APPLE__)
        glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_FALSE);
#endif
    }

    // Mouse passthrough
    if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_MOUSE_PASSTHROUGH)) glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
    else glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);

    if (FLAG_IS_SET(CORE.Window.flags, FLAG_MSAA_4X_HINT))
    {
        // NOTE: MSAA is only enabled for main framebuffer, not user-created FBOs
        TRACELOG(LOG_INFO, "DISPLAY: Trying to enable MSAA x4");
        glfwWindowHint(GLFW_SAMPLES, 4);   // Tries to enable multisampling x4 (MSAA), default is 0
    }

    // NOTE: When asking for an OpenGL context version, most drivers provide the highest supported version
    // with backward compatibility to older OpenGL versions
    // For example, if using OpenGL 1.1, driver can provide a 4.3 backwards compatible context

    // Check selection OpenGL version
    if (rlGetVersion() == RL_OPENGL_21)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);          // Choose OpenGL major version (just hint)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);          // Choose OpenGL minor version (just hint)
    }
    else if (rlGetVersion() == RL_OPENGL_33)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);          // Choose OpenGL major version (just hint)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);          // Choose OpenGL minor version (just hint)
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Profiles Hint: Only 3.3 and above!
                                                                       // Values: GLFW_OPENGL_CORE_PROFILE, GLFW_OPENGL_ANY_PROFILE, GLFW_OPENGL_COMPAT_PROFILE
#if defined(__APPLE__)
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);  // OSX Requires forward compatibility
#else
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_FALSE); // Forward Compatibility Hint: Only 3.3 and above!
#endif
        //glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE); // Request OpenGL DEBUG context
    }
    else if (rlGetVersion() == RL_OPENGL_43)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);          // Choose OpenGL major version (just hint)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);          // Choose OpenGL minor version (just hint)
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_FALSE);
#if defined(RLGL_ENABLE_OPENGL_DEBUG_CONTEXT)
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);   // Enable OpenGL Debug Context
#endif
    }
    else if (rlGetVersion() == RL_OPENGL_ES_20)                 // Request OpenGL ES 2.0 context
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    }
    else if (rlGetVersion() == RL_OPENGL_ES_30)                 // Request OpenGL ES 3.0 context
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    }

    // NOTE: GLFW 3.4+ defers initialization of the Joystick subsystem on the first call to any Joystick related functions
    // Forcing this initialization here avoids doing it on PollInputEvents() called by EndDrawing() after first frame has been just drawn
    // The initialization will still happen and possible delays still occur, but before the window is shown, which is a nicer experience
    // REF: https://github.com/raysan5/raylib/issues/1554
    // Route2: serialize window creation against other threads polling/destroying.
    RLGlfwGlobalLock();
    holdGlobalLock = true;
    glfwSetJoystickCallback(NULL);

    if ((CORE.Window.screen.width == 0) || (CORE.Window.screen.height == 0)) FLAG_SET(CORE.Window.flags, FLAG_FULLSCREEN_MODE);

#if defined(_WIN32)
    if (platform.useEventThread)
    {
        // Create the window on the event thread (Win32 message thread).
        // We must release the global lock here to avoid deadlocks.
        RLGlfwGlobalUnlock();
        holdGlobalLock = false;

        platform.createdEvent = RLEventCreate(false);
        platform.renderWakeEvent = RLEventCreate(false);
        platform.eventThreadStop = 0;
        platform.closing = 0;

        // Register this platform so shutdown/close can broadcast-wake sleeping render threads.
        RLGlfwPlatformRegister(&platform);

        RLGlfwEventThreadStart *start = (RLGlfwEventThreadStart *)RL_MALLOC(sizeof(RLGlfwEventThreadStart));
        RL_DIAG_PAYLOAD_ALLOC(RL_DIAG_PAYLOAD_OTHER, sizeof(RLGlfwEventThreadStart));
        start->ctx = RLGetCurrentContext();

        platform.eventThreadHandle = RLThreadCreate(RLGlfwEventThreadMain, start);
        if (platform.eventThreadHandle == NULL)
        {
            RL_DIAG_PAYLOAD_FREE(RL_DIAG_PAYLOAD_OTHER, sizeof(RLGlfwEventThreadStart));
            RL_FREE(start);
            TRACELOG(LOG_WARNING, "GLFW: Failed to create event thread");
            if (platform.createdEvent) { RLEventDestroy(platform.createdEvent); platform.createdEvent = NULL; }
            if (platform.renderWakeEvent) { RLEventDestroy(platform.renderWakeEvent); platform.renderWakeEvent = NULL; }
            RLGlfwPlatformUnregister(&platform);
	    	platform.win32Hwnd = NULL;
            RLGlfwGlobalRelease();
            return -1;
        }

        // Wait until the event thread creates the window (or fails).
        RLEventWait(platform.createdEvent);

        if (platform.handle == NULL)
        {
            TRACELOG(LOG_WARNING, "GLFW: Failed to initialize Window (event thread)");
            platform.eventThreadStop = 1;
            RLGlfwWakeEventThread();
            RLThreadJoin(platform.eventThreadHandle);
            RLThreadDestroy(platform.eventThreadHandle);
            platform.eventThreadHandle = NULL;

            if (platform.createdEvent) { RLEventDestroy(platform.createdEvent); platform.createdEvent = NULL; }
            if (platform.renderWakeEvent) { RLEventDestroy(platform.renderWakeEvent); platform.renderWakeEvent = NULL; }

            RLGlfwPlatformUnregister(&platform);

            RLGlfwGlobalRelease();
            return -1;
        }

        // Continue with remaining initialization on the render thread.
        goto _rlglfw_window_created;
    }
#endif

    // Init window in fullscreen mode if requested
    // NOTE: Keeping original screen size for toggle
    if (FLAG_IS_SET(CORE.Window.flags, FLAG_FULLSCREEN_MODE))
    {
        // NOTE: Fullscreen applications default to the primary monitor
        GLFWmonitor *monitor = glfwGetPrimaryMonitor();
        if (!monitor)
        {
            TRACELOG(LOG_WARNING, "GLFW: Failed to get primary monitor");
            RLGlfwGlobalRelease();
            RLGlfwGlobalUnlock();
            return -1;
        }

        // Set dimensions from monitor
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);

        // Default display resolution to that of the current mode
        CORE.Window.display.width = mode->width;
        CORE.Window.display.height = mode->height;

        // Check if user requested some screen size
        if ((CORE.Window.screen.width == 0) || (CORE.Window.screen.height == 0))
        {
            // Set some default screen size in case user decides to exit fullscreen mode
            CORE.Window.previousScreen.width = 800;
            CORE.Window.previousScreen.height = 450;
            CORE.Window.previousPosition.x = CORE.Window.display.width/2 - 800/2;
            CORE.Window.previousPosition.y = CORE.Window.display.height/2 - 450/2;

            // Set screen width/height to the display width/height
            if (CORE.Window.screen.width == 0) CORE.Window.screen.width = CORE.Window.display.width;
            if (CORE.Window.screen.height == 0) CORE.Window.screen.height = CORE.Window.display.height;
        }
        else
        {
            CORE.Window.previousScreen = CORE.Window.screen;
            CORE.Window.screen = CORE.Window.display;
        }

        GLFWwindow *shareWindow = RLGlfwResolveShareWindowForContext(RLGetCurrentContext());

        platform.handle = glfwCreateWindow(CORE.Window.screen.width, CORE.Window.screen.height, (CORE.Window.title != 0)? CORE.Window.title : " ", monitor, shareWindow);
        if (!platform.handle)
        {
            RLGlfwGlobalRelease();
            TRACELOG(LOG_WARNING, "GLFW: Failed to initialize Window");
            RLGlfwGlobalUnlock();
            return -1;
        }

        // Bind this GLFW window to the current raylib context (Route2 multi-window)
        glfwSetWindowUserPointer(platform.handle, (void *)RLGetCurrentContext());

    }
    else
    {
        // Default to at least one pixel in size, as creation with a zero dimension is not allowed
        if (CORE.Window.screen.width == 0) CORE.Window.screen.width = 1;
        if (CORE.Window.screen.height == 0) CORE.Window.screen.height = 1;

        GLFWwindow *shareWindow = RLGlfwResolveShareWindowForContext(RLGetCurrentContext());

        platform.handle = glfwCreateWindow(CORE.Window.screen.width, CORE.Window.screen.height, (CORE.Window.title != 0)? CORE.Window.title : " ", NULL, shareWindow);
        if (!platform.handle)
        {
            RLGlfwGlobalRelease();
            TRACELOG(LOG_WARNING, "GLFW: Failed to initialize Window");
            RLGlfwGlobalUnlock();
            return -1;
        }

        // Bind this GLFW window to the current raylib context (Route2 multi-window)
        glfwSetWindowUserPointer(platform.handle, (void *)RLGetCurrentContext());

        // After the window was created, determine the monitor that the window manager assigned
        // Derive display sizes and, if possible, window size in case it was zero at beginning

        int monitorCount = 0;
        int monitorIndex = RLGetCurrentMonitor();
        GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

        if (monitorIndex < monitorCount)
        {
            GLFWmonitor *monitor = monitors[monitorIndex];
            const GLFWvidmode *mode = glfwGetVideoMode(monitor);

            // Default display resolution to that of the current mode
            CORE.Window.display.width = mode->width;
            CORE.Window.display.height = mode->height;

            // Set screen width/height to the display width/height if they are 0
            if (CORE.Window.screen.width == 0) CORE.Window.screen.width = CORE.Window.display.width;
            if (CORE.Window.screen.height == 0) CORE.Window.screen.height = CORE.Window.display.height;

            glfwSetWindowSize(platform.handle, CORE.Window.screen.width, CORE.Window.screen.height);
        }
        else
        {
            // The monitor for the window-manager-created window can not be determined, so it can not be centered
            glfwDestroyWindow(platform.handle);
            platform.handle = NULL;
            RLGlfwGlobalRelease();
            TRACELOG(LOG_WARNING, "GLFW: Failed to determine Monitor to center Window");
            RLGlfwGlobalUnlock();
            return -1;
        }

        // NOTE: Not considering scale factor now, considered below
        CORE.Window.render.width = CORE.Window.screen.width;
        CORE.Window.render.height = CORE.Window.screen.height;
    }

_rlglfw_window_created:

    // Track primary window semantics and reset stale global quit when starting a fresh run.
#if defined(_WIN32)
    RLGlfwTrackWindowCreated(platform.handle, holdGlobalLock);

    // Cache HWND and ensure this PlatformData participates in the global registry.
    if (platform.win32Hwnd == NULL) platform.win32Hwnd = (HWND)glfwGetWin32Window(platform.handle);
    RLGlfwPlatformRegister(&platform);

    // NOTE: In useEventThread mode the native window is created on the GLFW event thread.
    // Some Win32-specific behavior (like Snap Layout affordances) must be explicitly synced
    // after the GLFWwindow exists, otherwise RLSetConfigFlags() may appear ineffective.
    if (FLAG_IS_SET(requestedWindowFlags, FLAG_WINDOW_SNAP_LAYOUT))
    {
        // Force-sync the GLFW window attribute to the requested flag.
        RLGlfwSetWindowAttribThreadAware(GLFW_WIN32_SNAP_LAYOUT, GLFW_TRUE);
    }
#endif

    glfwMakeContextCurrent(platform.handle);
    int result = glfwGetError(NULL);
    if ((result != GLFW_NO_WINDOW_CONTEXT) && (result != GLFW_PLATFORM_ERROR)) CORE.Window.ready = true; // Checking context activation

    if (CORE.Window.ready)
    {
        // Setup additional windows configs and register required window size info

        glfwSwapInterval(0); // No V-Sync by default

        // Try to enable GPU V-Sync, so frames are limited to screen refresh rate (60Hz -> 60 FPS)
        // NOTE: V-Sync can be enabled by graphic driver configuration, it doesn't need
        // to be activated on web platforms since VSync is enforced there
        if (FLAG_IS_SET(CORE.Window.flags, FLAG_VSYNC_HINT))
        {
            // WARNING: It seems to hit a critical render path in Intel HD Graphics
            glfwSwapInterval(1);
            TRACELOG(LOG_INFO, "DISPLAY: Trying to enable VSYNC");
        }

        int fbWidth = CORE.Window.screen.width;
        int fbHeight = CORE.Window.screen.height;

        if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_HIGHDPI))
        {
            // NOTE: On APPLE platforms system should manage window/input scaling and also framebuffer scaling
            // Framebuffer scaling is activated with: glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_TRUE);

            // Get current framebuffer size, on high-dpi it could be bigger than screen size
            glfwGetFramebufferSize(platform.handle, &fbWidth, &fbHeight);

            // Screen scaling matrix is required in case desired screen area is different from display area
            CORE.Window.screenScale = MatrixScale((float)fbWidth/CORE.Window.screen.width, (float)fbHeight/CORE.Window.screen.height, 1.0f);
#if !defined(__APPLE__)
            // Mouse input scaling for the new screen size
            RLSetMouseScale((float)CORE.Window.screen.width/fbWidth, (float)CORE.Window.screen.height/fbHeight);
#endif
        }

        CORE.Window.render.width = fbWidth;
        CORE.Window.render.height = fbHeight;
        CORE.Window.currentFbo.width = fbWidth;
        CORE.Window.currentFbo.height = fbHeight;

        TRACELOG(LOG_INFO, "DISPLAY: Device initialized successfully");
        TRACELOG(LOG_INFO, "    > Display size: %i x %i", CORE.Window.display.width, CORE.Window.display.height);
        TRACELOG(LOG_INFO, "    > Screen size:  %i x %i", CORE.Window.screen.width, CORE.Window.screen.height);
        TRACELOG(LOG_INFO, "    > Render size:  %i x %i", CORE.Window.render.width, CORE.Window.render.height);
        TRACELOG(LOG_INFO, "    > Viewport offsets: %i, %i", CORE.Window.renderOffset.x, CORE.Window.renderOffset.y);

        // Try to center window on screen but avoiding window-bar outside of screen
        // NOTE: In Win32 event-thread mode, monitor queries must be performed on the event thread.
        int monitorX = 0;
        int monitorY = 0;
        int monitorWidth = 0;
        int monitorHeight = 0;
        int monitorIndex = RLGetCurrentMonitor();

#if defined(_WIN32)
        if (platform.useEventThread && !RLGlfwIsThread(platform.eventThread))
        {
            RLGlfwMonitorInfo info = { 0 };
            info.index = monitorIndex;
            RLGlfwRunOnEventThread(RLGlfwTask_QueryMonitorInfo, &info, true);
            if (info.ok)
            {
                monitorX = info.workX;
                monitorY = info.workY;
                monitorWidth = info.workW;
                monitorHeight = info.workH;
            }
            else TRACELOG(LOG_WARNING, "GLFW: Failed to query current monitor workarea");
        }
        else
#endif
        {
            int monitorCount = 0;
            GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);
            GLFWmonitor *monitor = ((monitors != NULL) && (monitorIndex >= 0) && (monitorIndex < monitorCount))? monitors[monitorIndex] : NULL;

            if (monitor != NULL) glfwGetMonitorWorkarea(monitor, &monitorX, &monitorY, &monitorWidth, &monitorHeight);
            else TRACELOG(LOG_WARNING, "GLFW: Failed to query current monitor workarea");
        }

        // TODO: Here CORE.Window.render.width/height should be used instead of
        // CORE.Window.screen.width/height to center the window correctly when the high dpi flag is enabled
        CORE.Window.position.x = monitorX + (monitorWidth - (int)CORE.Window.screen.width)/2;
        CORE.Window.position.y = monitorY + (monitorHeight - (int)CORE.Window.screen.height)/2;
        //if (CORE.Window.position.x < monitorX) CORE.Window.position.x = monitorX;
        //if (CORE.Window.position.y < monitorY) CORE.Window.position.y = monitorY;

        RLSetWindowPosition(CORE.Window.position.x, CORE.Window.position.y);

        if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_MINIMIZED)) RLMinimizeWindow();
    }
    else
    {
        TRACELOG(LOG_FATAL, "PLATFORM: Failed to initialize graphics device");
        if (platform.handle != NULL)
        {
            glfwDestroyWindow(platform.handle);
            platform.handle = NULL;
        }
        RLGlfwGlobalRelease();
        RLGlfwGlobalUnlock();
        return -1;
    }

    // Apply window flags requested previous to initialization
    RLSetWindowState(requestedWindowFlags);

    // Load OpenGL extensions
    // NOTE: GL procedures address loader is required to load extensions
    rlLoadExtensions(glfwGetProcAddress);
    //----------------------------------------------------------------------------

    // Initialize input events callbacks
    //----------------------------------------------------------------------------
    // Set window callback events
#if defined(_WIN32)
    if (!platform.useEventThread)
    {
#endif
        glfwSetWindowSizeCallback(platform.handle, WindowSizeCallback); // NOTE: Resizing is not enabled by default
        glfwSetFramebufferSizeCallback(platform.handle, FramebufferSizeCallback);
        glfwSetWindowPosCallback(platform.handle, WindowPosCallback);
        glfwSetWindowMaximizeCallback(platform.handle, WindowMaximizeCallback);
        if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_REFRESH_CALLBACK))
            glfwSetWindowRefreshCallback(platform.handle, WindowRefreshCallback);
        glfwSetWindowIconifyCallback(platform.handle, WindowIconifyCallback);
        glfwSetWindowFocusCallback(platform.handle, WindowFocusCallback);
#if defined(_WIN32)
        // Ensure we get a primary-close signal even in non-event-thread mode.
        // This is required to wake sleeping event-thread render loops for a clean shutdown.
        glfwSetWindowCloseCallback(platform.handle, WindowCloseCallback);
#endif
        glfwSetDropCallback(platform.handle, WindowDropCallback);
        if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_HIGHDPI)) glfwSetWindowContentScaleCallback(platform.handle, WindowContentScaleCallback);

        // Set input callback events
        glfwSetKeyCallback(platform.handle, KeyCallback);
        glfwSetCharCallback(platform.handle, CharCallback);
        glfwSetMouseButtonCallback(platform.handle, MouseButtonCallback);
        glfwSetCursorPosCallback(platform.handle, MouseCursorPosCallback); // Track mouse position changes
        glfwSetScrollCallback(platform.handle, MouseScrollCallback);
        glfwSetCursorEnterCallback(platform.handle, CursorEnterCallback);
        glfwSetJoystickCallback(JoystickCallback);
        glfwSetInputMode(platform.handle, GLFW_LOCK_KEY_MODS, GLFW_TRUE); // Enable lock keys modifiers (CAPS, NUM)
#if defined(_WIN32)
    }
#endif

    // Retrieve gamepad names
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        // WARNING: If glfwGetJoystickName() is longer than MAX_GAMEPAD_NAME_LENGTH,
        // only copying up to (MAX_GAMEPAD_NAME_LENGTH - 1)
        if (glfwJoystickPresent(i))
        {
          CORE.Input.Gamepad.ready[i] = true;
          CORE.Input.Gamepad.axisCount[i] = GLFW_GAMEPAD_AXIS_LAST + 1;
          strncpy(CORE.Input.Gamepad.name[i], glfwGetJoystickName(i), MAX_GAMEPAD_NAME_LENGTH - 1);
        }
    }
    //----------------------------------------------------------------------------

    // Initialize timming system
    //----------------------------------------------------------------------------
    InitTimer();
    //----------------------------------------------------------------------------

    // Initialize storage system
    //----------------------------------------------------------------------------
    CORE.Storage.basePath = RLGetWorkingDirectory();
    //----------------------------------------------------------------------------

#if defined(__NetBSD__)
    // Workaround for NetBSD
    char *glfwPlatform = "X11 (NetBSD)";
#else
    char *glfwPlatform = "";
    switch (glfwGetPlatform())
    {
        case GLFW_PLATFORM_WIN32: glfwPlatform = "Win32"; break;
        case GLFW_PLATFORM_COCOA: glfwPlatform = "Cocoa"; break;
        case GLFW_PLATFORM_WAYLAND: glfwPlatform = "Wayland"; break;
        case GLFW_PLATFORM_X11: glfwPlatform = "X11"; break;
        case GLFW_PLATFORM_NULL: glfwPlatform = "Null"; break;
        default: break;
    }
#endif

    TRACELOG(LOG_INFO, "PLATFORM: DESKTOP (GLFW - %s): Initialized successfully", glfwPlatform);

	// Release global GLFW window-lifecycle lock acquired during InitPlatform().
	if (holdGlobalLock) RLGlfwGlobalUnlock();

    return 0;
}

// Close platform
void ClosePlatform(void)
{
#if defined(_WIN32)
    // Capture the handle value early (it may be nulled during destruction).
    GLFWwindow *closingWindow = platform.handle;
#endif

    // Win32 event-thread mode: window must be destroyed on the Win32 message thread.
#if defined(_WIN32)
    if (platform.useEventThread)
    {
        // Mark closing early so callbacks stop posting non-critical tasks.
        platform.closing = 1;

        // Closing the primary window implies a process-wide quit request.
        if (RLGlfwIsPrimaryPlatform(&platform)) RLGlfwRequestGlobalQuit();

        // Wake behavior is configurable: default wakes only this window, but during shutdown
        // (or if FLAG_WINDOW_BROADCAST_WAKE is set) we may broadcast to all windows.
        RLGlfwSignalWakeByPolicy(&platform, true);
        // Detach GL context from the render thread.
        if (glfwGetCurrentContext() == platform.handle) glfwMakeContextCurrent(NULL);

        // Destroy the window on the message thread (synchronous).
        if (platform.handle != NULL) RLGlfwRunOnEventThread(RLGlfwTask_DestroyWindow, NULL, true);

        // Stop and join the message thread.
        platform.eventThreadStop = 1;
        RLGlfwWakeEventThread();
        if (platform.eventThreadHandle != NULL)
        {
            RLThreadJoin(platform.eventThreadHandle);
            RLThreadDestroy(platform.eventThreadHandle);
            platform.eventThreadHandle = NULL;
        }

        // Drain any pending render-thread tasks that were posted before the event thread stopped.
        // This prevents tasks from touching CORE/ctx after they are freed by higher-level teardown.
        RLGlfwDrainRenderThreadTasks();

        // Remove from the broadcast registry *before* destroying the wake events.
        // Otherwise another thread broadcasting a wake during shutdown could touch freed handles.
        RLEvent *createdEvt = platform.createdEvent;
        RLEvent *wakeEvt = platform.renderWakeEvent;
        platform.createdEvent = NULL;
        platform.renderWakeEvent = NULL;
        platform.eventThread = NULL;
        platform.renderThread = NULL;

        RLGlfwPlatformUnregister(&platform);

        // Update global primary/window-count tracking after teardown.
        RLGlfwTrackWindowDestroyed(closingWindow, false);

        if (createdEvt) RLEventDestroy(createdEvt);
        if (wakeEvt) RLEventDestroy(wakeEvt);

        RLGlfwGlobalRelease();

#if defined(_WIN32) && defined(SUPPORT_WINMM_HIGHRES_TIMER) && !defined(SUPPORT_BUSY_WAIT_LOOP)
        timeEndPeriod(1);           // Restore time period
#endif
        return;
    }
#endif

    // Non event-thread path: serialize window destruction against other threads polling events
    // (glfwPollEvents/glfwWaitEvents are global and can race with glfwDestroyWindow).
    RLGlfwGlobalLock();

    if (platform.handle)
    {
        if (glfwGetCurrentContext() == platform.handle) glfwMakeContextCurrent(NULL);
        glfwDestroyWindow(platform.handle);
        platform.handle = NULL;
    }

#if defined(_WIN32)
    // Update global primary/window-count tracking while the global lock is held.
    RLGlfwTrackWindowDestroyed(closingWindow, true);
    RLGlfwPlatformUnregister(&platform);
    platform.win32Hwnd = NULL;
#endif

    RLGlfwGlobalUnlock();

    RLGlfwGlobalRelease();

#if defined(_WIN32) && defined(SUPPORT_WINMM_HIGHRES_TIMER) && !defined(SUPPORT_BUSY_WAIT_LOOP)
    timeEndPeriod(1);           // Restore time period
#endif
}

//----------------------------------------------------------------------------------
// Module Internal Functions Definition
// NOTE: Those functions are only required for current platform
//----------------------------------------------------------------------------------

// GLFW3: Error callback, runs on GLFW3 error
static void ErrorCallback(int error, const char *description)
{
    TRACELOG(LOG_WARNING, "GLFW: Error: %i Description: %s", error, description);
}

// GLFW3: Window size change callback, runs when window is resized
// NOTE: Window resizing not enabled by default, use SetConfigFlags()
//----------------------------------------------------------------------------------
// GLFW callbacks context binding
//----------------------------------------------------------------------------------
static inline bool RLGlfwBindCallbackContext(GLFWwindow *window)
{
    RLContext *ctx = (RLContext *)glfwGetWindowUserPointer(window);
    if (!ctx) return false;
    RLSetCurrentContext(ctx);
    return true;
}

#if defined(_WIN32)
// Render-thread tasks used by event-thread callbacks
typedef struct { int x; int y; } RLGlfwPosI2;
typedef struct { int w; int h; } RLGlfwSizeI2;
typedef struct { float x; float y; } RLGlfwPosF2;

typedef struct { int key; int scancode; int action; int mods; } RLGlfwKeyEvent;
typedef struct { unsigned int codepoint; } RLGlfwCharEvent;
typedef struct { int button; int action; int mods; } RLGlfwMouseButtonEvent;
typedef struct { double xpos; double ypos; } RLGlfwMouseMoveEvent;
typedef struct { double xoffset; double yoffset; } RLGlfwMouseWheelEvent;
typedef struct { int entered; } RLGlfwCursorEnterEvent;
typedef struct { int focused; } RLGlfwWindowFocusEvent;
typedef struct { int iconified; } RLGlfwWindowIconifyEvent;
typedef struct { int maximized; } RLGlfwWindowMaximizeEvent;
typedef struct { float sx; float sy; } RLGlfwWindowScaleEvent;
typedef struct { int count; char **paths; } RLGlfwDropEvent;
typedef struct { int jid; int event; char *name; } RLGlfwJoystickEvent;
typedef struct { int shouldClose; } RLGlfwWindowCloseEvent;

static void RLGlfwTask_WindowPos(void *user);
#if RL_EVENTTHREAD_COALESCE_STATE
static void RLGlfwTask_DrainPendingInput(void *user);
#endif
static void RLGlfwTask_FramebufferSize(void *user);
static void RLGlfwTask_WindowContentScale(void *user);
static void RLGlfwTask_WindowIconify(void *user);
static void RLGlfwTask_WindowMaximize(void *user);
static void RLGlfwTask_WindowFocus(void *user);
static void RLGlfwTask_WindowRefresh(void *user);
static void RLGlfwTask_WindowClose(void *user);
static void RLGlfwTask_Drop(void *user);
static void RLGlfwTask_Key(void *user);
static void RLGlfwTask_Char(void *user);
static void RLGlfwTask_MouseButton(void *user);
static void RLGlfwTask_MouseMove(void *user);
static void RLGlfwTask_MouseWheel(void *user);
static void RLGlfwTask_CursorEnter(void *user);
static void RLGlfwTask_Joystick(void *user);
#endif // _WIN32

static void WindowSizeCallback(GLFWwindow *window, int width, int height)
{
    if (!RLGlfwBindCallbackContext(window)) return;

    // Nothing to do for now on window resize...
    //TRACELOG(LOG_INFO, "GLFW3: Window size callback called [%i,%i]", width, height);
}

// GLFW3: Framebuffer size change callback, runs when framebuffer is resized
// WARNING: If FLAG_WINDOW_HIGHDPI is set, WindowContentScaleCallback() is called before this function
static void FramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    #if defined(_WIN32)
    {
        RLContext *ctx = (RLContext *)glfwGetWindowUserPointer(window);
        PlatformData *pd = (ctx != NULL)? (PlatformData *)ctx->platformData : NULL;
        if ((pd != NULL) && pd->useEventThread)
        {
            if (pd->closing) return;
            // WARNING: On window minimization, callback is called with 0 values,
            // but internal screen values should not be changed, it breaks things
            if ((width == 0) || (height == 0)) return;

#if RL_EVENTTHREAD_COALESCE_STATE
            RLSetCurrentContext(ctx);
            RLAtomicExchangeLong(&pd->pendingFbW, (long)width);
            RLAtomicExchangeLong(&pd->pendingFbH, (long)height);
            RLAtomicOrLong(&pd->pendingMask, RL_PENDING_FB_SIZE);
            RLGlfwQueuePendingDrain(ctx, pd);
            return;
#else
            RLSetCurrentContext(ctx);
            RLGlfwSizeI2 *e = (RLGlfwSizeI2 *)RL_MALLOC(sizeof(RLGlfwSizeI2));
            RL_DIAG_PAYLOAD_ALLOC(RL_DIAG_PAYLOAD_FBSIZE, sizeof(RLGlfwSizeI2));
            e->w = width; e->h = height;
            RLGlfwRunOnRenderThread(ctx, RLGlfwTask_FramebufferSize, e);
            return;
#endif
        }
    }
    #endif

    if (!RLGlfwBindCallbackContext(window)) return;

    //TRACELOG(LOG_INFO, "GLFW3: Window framebuffer size callback called [%i,%i]", width, height);

    // WARNING: On window minimization, callback is called with 0 values,
    // but internal screen values should not be changed, it breaks things
    if ((width == 0) || (height == 0)) return;

    // Reset viewport and projection matrix for new size
    // NOTE: Stores current render size: CORE.Window.render
    SetupViewport(width, height);

    // Set render size
    CORE.Window.currentFbo.width = width;
    CORE.Window.currentFbo.height = height;
    CORE.Window.resizedLastFrame = true;

    // If the window is tearing down, ignore late size notifications.
    if (!CORE.Window.ready || CORE.Window.shouldClose) return;

    if (FLAG_IS_SET(CORE.Window.flags, FLAG_FULLSCREEN_MODE))
    {
        // On fullscreen mode, strategy is ignoring high-dpi and
        // use the all available display size

        // Set screen size to render size (physical pixel size)
        CORE.Window.screen.width = width;
        CORE.Window.screen.height = height;
        CORE.Window.screenScale = MatrixScale(1.0f, 1.0f, 1.0f);
        RLSetMouseScale(1.0f, 1.0f);
    }
    else // Window mode (including borderless window)
    {
        // Check if render size was actually scaled for high-dpi
        if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_HIGHDPI))
        {
            // Set screen size to logical pixel size, considering content scaling
            RLVector2 scaleDpi = RLGetWindowScaleDPI();
            CORE.Window.screen.width = (int)((float)width/scaleDpi.x);
            CORE.Window.screen.height = (int)((float)height/scaleDpi.y);
            CORE.Window.screenScale = MatrixScale(scaleDpi.x, scaleDpi.y, 1.0f);
#if !defined(__APPLE__)
            // Mouse input scaling for the new screen size
            RLSetMouseScale(1.0f/scaleDpi.x, 1.0f/scaleDpi.y);
#endif
        }
        else
        {
            // Set screen size to render size (physical pixel size)
            CORE.Window.screen.width = width;
            CORE.Window.screen.height = height;
        }
    }

    // WARNING: If using a render texture, it is not scaled to new size
}

// GLFW3: Window content scale callback, runs on monitor content scale change detected
// WARNING: If FLAG_WINDOW_HIGHDPI is not set, this function is not called
static void WindowContentScaleCallback(GLFWwindow *window, float scalex, float scaley)
{
#if defined(_WIN32)
    {
        RLContext *ctx = (RLContext *)glfwGetWindowUserPointer(window);
        PlatformData *pd = (ctx != NULL)? (PlatformData *)ctx->platformData : NULL;
        if ((pd != NULL) && pd->useEventThread)
        {
            if (pd->closing) return;
#if RL_EVENTTHREAD_COALESCE_STATE
            RLSetCurrentContext(ctx);
            RLAtomicExchangeLong(&pd->pendingScaleXBits, RLFloatBitsFromFloat(scalex));
            RLAtomicExchangeLong(&pd->pendingScaleYBits, RLFloatBitsFromFloat(scaley));
            RLAtomicOrLong(&pd->pendingMask, RL_PENDING_SCALE);
            RLGlfwQueuePendingDrain(ctx, pd);
            return;
#else
            RLSetCurrentContext(ctx);
            RLGlfwWindowScaleEvent *e = (RLGlfwWindowScaleEvent *)RL_MALLOC(sizeof(RLGlfwWindowScaleEvent));
            RL_DIAG_PAYLOAD_ALLOC(RL_DIAG_PAYLOAD_SCALE, sizeof(RLGlfwWindowScaleEvent));
            e->sx = scalex; e->sy = scaley;
            RLGlfwRunOnRenderThread(ctx, RLGlfwTask_WindowContentScale, e);
            return;
#endif
        }
    }
#endif

    if (!RLGlfwBindCallbackContext(window)) return;

    //TRACELOG(LOG_INFO, "GLFW3: Window content scale changed, scale: [%.2f,%.2f]", scalex, scaley);

    float fbWidth = (float)CORE.Window.screen.width*scalex;
    float fbHeight = (float)CORE.Window.screen.height*scaley;

    // NOTE: On APPLE platforms system should manage window/input scaling and also framebuffer scaling
    // Framebuffer scaling is activated with: glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_TRUE);
    CORE.Window.screenScale = MatrixScale(scalex, scaley, 1.0f);

#if !defined(__APPLE__)
    // Mouse input scaling for the new screen size
    RLSetMouseScale(1.0f/scalex, 1.0f/scaley);
#endif

    CORE.Window.render.width = (int)fbWidth;
    CORE.Window.render.height = (int)fbHeight;
    CORE.Window.currentFbo = CORE.Window.render;
}

// GLFW3: Window position callback, runs when window position changes
static void WindowPosCallback(GLFWwindow *window, int x, int y)
{
#if defined(_WIN32)
    {
        RLContext *ctx = (RLContext *)glfwGetWindowUserPointer(window);
        PlatformData *pd = (ctx != NULL)? (PlatformData *)ctx->platformData : NULL;
        if ((pd != NULL) && pd->useEventThread)
        {
            if (pd->closing) return;
#if RL_EVENTTHREAD_COALESCE_STATE
            RLSetCurrentContext(ctx);
            RLAtomicExchangeLong(&pd->pendingWinX, (long)x);
            RLAtomicExchangeLong(&pd->pendingWinY, (long)y);
            RLAtomicOrLong(&pd->pendingMask, RL_PENDING_WIN_POS);
            RLGlfwQueuePendingDrain(ctx, pd);
            return;
#else
            RLSetCurrentContext(ctx);
            RLGlfwPosI2 *e = (RLGlfwPosI2 *)RL_MALLOC(sizeof(RLGlfwPosI2));
            RL_DIAG_PAYLOAD_ALLOC(RL_DIAG_PAYLOAD_WINPOS, sizeof(RLGlfwPosI2));
            e->x = x; e->y = y;
            RLGlfwRunOnRenderThread(ctx, RLGlfwTask_WindowPos, e);
            return;
#endif
        }
    }
#endif
    if (!RLGlfwBindCallbackContext(window)) return;

    // Set current window position
    CORE.Window.position.x = x;
    CORE.Window.position.y = y;
}

// GLFW3: Window iconify callback, runs when window is minimized/restored
static void WindowIconifyCallback(GLFWwindow *window, int iconified)
{
#if defined(_WIN32)
    {
        RLContext *ctx = (RLContext *)glfwGetWindowUserPointer(window);
        PlatformData *pd = (ctx != NULL)? (PlatformData *)ctx->platformData : NULL;
        if ((pd != NULL) && pd->useEventThread)
        {
            if (pd->closing) return;
            RLSetCurrentContext(ctx);
            RLGlfwWindowIconifyEvent *e = (RLGlfwWindowIconifyEvent *)RL_MALLOC(sizeof(RLGlfwWindowIconifyEvent));
            RL_DIAG_PAYLOAD_ALLOC(RL_DIAG_PAYLOAD_OTHER, sizeof(RLGlfwWindowIconifyEvent));
            e->iconified = iconified;
            RLGlfwRunOnRenderThread(ctx, RLGlfwTask_WindowIconify, e);
            return;
        }
    }
#endif
    if (!RLGlfwBindCallbackContext(window)) return;

    if (iconified) FLAG_SET(CORE.Window.flags, FLAG_WINDOW_MINIMIZED);   // The window was iconified
    else FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_MINIMIZED);           // The window was restored
}

// GLFW3: Window maximize callback, runs when window is maximized/restored
static void WindowMaximizeCallback(GLFWwindow *window, int maximized)
{
#if defined(_WIN32)
    {
        RLContext *ctx = (RLContext *)glfwGetWindowUserPointer(window);
        PlatformData *pd = (ctx != NULL)? (PlatformData *)ctx->platformData : NULL;
        if ((pd != NULL) && pd->useEventThread)
        {
            if (pd->closing) return;
            RLSetCurrentContext(ctx);
            RLGlfwWindowMaximizeEvent *e = (RLGlfwWindowMaximizeEvent *)RL_MALLOC(sizeof(RLGlfwWindowMaximizeEvent));
            RL_DIAG_PAYLOAD_ALLOC(RL_DIAG_PAYLOAD_OTHER, sizeof(RLGlfwWindowMaximizeEvent));
            e->maximized = maximized;
            RLGlfwRunOnRenderThread(ctx, RLGlfwTask_WindowMaximize, e);
            return;
        }
    }
#endif
    if (!RLGlfwBindCallbackContext(window)) return;

    if (maximized) FLAG_SET(CORE.Window.flags, FLAG_WINDOW_MAXIMIZED);  // The window was maximized
    else FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_MAXIMIZED);          // The window was restored
}

static void RLGlfwInvokeUserWindowRefresh(bool postEmptyEvent)
{
    // Called on the thread that owns the GL context (render thread in event-thread mode; main thread otherwise).
    // On Win32, this can be invoked from inside a system modal loop (interactive move/size or menu tracking),
    // so it must avoid nested event polling or frame sleeping.

    CORE.Window.resizedLastFrame = true;

    if (!CORE.Window.ready || CORE.Window.shouldClose)
    {
        if (postEmptyEvent) glfwPostEmptyEvent();
        return;
    }

    // The user refresh callback is only enabled when FLAG_WINDOW_REFRESH_CALLBACK is set.
    // This gating must apply to both single-threaded and event-thread modes.
    if (!FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_REFRESH_CALLBACK))
    {
        if (postEmptyEvent) glfwPostEmptyEvent();
        return;
    }

    if ((CORE.Window.refreshCallback != NULL) && !CORE.Window.refreshCallbackActive)
    {
        CORE.Window.refreshCallbackActive = true;
        RLBeginDrawing();
        CORE.Window.refreshCallback();
        RLEndDrawing();
        CORE.Window.refreshCallbackActive = false;
        return;
    }

    if (postEmptyEvent) glfwPostEmptyEvent();
}

static void WindowRefreshCallback(GLFWwindow *window)
{
#if defined(_WIN32)
    {
        RLContext *ctx = (RLContext *)glfwGetWindowUserPointer(window);
        PlatformData *pd = (ctx != NULL)? (PlatformData *)ctx->platformData : NULL;
        if ((pd != NULL) && pd->useEventThread)
        {
            RLSetCurrentContext(ctx);
            if (pd->closing)
            {
                // Keep shutdown responsive even if the window is being destroyed.
                RLGlfwSignalWakeByPolicy(pd, true);
                return;
            }
            RLGlfwRunOnRenderThread(ctx, RLGlfwTask_WindowRefresh, NULL);
            RLGlfwSignalWakeByPolicy(pd, false);
            return;
        }
    }
#endif
    if (!RLGlfwBindCallbackContext(window)) return;

    RLGlfwInvokeUserWindowRefresh(true);
}

// GLFW3: Window close callback, runs when user requests closing the window (alt+F4, close button, etc.)
static void WindowCloseCallback(GLFWwindow *window)
{
#if defined(_WIN32)
    {
        RLContext *ctx = (RLContext *)glfwGetWindowUserPointer(window);
        PlatformData *pd = (ctx != NULL)? (PlatformData *)ctx->platformData : NULL;
        if ((pd != NULL) && pd->useEventThread)
        {
            RLSetCurrentContext(ctx);
            // Closing the primary window is treated as a process-wide quit request.
            if (RLGlfwIsPrimaryPlatform(pd)) RLGlfwRequestGlobalQuit();
            if (pd->closing)
            {
                // Ensure all render threads wake to observe the close intent.
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                RLGlfwSignalWakeByPolicy(pd, true);
                return;
            }
            // Mirror GLFW close intent immediately on the window thread.
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            RLGlfwWindowCloseEvent *e = (RLGlfwWindowCloseEvent *)RL_MALLOC(sizeof(RLGlfwWindowCloseEvent));
            RL_DIAG_PAYLOAD_ALLOC(RL_DIAG_PAYLOAD_WINCLOSE, sizeof(RLGlfwWindowCloseEvent));
            e->shouldClose = 1;
            RLGlfwRunOnRenderThread(ctx, RLGlfwTask_WindowClose, e);
            RLGlfwSignalWakeByPolicy(pd, true);
            return;
        }
    }
#endif

    if (!RLGlfwBindCallbackContext(window)) return;

#if defined(_WIN32)
    // Closing the primary window is treated as a process-wide quit request.
    // Even in non-event-thread mode, we need to wake any event-thread render loops
    // that might be blocked in minimized pause/WaitEvents.
    if (RLGlfwIsPrimaryWindow(window))
    {
        RLGlfwRequestGlobalQuit();
        RLGlfwSignalAllRenderWake();
        glfwPostEmptyEvent();
    }
#endif

    CORE.Window.shouldClose = true;
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}


// GLFW3: Window focus callback, runs when window get/lose focus
static void WindowFocusCallback(GLFWwindow *window, int focused)
{
#if defined(_WIN32)
    {
        RLContext *ctx = (RLContext *)glfwGetWindowUserPointer(window);
        PlatformData *pd = (ctx != NULL)? (PlatformData *)ctx->platformData : NULL;
        if ((pd != NULL) && pd->useEventThread)
        {
            if (pd->closing) return;
            RLSetCurrentContext(ctx);
            RLGlfwWindowFocusEvent *e = (RLGlfwWindowFocusEvent *)RL_MALLOC(sizeof(RLGlfwWindowFocusEvent));
            RL_DIAG_PAYLOAD_ALLOC(RL_DIAG_PAYLOAD_OTHER, sizeof(RLGlfwWindowFocusEvent));
            e->focused = focused;
            RLGlfwRunOnRenderThread(ctx, RLGlfwTask_WindowFocus, e);
            return;
        }
    }
#endif
    if (!RLGlfwBindCallbackContext(window)) return;

    if (focused) FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_UNFOCUSED);    // The window was focused
    else FLAG_SET(CORE.Window.flags, FLAG_WINDOW_UNFOCUSED);          // The window lost focus
}

// GLFW3: Window drop callback, runs when files are dropped into window
static void WindowDropCallback(GLFWwindow *window, int count, const char **paths)
{
#if defined(_WIN32)
    {
        RLContext *ctx = (RLContext *)glfwGetWindowUserPointer(window);
        PlatformData *pd = (ctx != NULL)? (PlatformData *)ctx->platformData : NULL;
        if ((pd != NULL) && pd->useEventThread)
        {
            if (pd->closing) return;
            RLSetCurrentContext(ctx);
            RLGlfwDropEvent *e = (RLGlfwDropEvent *)RL_MALLOC(sizeof(RLGlfwDropEvent));
            RL_DIAG_PAYLOAD_ALLOC(RL_DIAG_PAYLOAD_DROP, sizeof(RLGlfwDropEvent));
            e->count = count;
            e->paths = NULL;
            if (count > 0)
            {
                e->paths = (char **)RL_CALLOC((size_t)count, sizeof(char *));
                for (int i = 0; i < count; i++)
                {
                    size_t n = strlen(paths[i]);
                    e->paths[i] = (char *)RL_MALLOC(n + 1);
                    memcpy(e->paths[i], paths[i], n + 1);
                }
            }
            RLGlfwRunOnRenderThread(ctx, RLGlfwTask_Drop, e);
            return;
        }
    }
#endif
    if (!RLGlfwBindCallbackContext(window)) return;

    if (count > 0)
    {
        // In case previous dropped filepaths have not been freed, free them
        if (CORE.Window.dropFileCount > 0)
        {
            for (unsigned int i = 0; i < CORE.Window.dropFileCount; i++) RL_FREE(CORE.Window.dropFilepaths[i]);

            RL_FREE(CORE.Window.dropFilepaths);

            CORE.Window.dropFileCount = 0;
            CORE.Window.dropFilepaths = NULL;
        }

        // WARNING: Paths are freed by GLFW when the callback returns, keeping an internal copy
        CORE.Window.dropFileCount = count;
        CORE.Window.dropFilepaths = (char **)RL_CALLOC(CORE.Window.dropFileCount, sizeof(char *));

        for (unsigned int i = 0; i < CORE.Window.dropFileCount; i++)
        {
            CORE.Window.dropFilepaths[i] = (char *)RL_CALLOC(MAX_FILEPATH_LENGTH, sizeof(char));
            strncpy(CORE.Window.dropFilepaths[i], paths[i], MAX_FILEPATH_LENGTH - 1);
        }
    }
}

// GLFW3: Keyboard callback, runs on key pressed
static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
#if defined(_WIN32)
    {
        RLContext *ctx = (RLContext *)glfwGetWindowUserPointer(window);
        PlatformData *pd = (ctx != NULL)? (PlatformData *)ctx->platformData : NULL;
        if ((pd != NULL) && pd->useEventThread)
        {
            if (pd->closing) return;
            RLSetCurrentContext(ctx);
            RLGlfwKeyEvent *e = (RLGlfwKeyEvent *)RL_MALLOC(sizeof(RLGlfwKeyEvent));
            RL_DIAG_PAYLOAD_ALLOC(RL_DIAG_PAYLOAD_KEY, sizeof(RLGlfwKeyEvent));
            // NOTE: GLFW LockKeyMods does not include lock state in `mods`, so query it here on the
            // owning (message) thread and forward the combined value to the render thread.
            int combinedMods = mods;
            if (glfwGetKey(window, GLFW_KEY_CAPS_LOCK) == GLFW_PRESS) combinedMods |= GLFW_MOD_CAPS_LOCK;
            if (glfwGetKey(window, GLFW_KEY_NUM_LOCK) == GLFW_PRESS) combinedMods |= GLFW_MOD_NUM_LOCK;

            e->key = key;
            e->scancode = scancode;
            e->action = action;
            e->mods = combinedMods;
            RLGlfwRunOnRenderThread(ctx, RLGlfwTask_Key, e);
            return;
        }
    }
#endif
    if (!RLGlfwBindCallbackContext(window)) return;

    if (key < 0) return;    // Security check, macOS fn key generates -1

    // WARNING: GLFW could return GLFW_REPEAT, it needs to be considered as 1
    // to work properly with our implementation (IsKeyDown/IsKeyUp checks)
    if (action == GLFW_RELEASE) CORE.Input.Keyboard.currentKeyState[key] = 0;
    else if (action == GLFW_PRESS) CORE.Input.Keyboard.currentKeyState[key] = 1;
    else if (action == GLFW_REPEAT) CORE.Input.Keyboard.keyRepeatInFrame[key] = 1;

    // WARNING: Check if CAPS/NUM key modifiers are enabled and force down state for those keys
    if (((key == KEY_CAPS_LOCK) && (FLAG_IS_SET(mods, GLFW_MOD_CAPS_LOCK))) ||
        ((key == KEY_NUM_LOCK) && (FLAG_IS_SET(mods, GLFW_MOD_NUM_LOCK)))) CORE.Input.Keyboard.currentKeyState[key] = 1;

    // Check if there is space available in the key queue
    if ((CORE.Input.Keyboard.keyPressedQueueCount < MAX_KEY_PRESSED_QUEUE) && (action == GLFW_PRESS))
    {
        // Add character to the queue
        CORE.Input.Keyboard.keyPressedQueue[CORE.Input.Keyboard.keyPressedQueueCount] = key;
        CORE.Input.Keyboard.keyPressedQueueCount++;
    }

    // Check the exit key to set close window
    if ((key == CORE.Input.Keyboard.exitKey) && (action == GLFW_PRESS)) glfwSetWindowShouldClose(platform.handle, GLFW_TRUE);
}

// GLFW3: Char callback, runs on key pressed to get unicode codepoint value
static void CharCallback(GLFWwindow *window, unsigned int codepoint)
{
#if defined(_WIN32)
    {
        RLContext *ctx = (RLContext *)glfwGetWindowUserPointer(window);
        PlatformData *pd = (ctx != NULL)? (PlatformData *)ctx->platformData : NULL;
        if ((pd != NULL) && pd->useEventThread)
        {
            if (pd->closing) return;
            RLSetCurrentContext(ctx);
            RLGlfwCharEvent *e = (RLGlfwCharEvent *)RL_MALLOC(sizeof(RLGlfwCharEvent));
            RL_DIAG_PAYLOAD_ALLOC(RL_DIAG_PAYLOAD_CHAR, sizeof(RLGlfwCharEvent));
            e->codepoint = codepoint;
            RLGlfwRunOnRenderThread(ctx, RLGlfwTask_Char, e);
            return;
        }
    }
#endif
    if (!RLGlfwBindCallbackContext(window)) return;

    // NOTE: Registers any key down considering OS keyboard layout but
    // does not detect action events, those should be managed by user...
    // REF: https://github.com/glfw/glfw/issues/668#issuecomment-166794907
    // REF: https://www.glfw.org/docs/latest/input_guide.html#input_char

    // Check if there is space available in the queue
    if (CORE.Input.Keyboard.charPressedQueueCount < MAX_CHAR_PRESSED_QUEUE)
    {
        // Add character to the queue
        CORE.Input.Keyboard.charPressedQueue[CORE.Input.Keyboard.charPressedQueueCount] = codepoint;
        CORE.Input.Keyboard.charPressedQueueCount++;
    }
}

// GLFW3: Mouse button callback, runs on mouse button pressed
static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
#if defined(_WIN32)
    {
        RLContext *ctx = (RLContext *)glfwGetWindowUserPointer(window);
        PlatformData *pd = (ctx != NULL)? (PlatformData *)ctx->platformData : NULL;
        if ((pd != NULL) && pd->useEventThread)
        {
            if (pd->closing) return;
            RLSetCurrentContext(ctx);
            RLGlfwMouseButtonEvent *e = (RLGlfwMouseButtonEvent *)RL_MALLOC(sizeof(RLGlfwMouseButtonEvent));
            RL_DIAG_PAYLOAD_ALLOC(RL_DIAG_PAYLOAD_MOUSEBUTTON, sizeof(RLGlfwMouseButtonEvent));
            e->button = button; e->action = action; e->mods = mods;
            RLGlfwRunOnRenderThread(ctx, RLGlfwTask_MouseButton, e);
            return;
        }
    }
#endif

    if (!RLGlfwBindCallbackContext(window)) return;

    // WARNING: GLFW could only return GLFW_PRESS (1) or GLFW_RELEASE (0) for now,
    // but future releases may add more actions (i.e. GLFW_REPEAT)
    CORE.Input.Mouse.currentButtonState[button] = action;
    CORE.Input.Touch.currentTouchState[button] = action;

#if defined(SUPPORT_GESTURES_SYSTEM) && defined(SUPPORT_MOUSE_GESTURES)
    // Process mouse events as touches to be able to use mouse-gestures
    GestureEvent gestureEvent = { 0 };

    // Register touch actions
    if ((CORE.Input.Mouse.currentButtonState[button] == 1) && (CORE.Input.Mouse.previousButtonState[button] == 0)) gestureEvent.touchAction = TOUCH_ACTION_DOWN;
    else if ((CORE.Input.Mouse.currentButtonState[button] == 0) && (CORE.Input.Mouse.previousButtonState[button] == 1)) gestureEvent.touchAction = TOUCH_ACTION_UP;

    // NOTE: TOUCH_ACTION_MOVE event is registered in MouseCursorPosCallback()

    // Assign a pointer ID
    gestureEvent.pointId[0] = 0;

    // Register touch points count
    gestureEvent.pointCount = 1;

    // Register touch points position, only one point registered
    gestureEvent.position[0] = RLGetMousePosition();

    // Normalize gestureEvent.position[0] for CORE.Window.screen.width and CORE.Window.screen.height
    gestureEvent.position[0].x /= (float)RLGetScreenWidth();
    gestureEvent.position[0].y /= (float)RLGetScreenHeight();

    // Gesture data is sent to gestures-system for processing
    ProcessGestureEvent(gestureEvent);
#endif
}

// GLFW3: Cursor position callback, runs on mouse movement
static void MouseCursorPosCallback(GLFWwindow *window, double x, double y)
{
#if defined(_WIN32)
    {
        RLContext *ctx = (RLContext *)glfwGetWindowUserPointer(window);
        PlatformData *pd = (ctx != NULL)? (PlatformData *)ctx->platformData : NULL;
        if ((pd != NULL) && pd->useEventThread)
        {
            if (pd->closing) return;
#if RL_EVENTTHREAD_COALESCE_STATE
            RLSetCurrentContext(ctx);
            RLAtomicExchangeLong(&pd->pendingMouseXBits, RLFloatBitsFromFloat((float)x));
            RLAtomicExchangeLong(&pd->pendingMouseYBits, RLFloatBitsFromFloat((float)y));
            RLAtomicOrLong(&pd->pendingMask, RL_PENDING_MOUSE_MOVE);
            RLGlfwQueuePendingDrain(ctx, pd);
            return;
#else
            RLSetCurrentContext(ctx);
            RLGlfwMouseMoveEvent *e = (RLGlfwMouseMoveEvent *)RL_MALLOC(sizeof(RLGlfwMouseMoveEvent));
            RL_DIAG_PAYLOAD_ALLOC(RL_DIAG_PAYLOAD_MOUSEMOVE, sizeof(RLGlfwMouseMoveEvent));
            e->xpos = x; e->ypos = y;
            RLGlfwRunOnRenderThread(ctx, RLGlfwTask_MouseMove, e);
            return;
#endif
        }
    }
#endif

    if (!RLGlfwBindCallbackContext(window)) return;

    CORE.Input.Mouse.currentPosition.x = (float)x;
    CORE.Input.Mouse.currentPosition.y = (float)y;
    CORE.Input.Touch.position[0] = CORE.Input.Mouse.currentPosition;

#if defined(SUPPORT_GESTURES_SYSTEM) && defined(SUPPORT_MOUSE_GESTURES)
    // Process mouse events as touches to be able to use mouse-gestures
    GestureEvent gestureEvent = { 0 };

    gestureEvent.touchAction = TOUCH_ACTION_MOVE;

    // Assign a pointer ID
    gestureEvent.pointId[0] = 0;

    // Register touch points count
    gestureEvent.pointCount = 1;

    // Register touch points position, only one point registered
    gestureEvent.position[0] = CORE.Input.Touch.position[0];

    // Normalize gestureEvent.position[0] for CORE.Window.screen.width and CORE.Window.screen.height
    gestureEvent.position[0].x /= (float)RLGetScreenWidth();
    gestureEvent.position[0].y /= (float)RLGetScreenHeight();

    // Gesture data is sent to gestures-system for processing
    ProcessGestureEvent(gestureEvent);
#endif
}

// GLFW3: Mouse wheel scroll callback, runs on mouse wheel changes
static void MouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
#if defined(_WIN32)
    {
        RLContext *ctx = (RLContext *)glfwGetWindowUserPointer(window);
        PlatformData *pd = (ctx != NULL)? (PlatformData *)ctx->platformData : NULL;
        if ((pd != NULL) && pd->useEventThread)
        {
            if (pd->closing) return;
#if RL_EVENTTHREAD_COALESCE_STATE
            RLSetCurrentContext(ctx);
            RLAtomicAddLong(&pd->pendingWheelX_fp, RLWheelToFixed(xoffset));
            RLAtomicAddLong(&pd->pendingWheelY_fp, RLWheelToFixed(yoffset));
            RLAtomicOrLong(&pd->pendingMask, RL_PENDING_WHEEL);
            RLGlfwQueuePendingDrain(ctx, pd);
            return;
#else
            RLSetCurrentContext(ctx);
            RLGlfwMouseWheelEvent *e = (RLGlfwMouseWheelEvent *)RL_MALLOC(sizeof(RLGlfwMouseWheelEvent));
            RL_DIAG_PAYLOAD_ALLOC(RL_DIAG_PAYLOAD_MOUSEWHEEL, sizeof(RLGlfwMouseWheelEvent));
            e->xoffset = xoffset;
            e->yoffset = yoffset;
            RLGlfwRunOnRenderThread(ctx, RLGlfwTask_MouseWheel, e);
            return;
#endif
        }
    }
#endif

    if (!RLGlfwBindCallbackContext(window)) return;

    CORE.Input.Mouse.currentWheelMove = (RLVector2){ (float)xoffset, (float)yoffset };
}

// GLFW3: Cursor ennter callback, when cursor enters the window
static void CursorEnterCallback(GLFWwindow *window, int enter)
{
#if defined(_WIN32)
    {
        RLContext *ctx = (RLContext *)glfwGetWindowUserPointer(window);
        PlatformData *pd = (ctx != NULL)? (PlatformData *)ctx->platformData : NULL;
        if ((pd != NULL) && pd->useEventThread)
        {
            if (pd->closing) return;
            RLSetCurrentContext(ctx);
            RLGlfwCursorEnterEvent *e = (RLGlfwCursorEnterEvent *)RL_MALLOC(sizeof(RLGlfwCursorEnterEvent));
            RL_DIAG_PAYLOAD_ALLOC(RL_DIAG_PAYLOAD_OTHER, sizeof(RLGlfwCursorEnterEvent));
            e->entered = enter;
            RLGlfwRunOnRenderThread(ctx, RLGlfwTask_CursorEnter, e);
            return;
        }
    }
#endif

    if (!RLGlfwBindCallbackContext(window)) return;

    if (enter) CORE.Input.Mouse.cursorOnScreen = true;
    else CORE.Input.Mouse.cursorOnScreen = false;
}

// GLFW3: Joystick connected/disconnected callback
static void JoystickCallback(int jid, int event)
{
#if defined(_WIN32)
    RLContext *ctx0 = RLGetCurrentContext();
    PlatformData *pd0 = (ctx0 != NULL)? (PlatformData *)ctx0->platformData : NULL;
    if ((pd0 != NULL) && pd0->useEventThread)
    {
        if (pd0->closing) return;
        RLContext *ctx = ctx0;
        if (ctx != NULL)
        {
            RLGlfwJoystickEvent *e = (RLGlfwJoystickEvent *)RL_MALLOC(sizeof(RLGlfwJoystickEvent));
            RL_DIAG_PAYLOAD_ALLOC(RL_DIAG_PAYLOAD_OTHER, sizeof(RLGlfwJoystickEvent));
            memset(e, 0, sizeof(RLGlfwJoystickEvent));
            e->jid = jid;
            e->event = event;

            const char *name = glfwGetJoystickName(jid);
            if (name != NULL)
            {
                e->name = (char *)RL_MALLOC(MAX_GAMEPAD_NAME_LENGTH);
                memset(e->name, 0, MAX_GAMEPAD_NAME_LENGTH);
                strncpy(e->name, name, MAX_GAMEPAD_NAME_LENGTH - 1);
            }

            RLGlfwRunOnRenderThread(ctx, RLGlfwTask_Joystick, e);
        }
        return;
    }
#endif

    if (event == GLFW_CONNECTED)
    {
        // WARNING: If glfwGetJoystickName() is longer than MAX_GAMEPAD_NAME_LENGTH,
        // only copy up to (MAX_GAMEPAD_NAME_LENGTH -1) to destination string
        memset(CORE.Input.Gamepad.name[jid], 0, MAX_GAMEPAD_NAME_LENGTH);
        strncpy(CORE.Input.Gamepad.name[jid], glfwGetJoystickName(jid), MAX_GAMEPAD_NAME_LENGTH - 1);
    }
    else if (event == GLFW_DISCONNECTED)
    {
        memset(CORE.Input.Gamepad.name[jid], 0, MAX_GAMEPAD_NAME_LENGTH);
    }
}

#if defined(_WIN32)
//----------------------------------------------------------------------------------
// Win32: event-thread mode support
//----------------------------------------------------------------------------------

static void RLGlfwTask_WindowPos(void *user)
{
    RLGlfwPosI2 *e = (RLGlfwPosI2 *)user;
    if (e == NULL) return;
    CORE.Window.position.x = e->x;
    CORE.Window.position.y = e->y;
    RL_DIAG_PAYLOAD_FREE(RL_DIAG_PAYLOAD_WINPOS, sizeof(RLGlfwPosI2));
    RL_FREE(e);
}

#if RL_EVENTTHREAD_COALESCE_STATE
// Drain coalesced pending input/state for Win32 event-thread mode.
// This runs on the render thread (via RLGlfwRunOnRenderThread).
static void RLGlfwTask_DrainPendingInput(void *user)
{
    PlatformData *pd = (PlatformData *)user;
    if (pd == NULL) return;

    // If shutting down, drop any pending state and exit.
    if (pd->closing)
    {
        RLAtomicExchangeLong(&pd->pendingMask, 0);
        RLAtomicExchangeLong(&pd->pendingWheelX_fp, 0);
        RLAtomicExchangeLong(&pd->pendingWheelY_fp, 0);
        RLAtomicExchangeLong(&pd->pendingQueued, 0);
        return;
    }

    for (;;)
    {
        long mask = RLAtomicExchangeLong(&pd->pendingMask, 0);

        if (mask & RL_PENDING_SCALE)
        {
            const float scalex = RLFloatFromBits(RLAtomicLoadLong(&pd->pendingScaleXBits));
            const float scaley = RLFloatFromBits(RLAtomicLoadLong(&pd->pendingScaleYBits));

            float fbWidth = (float)CORE.Window.screen.width*scalex;
            float fbHeight = (float)CORE.Window.screen.height*scaley;

            CORE.Window.screenScale = MatrixScale(scalex, scaley, 1.0f);
#if !defined(__APPLE__)
            RLSetMouseScale(1.0f/scalex, 1.0f/scaley);
#endif
            CORE.Window.render.width = (int)fbWidth;
            CORE.Window.render.height = (int)fbHeight;
            CORE.Window.currentFbo = CORE.Window.render;
        }

        if (mask & RL_PENDING_FB_SIZE)
        {
            const int width = (int)RLAtomicLoadLong(&pd->pendingFbW);
            const int height = (int)RLAtomicLoadLong(&pd->pendingFbH);

            if ((width != 0) && (height != 0))
            {
                // Reset viewport and projection matrix for new size
                SetupViewport(width, height);

                // Set render size
                CORE.Window.currentFbo.width = width;
                CORE.Window.currentFbo.height = height;
                CORE.Window.resizedLastFrame = true;

                if (FLAG_IS_SET(CORE.Window.flags, FLAG_FULLSCREEN_MODE))
                {
                    CORE.Window.screen.width = width;
                    CORE.Window.screen.height = height;
                    CORE.Window.screenScale = MatrixScale(1.0f, 1.0f, 1.0f);
                    RLSetMouseScale(1.0f, 1.0f);
                }
                else
                {
                    if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_HIGHDPI))
                    {
                        RLVector2 scaleDpi = RLGetWindowScaleDPI();
                        CORE.Window.screen.width = (int)((float)width/scaleDpi.x);
                        CORE.Window.screen.height = (int)((float)height/scaleDpi.y);
                        CORE.Window.screenScale = MatrixScale(scaleDpi.x, scaleDpi.y, 1.0f);
#if !defined(__APPLE__)
                        RLSetMouseScale(1.0f/scaleDpi.x, 1.0f/scaleDpi.y);
#endif
                    }
                    else
                    {
                        CORE.Window.screen.width = width;
                        CORE.Window.screen.height = height;
                    }
                }
            }
        }

        if (mask & RL_PENDING_WIN_POS)
        {
            CORE.Window.position.x = (int)RLAtomicLoadLong(&pd->pendingWinX);
            CORE.Window.position.y = (int)RLAtomicLoadLong(&pd->pendingWinY);
        }

        if (mask & RL_PENDING_MOUSE_MOVE)
        {
            const float x = RLFloatFromBits(RLAtomicLoadLong(&pd->pendingMouseXBits));
            const float y = RLFloatFromBits(RLAtomicLoadLong(&pd->pendingMouseYBits));

            CORE.Input.Mouse.currentPosition.x = x;
            CORE.Input.Mouse.currentPosition.y = y;
            CORE.Input.Touch.position[0].x = x;
            CORE.Input.Touch.position[0].y = y;
        }

        if (mask & RL_PENDING_WHEEL)
        {
            const long dx_fp = RLAtomicExchangeLong(&pd->pendingWheelX_fp, 0);
            const long dy_fp = RLAtomicExchangeLong(&pd->pendingWheelY_fp, 0);

            // Accumulate all wheel steps that happened before this drain task executed.
            CORE.Input.Mouse.currentWheelMove.x += (float)dx_fp/(float)RL_WHEEL_FP_SCALE;
            CORE.Input.Mouse.currentWheelMove.y += (float)dy_fp/(float)RL_WHEEL_FP_SCALE;
        }

        // Mark this drain task as complete (allow another to be queued).
        RLAtomicExchangeLong(&pd->pendingQueued, 0);

        // If more pending events arrived while draining, try to continue in this same task.
        if (RLAtomicLoadLong(&pd->pendingMask) == 0) break;
        if (!RLAtomicCASLong(&pd->pendingQueued, 0, 1)) break;
    }
}
#endif // RL_EVENTTHREAD_COALESCE_STATE

static void RLGlfwTask_FramebufferSize(void *user)
{
    RLGlfwSizeI2 *e = (RLGlfwSizeI2 *)user;
    if (e == NULL) return;

    const int width = e->w;
    const int height = e->h;
    RL_DIAG_PAYLOAD_FREE(RL_DIAG_PAYLOAD_FBSIZE, sizeof(RLGlfwSizeI2));
    RL_FREE(e);

    if ((width == 0) || (height == 0)) return;

    // Reset viewport and projection matrix for new size
    SetupViewport(width, height);

    // Set render size
    CORE.Window.currentFbo.width = width;
    CORE.Window.currentFbo.height = height;
    CORE.Window.resizedLastFrame = true;

    if (FLAG_IS_SET(CORE.Window.flags, FLAG_FULLSCREEN_MODE))
    {
        CORE.Window.screen.width = width;
        CORE.Window.screen.height = height;
        CORE.Window.screenScale = MatrixScale(1.0f, 1.0f, 1.0f);
        RLSetMouseScale(1.0f, 1.0f);
    }
    else
    {
        if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_HIGHDPI))
        {
            RLVector2 scaleDpi = RLGetWindowScaleDPI();
            CORE.Window.screen.width = (int)((float)width/scaleDpi.x);
            CORE.Window.screen.height = (int)((float)height/scaleDpi.y);
            CORE.Window.screenScale = MatrixScale(scaleDpi.x, scaleDpi.y, 1.0f);
#if !defined(__APPLE__)
            RLSetMouseScale(1.0f/scaleDpi.x, 1.0f/scaleDpi.y);
#endif
        }
        else
        {
            CORE.Window.screen.width = width;
            CORE.Window.screen.height = height;
        }
    }
}

static void RLGlfwTask_WindowContentScale(void *user)
{
    RLGlfwWindowScaleEvent *e = (RLGlfwWindowScaleEvent *)user;
    if (e == NULL) return;

    const float scalex = e->sx;
    const float scaley = e->sy;
    RL_DIAG_PAYLOAD_FREE(RL_DIAG_PAYLOAD_SCALE, sizeof(RLGlfwWindowScaleEvent));
    RL_FREE(e);

    float fbWidth = (float)CORE.Window.screen.width*scalex;
    float fbHeight = (float)CORE.Window.screen.height*scaley;

    CORE.Window.screenScale = MatrixScale(scalex, scaley, 1.0f);
#if !defined(__APPLE__)
    RLSetMouseScale(1.0f/scalex, 1.0f/scaley);
#endif

    CORE.Window.render.width = (int)fbWidth;
    CORE.Window.render.height = (int)fbHeight;
    CORE.Window.currentFbo = CORE.Window.render;
}

static void RLGlfwTask_WindowIconify(void *user)
{
    RLGlfwWindowIconifyEvent *e = (RLGlfwWindowIconifyEvent *)user;
    if (e == NULL) return;
    if (e->iconified) FLAG_SET(CORE.Window.flags, FLAG_WINDOW_MINIMIZED);
    else FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_MINIMIZED);
    RL_DIAG_PAYLOAD_FREE(RL_DIAG_PAYLOAD_OTHER, sizeof(RLGlfwWindowIconifyEvent));
    RL_FREE(e);
}

static void RLGlfwTask_WindowMaximize(void *user)
{
    RLGlfwWindowMaximizeEvent *e = (RLGlfwWindowMaximizeEvent *)user;
    if (e == NULL) return;
    if (e->maximized) FLAG_SET(CORE.Window.flags, FLAG_WINDOW_MAXIMIZED);
    else FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_MAXIMIZED);
    RL_DIAG_PAYLOAD_FREE(RL_DIAG_PAYLOAD_OTHER, sizeof(RLGlfwWindowMaximizeEvent));
    RL_FREE(e);
}

static void RLGlfwTask_WindowFocus(void *user)
{
    RLGlfwWindowFocusEvent *e = (RLGlfwWindowFocusEvent *)user;
    if (e == NULL) return;
    if (e->focused) FLAG_CLEAR(CORE.Window.flags, FLAG_WINDOW_UNFOCUSED);
    else FLAG_SET(CORE.Window.flags, FLAG_WINDOW_UNFOCUSED);
    RL_DIAG_PAYLOAD_FREE(RL_DIAG_PAYLOAD_OTHER, sizeof(RLGlfwWindowFocusEvent));
    RL_FREE(e);
}

static void RLGlfwTask_WindowRefresh(void *user)
{
    (void)user;
    // In event-thread mode, this task must run on the render thread.
    if (platform.useEventThread)
    {
        RLGLFW_ASSERT(platform.renderThread != NULL);
        RLGLFW_ASSERT(RLGlfwIsThread(platform.renderThread));
    }
    RLGlfwInvokeUserWindowRefresh(false);
}

static void RLGlfwTask_WindowClose(void *user)
{
    RLGlfwWindowCloseEvent *e = (RLGlfwWindowCloseEvent *)user;
    if (e != NULL) { RL_DIAG_PAYLOAD_FREE(RL_DIAG_PAYLOAD_WINCLOSE, sizeof(RLGlfwWindowCloseEvent)); RL_FREE(e); }
    CORE.Window.shouldClose = true;
}

static void RLGlfwTask_Drop(void *user)
{
    RLGlfwDropEvent *e = (RLGlfwDropEvent *)user;
    if (e == NULL) return;

    if (e->count > 0)
    {
        // In case previous dropped filepaths have not been freed, free them
        if (CORE.Window.dropFileCount > 0)
        {
            for (unsigned int i = 0; i < CORE.Window.dropFileCount; i++) RL_FREE(CORE.Window.dropFilepaths[i]);
            RL_FREE(CORE.Window.dropFilepaths);
            CORE.Window.dropFileCount = 0;
            CORE.Window.dropFilepaths = NULL;
        }

        CORE.Window.dropFileCount = (unsigned int)e->count;
        CORE.Window.dropFilepaths = e->paths; // ownership transferred
        e->paths = NULL;
    }

    // Free envelope only (strings are now owned by CORE)
    RL_DIAG_PAYLOAD_FREE(RL_DIAG_PAYLOAD_DROP, sizeof(RLGlfwDropEvent));
    RL_FREE(e);
}

static void RLGlfwTask_Key(void *user)
{
    RLGlfwKeyEvent *e = (RLGlfwKeyEvent *)user;
    if (e == NULL) return;

    int key = e->key;
    const int scancode = e->scancode;
    const int action = e->action;
    const int mods = e->mods;
    RL_DIAG_PAYLOAD_FREE(RL_DIAG_PAYLOAD_KEY, sizeof(RLGlfwKeyEvent));
    RL_FREE(e);

    if (key == GLFW_KEY_UNKNOWN) return;

    // WARNING: GLFW could return GLFW_REPEAT, we need to consider it as a key down event
    if ((action == GLFW_PRESS) || (action == GLFW_REPEAT))
    {
        CORE.Input.Keyboard.currentKeyState[key] = 1;
        CORE.Input.Keyboard.keyRepeatInFrame[key] = (action == GLFW_REPEAT);

        // WARNING: Check if CAPS/NUM lock modifiers are enabled and force down state for those keys
        if (((key == KEY_CAPS_LOCK) && (FLAG_IS_SET(mods, GLFW_MOD_CAPS_LOCK))) ||
            ((key == KEY_NUM_LOCK) && (FLAG_IS_SET(mods, GLFW_MOD_NUM_LOCK)))) CORE.Input.Keyboard.currentKeyState[key] = 1;

        // Check if there is space available in the key queue (only on initial press)
        if ((action == GLFW_PRESS) && (CORE.Input.Keyboard.keyPressedQueueCount < MAX_KEY_PRESSED_QUEUE))
        {
            CORE.Input.Keyboard.keyPressedQueue[CORE.Input.Keyboard.keyPressedQueueCount] = key;
            CORE.Input.Keyboard.keyPressedQueueCount++;
        }
    }
    else if (action == GLFW_RELEASE)
    {
        CORE.Input.Keyboard.currentKeyState[key] = 0;
        CORE.Input.Keyboard.keyRepeatInFrame[key] = 0;
    }

    // Exit on configured exit key
    if ((key == CORE.Input.Keyboard.exitKey) && (action == GLFW_PRESS))
    {
        CORE.Window.shouldClose = true;

#if defined(_WIN32)
        if (platform.useEventThread) RLGlfwRunOnEventThread(RLGlfwTask_SetWindowShouldCloseTrue, NULL, true);
#endif
    }

    // NOTE: We intentionally avoid calling glfwGetKeyName() here in event-thread mode.
}

static void RLGlfwTask_Char(void *user)
{
    RLGlfwCharEvent *e = (RLGlfwCharEvent *)user;
    if (e == NULL) return;
    const unsigned int codepoint = e->codepoint;
    RL_DIAG_PAYLOAD_FREE(RL_DIAG_PAYLOAD_CHAR, sizeof(RLGlfwCharEvent));
    RL_FREE(e);

    if (CORE.Input.Keyboard.charPressedQueueCount < MAX_CHAR_PRESSED_QUEUE)
    {
        CORE.Input.Keyboard.charPressedQueue[CORE.Input.Keyboard.charPressedQueueCount] = codepoint;
        CORE.Input.Keyboard.charPressedQueueCount++;
    }
}

static void RLGlfwTask_MouseButton(void *user)
{
    RLGlfwMouseButtonEvent *e = (RLGlfwMouseButtonEvent *)user;
    if (e == NULL) return;
    const int button = e->button;
    const int action = e->action;
    const int mods = e->mods;
    RL_DIAG_PAYLOAD_FREE(RL_DIAG_PAYLOAD_MOUSEBUTTON, sizeof(RLGlfwMouseButtonEvent));
    RL_FREE(e);

    if (button >= 0)
    {
        if (action == GLFW_PRESS)
        {
            CORE.Input.Mouse.currentButtonState[button] = 1;
            CORE.Input.Touch.currentTouchState[button] = 1;

            if ((button == GLFW_MOUSE_BUTTON_LEFT) && (mods == GLFW_MOD_SUPER))
            {
                // NOTE: For macOS, control key is treated as super key for the right click emulation
                CORE.Input.Mouse.currentButtonState[GLFW_MOUSE_BUTTON_RIGHT] = 1;
            }
        }
        else if (action == GLFW_RELEASE)
        {
            CORE.Input.Mouse.currentButtonState[button] = 0;
            CORE.Input.Touch.currentTouchState[button] = 0;

            if ((button == GLFW_MOUSE_BUTTON_LEFT) && (mods == GLFW_MOD_SUPER))
            {
                CORE.Input.Mouse.currentButtonState[GLFW_MOUSE_BUTTON_RIGHT] = 0;
            }
        }
    }
}

static void RLGlfwTask_MouseMove(void *user)
{
    RLGlfwMouseMoveEvent *e = (RLGlfwMouseMoveEvent *)user;
    if (e == NULL) return;
    const double xpos = e->xpos;
    const double ypos = e->ypos;
    RL_DIAG_PAYLOAD_FREE(RL_DIAG_PAYLOAD_MOUSEMOVE, sizeof(RLGlfwMouseMoveEvent));
    RL_FREE(e);

    CORE.Input.Mouse.currentPosition.x = (float)xpos;
    CORE.Input.Mouse.currentPosition.y = (float)ypos;

    CORE.Input.Touch.position[0].x = (float)xpos;
    CORE.Input.Touch.position[0].y = (float)ypos;
}

static void RLGlfwTask_MouseWheel(void *user)
{
    RLGlfwMouseWheelEvent *e = (RLGlfwMouseWheelEvent *)user;
    if (e == NULL) return;
    const double xoffset = e->xoffset;
    const double yoffset = e->yoffset;
    RL_DIAG_PAYLOAD_FREE(RL_DIAG_PAYLOAD_MOUSEWHEEL, sizeof(RLGlfwMouseWheelEvent));
    RL_FREE(e);

    // WARNING: GLFW could return both X and Y offset values for a mouse wheel event
    CORE.Input.Mouse.currentWheelMove.x = (float)xoffset;
    CORE.Input.Mouse.currentWheelMove.y = (float)yoffset;
}

static void RLGlfwTask_CursorEnter(void *user)
{
    RLGlfwCursorEnterEvent *e = (RLGlfwCursorEnterEvent *)user;
    if (e == NULL) return;
    const int entered = e->entered;
    RL_DIAG_PAYLOAD_FREE(RL_DIAG_PAYLOAD_OTHER, sizeof(RLGlfwCursorEnterEvent));
    RL_FREE(e);

    CORE.Input.Mouse.cursorOnScreen = (entered != 0);
}

static void RLGlfwTask_Joystick(void *user)
{
    RLGlfwJoystickEvent *e = (RLGlfwJoystickEvent *)user;
    if (e == NULL) return;

    const int jid = e->jid;
    const int event = e->event;

    if (event == GLFW_CONNECTED)
    {
        memset(CORE.Input.Gamepad.name[jid], 0, MAX_GAMEPAD_NAME_LENGTH);
        if (e->name != NULL) strncpy(CORE.Input.Gamepad.name[jid], e->name, MAX_GAMEPAD_NAME_LENGTH - 1);
        else
        {
            const char *name = glfwGetJoystickName(jid);
            if (name != NULL) strncpy(CORE.Input.Gamepad.name[jid], name, MAX_GAMEPAD_NAME_LENGTH - 1);
        }
    }
    else if (event == GLFW_DISCONNECTED)
    {
        memset(CORE.Input.Gamepad.name[jid], 0, MAX_GAMEPAD_NAME_LENGTH);
    }

    if (e->name != NULL) RL_FREE(e->name);
    RL_DIAG_PAYLOAD_FREE(RL_DIAG_PAYLOAD_OTHER, sizeof(RLGlfwJoystickEvent));
    RL_FREE(e);
}

// Runs on event thread: destroy GLFW window on owning Win32 message thread.
static void RLGlfwTask_DestroyWindow(void *user)
{
    (void)user;
    if (platform.handle != NULL)
    {
        // Disarm per-window callbacks first so no further render-thread tasks are enqueued.
        glfwSetWindowUserPointer(platform.handle, NULL);
        glfwSetWindowSizeCallback(platform.handle, NULL);
        glfwSetFramebufferSizeCallback(platform.handle, NULL);
        glfwSetWindowPosCallback(platform.handle, NULL);
        glfwSetWindowMaximizeCallback(platform.handle, NULL);
        glfwSetWindowIconifyCallback(platform.handle, NULL);
        glfwSetWindowFocusCallback(platform.handle, NULL);
        glfwSetWindowRefreshCallback(platform.handle, NULL);
        glfwSetWindowCloseCallback(platform.handle, NULL);
        glfwSetDropCallback(platform.handle, NULL);
        glfwSetWindowContentScaleCallback(platform.handle, NULL);
        glfwSetKeyCallback(platform.handle, NULL);
        glfwSetCharCallback(platform.handle, NULL);
        glfwSetMouseButtonCallback(platform.handle, NULL);
        glfwSetCursorPosCallback(platform.handle, NULL);
        glfwSetScrollCallback(platform.handle, NULL);
        glfwSetCursorEnterCallback(platform.handle, NULL);

        glfwDestroyWindow(platform.handle);
        platform.handle = NULL;
    }

    // Make sure the render thread(s) unblock and can observe destruction/close.
    RLGlfwSignalWakeByPolicy(&platform, true);
}

// Runs on event thread: mark GLFW close flag.
static void RLGlfwTask_SetWindowShouldCloseTrue(void *user)
{
    (void)user;
    if (platform.handle != NULL) glfwSetWindowShouldClose(platform.handle, GLFW_TRUE);

    // Ensure the waiting render thread(s) can observe the close request.
    RLGlfwSignalWakeByPolicy(&platform, true);
}

// Runs on event thread: window-affine operations used by thread-aware wrappers.
static void RLGlfwTask_SetWindowPos(void *user)
{
    const int *xy = (const int *)user;
    if ((platform.handle != NULL) && (xy != NULL)) glfwSetWindowPos(platform.handle, xy[0], xy[1]);
}

static void RLGlfwTask_SetWindowSize(void *user)
{
    const int *wh = (const int *)user;
    if ((platform.handle != NULL) && (wh != NULL)) glfwSetWindowSize(platform.handle, wh[0], wh[1]);
}

static void RLGlfwTask_SetWindowTitle(void *user)
{
    const char *title = (const char *)user;
    if ((platform.handle != NULL) && (title != NULL)) glfwSetWindowTitle(platform.handle, title);
}

static void RLGlfwTask_SetWindowAttrib(void *user)
{
    const int *av = (const int *)user;
    if ((platform.handle != NULL) && (av != NULL)) glfwSetWindowAttrib(platform.handle, av[0], av[1]);
}

// Toggle GLFW refresh callback (thread-affine in Win32 event-thread mode)
static void RLGlfwTask_SetWindowRefreshCallback(void *user)
{
    const int *enable = (const int *)user;
    if ((platform.handle == NULL) || (enable == NULL)) return;
    glfwSetWindowRefreshCallback(platform.handle, (*enable) ? WindowRefreshCallback : NULL);
}

static void RLGlfwTask_SetWindowSizeLimits(void *user)
{
    const int *lim = (const int *)user;
    if ((platform.handle != NULL) && (lim != NULL)) glfwSetWindowSizeLimits(platform.handle, lim[0], lim[1], lim[2], lim[3]);
}

static void RLGlfwTask_SetWindowOpacity(void *user)
{
    const float *op = (const float *)user;
    if ((platform.handle != NULL) && (op != NULL)) glfwSetWindowOpacity(platform.handle, *op);
}

static void RLGlfwTask_SetWindowMonitor(void *user)
{
    const RLGlfwMonitorTask *t = (const RLGlfwMonitorTask *)user;
    if ((platform.handle == NULL) || (t == NULL)) return;
    glfwSetWindowMonitor(platform.handle, t->monitor, t->xpos, t->ypos, t->width, t->height, t->refreshRate);
}

static void RLGlfwTask_ShowWindow(void *user)
{
    (void)user;
    if (platform.handle != NULL) glfwShowWindow(platform.handle);
}

static void RLGlfwTask_HideWindow(void *user)
{
    (void)user;
    if (platform.handle != NULL) glfwHideWindow(platform.handle);
}

static void RLGlfwTask_FocusWindow(void *user)
{
    (void)user;
    if (platform.handle != NULL) glfwFocusWindow(platform.handle);
}

static void RLGlfwTask_IconifyWindow(void *user)
{
    (void)user;
    if (platform.handle != NULL) glfwIconifyWindow(platform.handle);
}

static void RLGlfwTask_MaximizeWindow(void *user)
{
    (void)user;
    if (platform.handle != NULL) glfwMaximizeWindow(platform.handle);
}

static void RLGlfwTask_RestoreWindow(void *user)
{
    (void)user;
    if (platform.handle != NULL) glfwRestoreWindow(platform.handle);
}

static void RLGlfwTask_SetWindowIcon(void *user)
{
    const RLGlfwIconTask *t = (const RLGlfwIconTask *)user;
    if ((platform.handle == NULL) || (t == NULL)) return;
    glfwSetWindowIcon(platform.handle, t->count, t->icons);
}

static void RLGlfwTask_QueryMonitorCount(void *user)
{
    int *outCount = (int *)user;
    if (outCount == NULL) return;
    int monitorCount = 0;
    glfwGetMonitors(&monitorCount);
    *outCount = monitorCount;
}

static void RLGlfwTask_QueryMonitorInfo(void *user)
{
    RLGlfwMonitorInfo *out = (RLGlfwMonitorInfo *)user;
    if (out == NULL) return;

    int monitorCount = 0;
    GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);
    out->monitorCount = monitorCount;
    out->ok = 0;
    out->monitor = NULL;
    out->name = "";

    if ((monitors == NULL) || (out->index < 0) || (out->index >= monitorCount)) return;

    GLFWmonitor *m = monitors[out->index];
    out->monitor = m;
    out->name = (m != NULL)? glfwGetMonitorName(m) : "";

    out->posX = out->posY = 0;
    out->workX = out->workY = out->workW = out->workH = 0;
    out->modeW = out->modeH = 0;
    out->refresh = 0;
    out->physW = out->physH = 0;

    if (m != NULL)
    {
        glfwGetMonitorPos(m, &out->posX, &out->posY);
        glfwGetMonitorWorkarea(m, &out->workX, &out->workY, &out->workW, &out->workH);
        glfwGetMonitorPhysicalSize(m, &out->physW, &out->physH);
        const GLFWvidmode *mode = glfwGetVideoMode(m);
        if (mode != NULL)
        {
            out->modeW = mode->width;
            out->modeH = mode->height;
            out->refresh = mode->refreshRate;
        }

        out->ok = 1;
    }
}

static void RLGlfwTask_QueryCurrentMonitorIndex(void *user)
{
    int *outIndex = (int *)user;
    if (outIndex == NULL) return;

    int index = 0;
    int monitorCount = 0;
    GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);
    if ((monitors == NULL) || (monitorCount <= 0) || (platform.handle == NULL))
    {
        *outIndex = 0;
        return;
    }

    // If fullscreen, match window monitor.
    GLFWmonitor *wm = glfwGetWindowMonitor(platform.handle);
    if (wm != NULL)
    {
        for (int i = 0; i < monitorCount; i++)
        {
            if (monitors[i] == wm) { index = i; break; }
        }
        *outIndex = index;
        return;
    }

    // Window center position.
    int wx = 0, wy = 0, ww = 0, wh = 0;
    glfwGetWindowPos(platform.handle, &wx, &wy);
    glfwGetWindowSize(platform.handle, &ww, &wh);
    const int wcx = wx + (ww/2);
    const int wcy = wy + (wh/2);

    int closestDist = 0x7FFFFFFF;
    for (int i = 0; i < monitorCount; i++)
    {
        GLFWmonitor *m = monitors[i];
        int mx = 0, my = 0;
        glfwGetMonitorPos(m, &mx, &my);
        const GLFWvidmode *mode = glfwGetVideoMode(m);
        if (mode == NULL) continue;

        const int right = mx + mode->width - 1;
        const int bottom = my + mode->height - 1;
        if ((wcx >= mx) && (wcx <= right) && (wcy >= my) && (wcy <= bottom)) { index = i; break; }

        int xclosest = wcx;
        if (wcx < mx) xclosest = mx;
        else if (wcx > right) xclosest = right;
        int yclosest = wcy;
        if (wcy < my) yclosest = my;
        else if (wcy > bottom) yclosest = bottom;

        int dx = wcx - xclosest;
        int dy = wcy - yclosest;
        int dist = (dx*dx) + (dy*dy);
        if (dist < closestDist) { closestDist = dist; index = i; }
    }

    *outIndex = index;
}

static void RLGlfwTask_SetClipboardText(void *user)
{
    const char *text = (const char *)user;
    if (platform.handle != NULL) glfwSetClipboardString(platform.handle, (text != NULL)? text : "");
}

static void RLGlfwTask_GetClipboardText(void *user)
{
    RLGlfwClipboardGetTask *t = (RLGlfwClipboardGetTask *)user;
    if (t == NULL) return;
    t->out = (platform.handle != NULL)? glfwGetClipboardString(platform.handle) : NULL;
}

static void RLGlfwTask_GetWindowContentScale(void *user)
{
    RLGlfwContentScaleTask *t = (RLGlfwContentScaleTask *)user;
    if (t == NULL) return;
    t->x = 1.0f;
    t->y = 1.0f;
    if (platform.handle != NULL) glfwGetWindowContentScale(platform.handle, &t->x, &t->y);
}

// Win32 message thread entry for event-thread mode.
static void RLGlfwEventThreadMain(void *p)
{
    RLGlfwEventThreadStart *start = (RLGlfwEventThreadStart *)p;
    RLContext *ctx = (start != NULL)? start->ctx : NULL;
    if (start != NULL) { RL_DIAG_PAYLOAD_FREE(RL_DIAG_PAYLOAD_OTHER, sizeof(RLGlfwEventThreadStart)); RL_FREE(start); }
    if (ctx == NULL) return;

    RLSetCurrentContext(ctx);

    // Capture GLFW thread handle for this message thread.
    platform.eventThread = glfwGetCurrentThread();

    // Create window on the Win32 message thread.
    GLFWmonitor *monitor = NULL;
    if (FLAG_IS_SET(CORE.Window.flags, FLAG_FULLSCREEN_MODE))
    {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = (monitor != NULL)? glfwGetVideoMode(monitor) : NULL;
        if (mode != NULL)
        {
            CORE.Window.display.width = mode->width;
            CORE.Window.display.height = mode->height;
            if (CORE.Window.screen.width == 0) CORE.Window.screen.width = CORE.Window.display.width;
            if (CORE.Window.screen.height == 0) CORE.Window.screen.height = CORE.Window.display.height;
        }
    }
    else
    {
        GLFWmonitor *pm = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = (pm != NULL)? glfwGetVideoMode(pm) : NULL;
        if (mode != NULL)
        {
            CORE.Window.display.width = mode->width;
            CORE.Window.display.height = mode->height;
        }
    }

    GLFWwindow *shareWindow = RLGlfwResolveShareWindowForContext(ctx);

    platform.handle = glfwCreateWindow(CORE.Window.screen.width, CORE.Window.screen.height, CORE.Window.title, monitor, shareWindow);

    if (platform.handle != NULL)
    {
        glfwSetWindowUserPointer(platform.handle, ctx);

        // Cache HWND for cross-thread management APIs.
        platform.win32Hwnd = (HWND)glfwGetWin32Window(platform.handle);

        // Register callbacks on the message thread.
        glfwSetWindowSizeCallback(platform.handle, WindowSizeCallback);
        glfwSetFramebufferSizeCallback(platform.handle, FramebufferSizeCallback);
        glfwSetWindowPosCallback(platform.handle, WindowPosCallback);
        glfwSetWindowMaximizeCallback(platform.handle, WindowMaximizeCallback);
        // In event-thread mode, the window refresh callback is optional and controlled by
        // FLAG_WINDOW_REFRESH_CALLBACK. When disabled, we still keep the event-thread mode
        // semantics (render thread runs normally) without injecting user refresh draws.
        if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_REFRESH_CALLBACK))
            glfwSetWindowRefreshCallback(platform.handle, WindowRefreshCallback);
        glfwSetWindowCloseCallback(platform.handle, WindowCloseCallback);
        glfwSetWindowIconifyCallback(platform.handle, WindowIconifyCallback);
        glfwSetWindowFocusCallback(platform.handle, WindowFocusCallback);
        glfwSetDropCallback(platform.handle, WindowDropCallback);
        if (FLAG_IS_SET(CORE.Window.flags, FLAG_WINDOW_HIGHDPI)) glfwSetWindowContentScaleCallback(platform.handle, WindowContentScaleCallback);

        glfwSetKeyCallback(platform.handle, KeyCallback);
        glfwSetCharCallback(platform.handle, CharCallback);
        glfwSetMouseButtonCallback(platform.handle, MouseButtonCallback);
        glfwSetCursorPosCallback(platform.handle, MouseCursorPosCallback);
        glfwSetScrollCallback(platform.handle, MouseScrollCallback);
        glfwSetCursorEnterCallback(platform.handle, CursorEnterCallback);
        glfwSetJoystickCallback(JoystickCallback);
        glfwSetInputMode(platform.handle, GLFW_LOCK_KEY_MODS, GLFW_TRUE);
    }

    // Wake render thread waiting for window creation.
    if (platform.createdEvent != NULL) RLEventSignal(platform.createdEvent);
    RLGlfwWakeRenderThread();

    // If creation failed, nothing else to do.
    if (platform.handle == NULL) return;

    // Main message loop: wait + pump posted tasks. Use a small timeout to ensure tasks drain.
    while (!platform.eventThreadStop)
    {
        glfwWaitEventsTimeout(0.05);
        RLGlfwPumpThreadTasksWithDiag();
    }

    // Ensure any remaining tasks are drained.
    RLGlfwPumpThreadTasksWithDiag();
}

#endif // defined(_WIN32)

#ifdef _WIN32
#   define WIN32_CLIPBOARD_IMPLEMENTATION
#   include "../external/win32_clipboard.h"
#endif
// EOF
