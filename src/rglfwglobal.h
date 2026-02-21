#pragma once

// NOTE: This header provides a tiny C ABI for managing GLFW global state
// in multi-thread/multi-window scenarios.
//
// Route2 Stage-A constraints:
//  - One window per thread.
//  - We still allow multiple windows in one process.
//
// GLFW caveat:
//  - glfwInit/glfwTerminate are process-global.
//  - glfwPollEvents is a global event pump and is not safe to run concurrently.
//  - Many platforms expect event processing on a single (often main) thread.

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// Acquire the global GLFW runtime (ref-counted). The first acquire initializes GLFW.
// Returns false if glfwInit fails.
bool RLGlfwGlobalAcquire(void);

// Release the global GLFW runtime (ref-counted). The last release terminates GLFW.
void RLGlfwGlobalRelease(void);

// Serialize any GLFW global operations (event pump, create/destroy, etc.).
void RLGlfwGlobalLock(void);
void RLGlfwGlobalUnlock(void);

// Event pump ownership helper.
// By default, the thread that successfully performs the first initialization
// becomes the event-pump thread.
bool RLGlfwIsEventPumpThread(void);
void RLGlfwSetEventPumpThreadToCurrent(void);

// ---------------------------------------------------------------------------------
// Thread/sync primitives (C ABI)
// ---------------------------------------------------------------------------------
// These are intentionally tiny helpers to allow Win32 event-thread separation
// from C code without introducing platform-specific headers here.

typedef struct RLThread RLThread;
typedef struct RLMutex  RLMutex;
typedef struct RLEvent  RLEvent;

typedef void (*RLThreadFn)(void* user);

// Best-effort thread naming (primarily for debugging). This only affects the
// current thread and is a no-op on unsupported platforms.
void RLThreadSetNameCurrent(const char* nameUtf8);

// Thread
// NOTE: RLThreadCreateNamed is preferred for internal threads to provide a
// stable, descriptive name in debuggers and diagnostics.
RLThread* RLThreadCreateNamed(RLThreadFn fn, void* user, const char* nameUtf8);
RLThread* RLThreadCreate(RLThreadFn fn, void* user);
void      RLThreadJoin(RLThread* t);
void      RLThreadDestroy(RLThread* t);

// Mutex
RLMutex*  RLMutexCreate(void);
void      RLMutexLock(RLMutex* m);
void      RLMutexUnlock(RLMutex* m);
void      RLMutexDestroy(RLMutex* m);

// Auto-reset event
RLEvent*  RLEventCreate(bool initialSignaled);
void      RLEventSignal(RLEvent* e);
void      RLEventReset(RLEvent* e);
void      RLEventWait(RLEvent* e);
bool      RLEventWaitTimeout(RLEvent* e, uint32_t timeoutMs);
void      RLEventDestroy(RLEvent* e);

#ifdef __cplusplus
}
#endif
