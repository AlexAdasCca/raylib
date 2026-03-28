# Threading And Lifecycle Contract

[English](./threading-and-lifecycle.md) | [简体中文](./threading-and-lifecycle.zh-CN.md)

## 1. Why This Page Matters

Most failures in this branch are caused by valid API calls made on the wrong thread or at the wrong lifecycle stage.

## 2. Thread Roles

| Role | Definition | Typical Responsibilities |
| --- | --- | --- |
| Current-context thread | Thread that selected an `RLContext` through `RLSetCurrentContext()` | Current-context APIs |
| Render thread | Thread running `RLBeginDrawing()` / `RLEndDrawing()` | Rendering, frame callbacks, shared-resource cleanup |
| Event thread | Dedicated Win32 message thread in `RL_E_FLAG_WINDOW_EVENT_THREAD` mode | Message loop and HWND-affine operations |
| Window thread | Thread owning the target HWND message queue | Win32 UI work and message hooks |

> [!NOTE]
> In event-thread mode, window thread is typically the event thread. Render thread remains separate.

## 3. Lifecycle Sequence

```text
RLCreateContext
RLSetCurrentContext
[optional] RLContextSetResourceShareMode + validation
RLInitWindow / RLInitWindowEx
draw loop
RLCloseWindow
RLDestroyContext
```

### 3.1 `RLCreateContext()`
Creates an opaque context handle.

### 3.2 `RLSetCurrentContext()`
Sets current context for the calling thread.

### 3.3 `RLInitWindow()` / `RLInitWindowEx()`
Binds native window and GL context to current `RLContext`.

### 3.4 `RLCloseWindow()`
Closes current window and runs required close-time cleanup.

### 3.5 `RLDestroyContext()`
Destroys the context object itself.

> [!WARNING]
> `RLDestroyContext()` is not a substitute for `RLCloseWindow()`.

## 4. Thread-Affinity Classes

### 4.1 Current-context-affine APIs
Examples:
- `RLInitWindow()`
- `RLInitWindowEx()`
- `RLCloseWindow()`
- `RLDeletePendingSharedGpuResources()`
- `RLSharedRetain*()` / `RLSharedRelease*()`
- `RLBeginSharedShaderUse()` / `RLSharedShaderUseEnd()`

### 4.2 Window-thread-affine APIs
Examples:
- callbacks posted through `RLWin32InvokeOnWindowThreadByHandle()`
- HWND-affine property and hook operations

### 4.3 Render-thread-affine APIs
Examples:
- `RLInvokeOnWindowRenderThreadByHandle()` callbacks
- `RLPostWindowFrameCallbackByHandle*()` callbacks
- by-handle shared-resource cleanup on target render thread

## 5. Legal Dispatch Choices

1. Need Win32 thread affinity:
Use `RLWin32InvokeOnWindowThreadByHandle()`.

2. Need render-thread affinity but not frame-boundary safety:
Use `RLInvokeOnWindowRenderThreadByHandle()`.

3. Need frame-boundary-safe draw or cleanup:
Use `RLPostWindowFrameCallbackByHandle*()`.

> [!IMPORTANT]
> `RLInvokeOnWindowRenderThreadByHandle()` does not guarantee frame-boundary execution.

## 6. Event-Thread Mode Restrictions

Current restriction in this branch:
- one active window per render thread in event-thread mode.

Reason:
- explicit affinity and ownership model
- no half-supported multi-window-per-render-thread state

## 7. Share-Mode Lifecycle Restrictions

1. Share mode must be configured before window creation.
2. Explicit sharing requires an existing initialized target window.

| Mode | Required precondition |
| --- | --- |
| `RL_CONTEXT_SHARE_WITH_PRIMARY` | Primary window is already initialized |
| `RL_CONTEXT_SHARE_WITH_CONTEXT` | Target context already owns an initialized window |

> [!IMPORTANT]
> No automatic fallback to `RL_CONTEXT_SHARE_NONE`.

## 8. Modal Refresh Callback Restrictions

Enable:
- `RL_E_FLAG_WINDOW_REFRESH_CALLBACK`
- `RLSetWindowRefreshCallback(callback)`

Restrictions:
- draw-only callback logic
- do not run another event loop inside callback
- do not call `RLBeginDrawing()` / `RLEndDrawing()` manually in callback

## 9. Frame Callback Payload Ownership

`RLPostWindowFrameCallbackByHandleEx2()` uses `userDtor` with this contract:
1. callback not executed:
framework cleans payload according to contract
2. callback executed:
ownership transfers to callback implementation

## 10. Close And Waiters

### Frame-callback enqueue waiters
On close/stop:
- waiters are woken
- enqueue fails cleanly
- waiting producers must not succeed after close state

### Shared cleanup waiters
Owner and cleanup operations use explicit lock order and bounded wait semantics to avoid half-committed metadata.

## 11. Review Checklist

1. Is the current thread the correct current-context thread?
2. Is this call window-thread-affine or render-thread-affine?
3. Is this raw render-thread invoke or frame-boundary-safe callback?
4. Was share mode configured before `RLInitWindow()`?
5. Is window closed before context destroy?
