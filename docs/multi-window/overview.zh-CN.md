# 概述

[English](./overview.md) | [简体中文](./overview.zh-CN.md)

## 1. 文档目标

这份文档将回答三个最核心的问题：

1. 当前分支到底支持哪些多窗口能力。
2. 使用这些能力时必须遵守哪些规则。
3. 哪些是已经固定的契约，哪些是不稳定的实现。

> [!IMPORTANT]
> 这份文档只解释稳定功能和 API 契约，不展开底层队列、锁、作用域提升和共享组内部实现细节。  
> 这些底层细节内容请查看 [implementation-notes.zh-CN.md](./implementation-notes.zh-CN.md)。

## 2. 支持的能力

### 2.1 单窗口传统模式

特点：
- 不启用事件线程。
- 窗口消息(事件)和渲染都在同一线程。
- 接近传统 raylib 用法。
缺点：
- 窗口消息处理很容易影响渲染的性能，且在 DefWindowProc 中进入 Win32 模态循环时，消息线程会被阻滞，影响渲染。
- 没法跨线程改变渲染流程。

适用场景：
- 不需要多窗口。
- 不需要跨线程操作窗口或渲染。

### 2.2 事件线程分离模式

开启方式：
- 必须在调用 `RLInitWindow()` 之前设置 `RL_E_FLAG_WINDOW_EVENT_THREAD`
- 每个上下文（线程）仅允许创建一次窗口，窗口创建后设置 `RL_E_FLAG_WINDOW_EVENT_THREAD` 是无效的。

特点：
- Win32 消息循环运行在专用事件线程上。
- 渲染相关代码仍在持有当前 `RLContext` 的渲染线程上执行。
- 提供当前分支上完整的多窗口能力（资源共享、跨线程边界的回调）。

适用场景：
- 一个进程有多个 raylib 窗口。
- 每个窗口由不同线程渲染。
- 需要跨线程向某个窗口投递 UI 或渲染任务。

### 2.3 共享上下文资源模式

开启方式：
- `RLContextSetResourceShareMode(ctx, mode, shareWith)`
- 必须在 `RLInitWindow()` 前调用
- 目前建议在启用事件线程分离的基础上设置共享上下文，不调用 `RLContextSetResourceShareMode`时，默认为`RL_CONTEXT_SHARE_NONE`

支持的共享模式：

| 模式                              | 含义                      | 前置条件           |
| ------------------------------- | ----------------------- | -------------- |
| `RL_CONTEXT_SHARE_NONE`         | 不与其他上下文显式共享 GPU 命名空间    | 无              |
| `RL_CONTEXT_SHARE_WITH_PRIMARY` | 与主窗口对应的上下文共享            | 主窗口已创建并初始化     |
| `RL_CONTEXT_SHARE_WITH_CONTEXT` | 与指定 `RLContext` 对应的窗口共享 | 目标上下文已创建窗口并初始化 |

适用场景：
- 主窗口加载纹理或着色器，子窗口继续使用。
- 一个窗口关闭后，另一个窗口仍然继续持有共享资源。
- 需要跨上下文管理 GPU 对象生命周期。

### 2.4 Win32 模态循环刷新回调

开启方式：
- `RL_E_FLAG_WINDOW_REFRESH_CALLBACK`
- `RLSetWindowRefreshCallback(callback)`

特点：
- 解决在单线程模型下，Win32 窗口框架会在 titlebar/move/resize/menu tracking 期间进入系统内部的模态循环，正常线程的主循环被暂停，导致窗口不能刷新、渲染被暂停的问题。
- 回调由框架投递到正确的渲染过程里。
- 在使用事件线程（Win32 UI 线程，消息线程）分离的多线程模型下，不需要使用此刷新回调。刷新回调仅供创建窗口但并不期望使用事件线程分离时使用。

适用场景：
- 需要在模态循环中执行任务。
- 需要在窗口拖动、菜单跟踪期间仍有可视反馈。

## 3. 明确不支持的能力

### 3.1 事件线程模式下一条渲染线程对应多个窗口

当前实现明确拒绝。

原因：
- 当前渲染线程模型仅支持一条渲染线程创建一个窗口。
- 开放任意单线程多窗口将引入相对复杂的系统设计，这不是我们所期望的。尽管单线程多窗口可以提供更严格的上下文控制。

### 3.2 已经绑定到共享组的上下文，再运行时切换到另一个共享组

当前实现明确拒绝。

原因：
- 这不是简单替换指针，而是需要迁移大量共享组级状态。
- 如果勉强支持，会造成跟踪作用域残留、所有权元数据不一致，以及资源生命周期分裂。
- 这种操作不仅是对底层设计的挑战，也是对性能的一大影响。并且我们仅允许内部在某些条件下隐式提升上下文的作用域，但不会提供公共操作API。

### 3.3 任意线程随意调用所有窗口和渲染 API

当前实现要求用户自己设计调用，并不为用户和终端用户提供自动和稳定的调用接口。用户（开发者）必须自己控制调用，并注意到渲染线程回调并不具备完整的稳定性，帧间会有轻微闪烁。开发者可以认为当前渲染线程回调是实验性功能，将在未来优化其稳定性。

我们将 API 按照线程亲和分成了：
- 当前上下文亲和
- 窗口线程亲和
- 渲染线程亲和

如果调用线程不匹配，必须通过相应的线程调度 API 进行调度。调度 API 中，帧安全的调度不支持同步等待模型，其他API目前均支持同步等待（但目前不支持设置有超时的等待）。

## 4. 最关键的稳定契约

## 4.1 生命周期顺序

标准顺序：

1. `RLCreateContext()`
2. `RLSetCurrentContext()`
3. 可选：先调用 `RLContextSetResourceShareMode()`，再调用 `RLContextValidateResourceShareConfig()`
4. `RLSetConfigFlags()`
5. `RLInitWindow()` / `RLInitWindowEx()`
6. 渲染循环
7. `RLCloseWindow()`
8. `RLDestroyContext()`

> [!WARNING]
> `RLDestroyContext()` 不会自动处理关闭窗口，用户必须自行调用`RLCloseWindow()`。  
> 使用事件线程分离时，绑定上下文的线程不是事件线程（UI 线程），如果需要处理窗口消息，请使用线程调度 API 调度到事件线程，或者使用 Native 互操作操作窗口过程。

## 4.2 共享模式配置时机

`RLContextSetResourceShareMode()` 只能在窗口创建前调用。  
窗口已经就绪后，修改会被拒绝。

## 4.3 共享资源要求目标上下文已存在窗口

`RL_CONTEXT_SHARE_WITH_PRIMARY` 和 `RL_CONTEXT_SHARE_WITH_CONTEXT` 都要求目标窗口已经存在。  
在操作失败时会显式提供错误号码，不会静默回退到 `RL_CONTEXT_SHARE_NONE`。

## 4.4 帧边界安全回调与原始渲染线程调用不是一回事

| API                                                                                                                          | 是否属于帧边界安全接口 | 典型用途                                 |
| ---------------------------------------------------------------------------------------------------------------------------- | ----------- | ------------------------------------ |
| `RLInvokeOnWindowRenderThreadByHandle()`                                                                                     | 否           | 原始渲染线程亲和，仅用于在渲染线程执行任务（此 API 不用于常规用途） |
| `RLPostWindowFrameCallbackByHandle()`<br>`RLPostWindowFrameCallbackByHandleEx()`<br>`RLPostWindowFrameCallbackByHandleEx2()` | 是           | 在目标窗口 `RLEndDrawing()` 固定点执行的绘制或清理回调 |

## 4.5 共享对象所有权操作具有事务性

`RLTryTransferSharedObjectOwner()` 和 `RLTryAdoptOrphanedSharedObject()` 不再允许出现：
- 共享所有权已更新
- 跟踪所有权未更新
- 但 API 仍返回成功

也就是说，公开 API 仅允许两个所有权的获取或转移操作“要么一起成功，要么一起失败”。

## 5. 最小用法示例

### 5.1 在独立的工作线程上创建窗口

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

操作步骤：
1. 创建上下文。
2. 设为当前上下文。
3. 设置事件线程标志。
4. 创建窗口。
5. 在当前线程进行渲染循环。
6. 先关闭窗口，再销毁上下文。

### 5.2 共享主窗口资源的工作线程窗口

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
    // 不要继续 RLInitWindow()
}

RLSetConfigFlags(RL_E_FLAG_WINDOW_EVENT_THREAD);
RLInitWindow(640, 360, "worker(shared)");
```

> [!IMPORTANT]
> `RLContextValidateResourceShareConfig()` 用于检查上下文配置是否正确。  
> 对显式共享路径，它应该是创建前的常规保护步骤。

## 6. 常见误解

### Q1：只有共享组的对象跟踪作用域是共享组？
不对。  
`RL_CONTEXT_SHARE_NONE` 内部也会以共享组的形式创建（只不过它暂时没有与他共享的成员），且在任何其他上下文成功绑定后，当前上下文会提升至真正的共享组。这种设计保证了统一的生命周期管理。

### Q2：渲染线程亲和的调用都是帧边界安全的吗？
不是。  
原始渲染线程调用不提供固定帧边界语义。

### Q3：帧回调队列一定不会失败，对吗？
不对。  
当前实现采用有界队列、有限等待和显式返回失败。在高压力下，环形队列会达到容量上限，等待操作超时后会对操作返回失败。

## 7. 进一步阅读

- 线程与生命周期细节： [threading-and-lifecycle.zh-CN.md](./threading-and-lifecycle.zh-CN.md)
- 共享资源和所有权语义： [resource-sharing.zh-CN.md](./resource-sharing.zh-CN.md)
- API 参数说明： [api-reference.zh-CN.md](./api-reference.zh-CN.md)
