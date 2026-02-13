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

#ifdef __cplusplus
}
#endif
