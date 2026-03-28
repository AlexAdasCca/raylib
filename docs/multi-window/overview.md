# Overview

[English](./overview.md) | [简体中文](./overview.zh-CN.md)

## 1. Documentation Goal

This page gives a high-level contract for multi-window behavior in this branch. It is not a full API index.

It answers three questions:
1. Which capabilities are supported.
2. Which capabilities are intentionally unsupported.
3. Which rules are stable and must be treated as contract.

## 2. Supported Capabilities

### 2.1 Traditional single-window mode
- No event-thread separation.
- Window and rendering typically run on one thread.
- Cross-thread helpers are not the primary path.

### 2.2 Event-thread separation mode
- Enable `RL_E_FLAG_WINDOW_EVENT_THREAD` before window creation.
- Win32 message loop runs on a dedicated event thread.
- Rendering remains on the render thread.

### 2.3 Shared-context resource mode
- Configure share mode before `RLInitWindow()`.
- Share with primary or a specific peer context.
- Shared metadata and tracked metadata follow transactional ownership rules.

### 2.4 Win32 modal-loop refresh callback mode
- Enable `RL_E_FLAG_WINDOW_REFRESH_CALLBACK`.
- Install callback through `RLSetWindowRefreshCallback()`.
- Used to repaint during Win32 modal loops when normal frame loop is paused.

## 3. Explicit Non-Goals

### 3.1 Multiple active windows on one render thread in event-thread mode
Current implementation rejects this mode.

### 3.2 Live rebind from one share-group to another after binding
Current implementation rejects live rebind to avoid half-migrated metadata state.

### 3.3 Arbitrary thread calling arbitrary window/render APIs
Thread affinity is explicit. Window-thread, render-thread, and current-context APIs are not interchangeable.

## 4. Stable Contract

### 4.1 Lifecycle order

```text
RLCreateContext
RLSetCurrentContext
[optional] RLContextSetResourceShareMode + validation
RLInitWindow / RLInitWindowEx
draw loop
RLCloseWindow
RLDestroyContext
```

### 4.2 Share-mode timing
`RLContextSetResourceShareMode()` is pre-init only.

### 4.3 Explicit sharing requires an existing target window
- `RL_CONTEXT_SHARE_WITH_PRIMARY`: primary window must already be initialized.
- `RL_CONTEXT_SHARE_WITH_CONTEXT`: target context must already own an initialized window.

### 4.4 Raw render-thread invoke and frame-safe callback are different
- `RLInvokeOnWindowRenderThreadByHandle()` gives render-thread affinity.
- `RLPostWindowFrameCallbackByHandle*()` gives frame-boundary-safe execution.

### 4.5 Shared ownership operations are transactional
Public transfer/adopt APIs do not report success after updating only one metadata layer.

## 5. Minimal Usage Patterns

### 5.1 Independent worker-thread window
```c
static unsigned __stdcall Worker(void *arg)
{
    RLContext *workerContext = RLCreateContext();
    RLSetCurrentContext(workerContext);

    RLSetConfigFlags(RL_E_FLAG_WINDOW_EVENT_THREAD | RL_E_FLAG_WINDOW_RESIZABLE);
    RLInitWindow(640, 360, "worker");

    while (!RLWindowShouldClose())
    {
        RLBeginDrawing();
        RLClearBackground(RAYWHITE);
        RLDrawText("worker window", 20, 20, 20, BLACK);
        RLEndDrawing();
    }

    RLCloseWindow();
    RLDestroyContext(workerContext);
    return 0;
}
```

### 5.2 Worker window sharing primary resources
```c
RLContext *primaryContext = RLCreateContext();
RLSetCurrentContext(primaryContext);
RLSetConfigFlags(RL_E_FLAG_WINDOW_EVENT_THREAD);
RLInitWindow(960, 540, "primary");

RLContext *workerContext = RLCreateContext();
RLSetCurrentContext(workerContext);
RLContextSetResourceShareMode(workerContext, RL_CONTEXT_SHARE_WITH_PRIMARY, NULL);
if (!RLContextValidateResourceShareConfig(workerContext))
{
    // configuration invalid
}
RLSetConfigFlags(RL_E_FLAG_WINDOW_EVENT_THREAD);
RLInitWindow(640, 360, "worker(shared)");
```

## 6. Common Misunderstandings

1. `RLInvokeOnWindowRenderThreadByHandle()` is frame-safe.
It is not.

2. Frame callback enqueue cannot fail.
It can fail under bounded queue pressure or close/stop state.

3. Explicit share modes auto-fallback to no-share.
They do not.

## 7. Further Reading

- [threading-and-lifecycle.md](./threading-and-lifecycle.md)
- [resource-sharing.md](./resource-sharing.md)
- [api-reference.md](./api-reference.md)
- [runtime-scenarios.md](./runtime-scenarios.md)
