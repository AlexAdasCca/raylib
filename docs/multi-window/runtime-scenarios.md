# Runtime Scenarios

[English](./runtime-scenarios.md) | [简体中文](./runtime-scenarios.zh-CN.md)

## 1. Purpose

This page documents representative runtime flows with:
1. operation sequence
2. expected results
3. common failure points
4. code references

## 2. Scenario: Create an Independent Event-Thread Worker Window

### Steps
1. Worker thread creates an `RLContext`.
2. Worker thread sets current context.
3. Enable `RL_E_FLAG_WINDOW_EVENT_THREAD` before init.
4. Call `RLInitWindow()`.

### Expected behavior
1. Event thread owns Win32 message loop.
2. Worker thread remains render thread.
3. Render loop proceeds through `RLBeginDrawing()` / `RLEndDrawing()`.

### Code snippet
```c
RLContext *workerContext = RLCreateContext();
RLSetCurrentContext(workerContext);
RLSetConfigFlags(RL_E_FLAG_WINDOW_EVENT_THREAD | RL_E_FLAG_WINDOW_RESIZABLE);
RLInitWindow(640, 360, "worker");
```

## 3. Scenario: Create a Shared Secondary Window

### Steps
1. Create and initialize primary window first.
2. Create worker context.
3. Configure share mode.
4. Validate share configuration.
5. Initialize worker window only if validation succeeds.

### Expected behavior
1. Share bind succeeds only when target preconditions are valid.
2. Bind fails when tracked-scope promotion fails.
3. No fallback to no-share mode.

### Code snippet
```c
RLContextSetResourceShareMode(workerContext, RL_CONTEXT_SHARE_WITH_PRIMARY, NULL);
if (!RLContextValidateResourceShareConfig(workerContext))
{
    // fail fast: do not call RLInitWindow()
}
RLInitWindow(640, 360, "worker(shared)");
```

## 4. Scenario: Post Work to Win32 Window Thread

### API
`RLWin32InvokeOnWindowThreadByHandle(hwnd, fn, user, wait)`

### `wait` semantics
1. `wait == 0`: asynchronous post
2. `wait != 0`: synchronous wait until completion

### Use case
Win32 thread-affine operations that must execute on the target HWND owner thread.

## 5. Scenario: Post Raw Work to Render Thread

### API
`RLInvokeOnWindowRenderThreadByHandle(hwnd, fn, user, wait)`

### Expected behavior
1. Callback runs on target render thread.
2. Not frame-boundary-safe by contract.
3. In non-event-thread mode, call is limited by current-context ownership checks.

## 6. Scenario: Post Frame-Boundary-Safe Callback

### API
`RLPostWindowFrameCallbackByHandleEx2(...)`

### Expected behavior
1. Callback executes at fixed point inside target-window `RLEndDrawing()`.
2. Queue is bounded; enqueue can fail.
3. `userDtor` handles payload cleanup when callback never executes.

### Minimal snippet
```c
RLPostWindowFrameCallbackByHandleEx2(
    hwnd,                       // target window handle
    DrawOverlayOnTarget,        // render-thread callback
    userPayload,                // payload
    RL_FRAME_CALLBACK_KIND_NORMAL,
    FreeUserPayload);           // cleanup when callback is not executed
```

## 7. Scenario: Frame Callback Queue Saturation

### Steps
1. Producer attempts enqueue.
2. Producer waits for per-kind slot credits with bounded timeout.
3. On timeout, enqueue fails and failure counters are updated.
4. Payload destructor is called on failure paths.

### Code context
Frame callback wait timeout selection:
[`src/platforms/rcore_desktop_glfw.c`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:2770)

```c
return (kind == RL_FRAME_CALLBACK_KIND_CRITICAL)
    ? RL_FRAME_CALLBACK_WAIT_CRITICAL_MS
    : RL_FRAME_CALLBACK_WAIT_NORMAL_MS;
```

### Special case
If producer is already the target render thread and queue is saturated, implementation may execute callback immediately to avoid self-deadlock.

## 8. Scenario: Orphan Adopt After Context Destroy

### Steps
1. Owner context is destroyed.
2. Shared owner becomes orphaned.
3. Another context in the same share-group attempts adopt.

### Expected behavior
Adopt succeeds only when both metadata layers accept the transaction.

## 9. Scenario: Minimize Then Restore

### Steps
1. Minimized window may accumulate pending frame callbacks.
2. After restore, queue drains with bounded per-frame budget.
3. FPS can recover gradually due to queue drain and FPS smoothing.

### Review hint
Inspect both:
1. callback queue counters
2. frame timing counters

before concluding a render regression.

## 10. Scenario: Window Close While Producers Wait for Frame-Callback Slots

### Steps
1. Producers block waiting for slot credits.
2. Close starts; queue enters closing/stopped state.
3. Waiting producers are woken.
4. All new enqueue attempts fail.

### Expected behavior
No callback enqueue succeeds after close state transition.

### Related self-test
`core_event_thread_diagnostics --queue-saturation-selftest`
