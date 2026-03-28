# 线程与生命周期契约

[English](./threading-and-lifecycle.md) | [简体中文](./threading-and-lifecycle.zh-CN.md)

## 1. 为什么这一章最重要

当前分支里，大部分错误都不是“API 名字写错了”，而是：
- 在错误线程调用了正确 API
- 在错误生命周期阶段调用了正确 API
- 把原始线程调用误当成帧边界安全回调
- 把 destroy 当成了 close

所以这一章的目标不是解释“API 有哪些”，而是解释“什么时候能调、什么时候不能调”。

## 2. 线程角色

| 角色 | 定义 | 典型职责 |
|---|---|---|
| 当前上下文线程 | 当前线程已经通过 `RLSetCurrentContext()` 选中了某个 `RLContext` | 当前上下文相关 API |
| 渲染线程 | 真正执行 `RLBeginDrawing()` / `RLEndDrawing()` 的线程 | 绘制、帧回调、共享资源清理 |
| 事件线程 | `RL_E_FLAG_WINDOW_EVENT_THREAD` 模式下的专用 Win32 消息线程 | Win32 消息循环、窗口亲和操作 |
| 窗口线程 | 对 Win32 来说，就是拥有该 `HWND` 消息循环的线程 | Win32 UI 操作、消息钩子、属性 |

> [!NOTE]
> 在事件线程模式下，窗口线程通常就是事件线程。  
> 渲染线程仍然是调用 `RLBeginDrawing()` / `RLEndDrawing()` 的那条线程。

## 3. 生命周期顺序

标准顺序：

```text
RLCreateContext
RLSetCurrentContext
[optional] RLContextSetResourceShareMode, then RLContextValidateResourceShareConfig
RLSetConfigFlags
RLInitWindow / RLInitWindowEx
draw loop
RLCloseWindow
RLDestroyContext
```

### 3.1 `RLCreateContext()`
创建一个不透明的 `RLContext` 句柄。

### 3.2 `RLSetCurrentContext()`
把这个上下文设为调用线程的当前上下文。

### 3.3 `RLInitWindow()` / `RLInitWindowEx()`
将本地窗口和 GL 上下文绑定到当前 `RLContext`。

### 3.4 `RLCloseWindow()`
关闭当前上下文的窗口，并完成窗口关闭时必须执行的清理工作。

### 3.5 `RLDestroyContext()`
销毁上下文对象本身。

> [!WARNING]
> `RLDestroyContext()` 不是 `RLCloseWindow()` 的替代品。  
> 如果窗口还活着，你必须先按正确线程路径调用 `RLCloseWindow()`。

## 4. 线程亲和分类

## 4.1 当前上下文亲和 API

这类 API 依赖“当前线程已经选中的 `RLContext`”。

典型 API：
- `RLInitWindow()`
- `RLInitWindowEx()`
- `RLCloseWindow()`
- `RLDeletePendingSharedGpuResources()`
- `RLSharedRetain*()` / `RLSharedRelease*()`
- `RLBeginSharedShaderUse()` / `RLSharedShaderUseEnd()`

错误模式：
- 在错误线程上调用，并误以为 `ctx` 是全局共享状态

## 4.2 窗口线程亲和 API

这类 API 必须运行在目标窗口所属的 Win32 线程上。

典型 API：
- `RLWin32InvokeOnWindowThreadByHandle()` 提交的回调
- Win32 property bag / hook 的底层处理路径

典型用途：
- 需要操作 HWND 亲和状态
- 需要在窗口线程执行 Win32 UI 逻辑

## 4.3 渲染线程亲和 API

这类 API 必须运行在目标窗口的渲染线程上。

典型 API：
- `RLInvokeOnWindowRenderThreadByHandle()`
- `RLPostWindowFrameCallbackByHandle*()` 的回调本体
- `RLDeletePendingSharedGpuResourcesByHandle()`

## 5. 合法调度方式

### 5.1 需要 Win32 线程亲和
用：
- `RLWin32InvokeOnWindowThreadByHandle()`

### 5.2 需要渲染线程亲和，但不要求帧边界安全
用：
- `RLInvokeOnWindowRenderThreadByHandle()`

### 5.3 需要帧边界安全的绘制或清理
用：
- `RLPostWindowFrameCallbackByHandle()`
- `RLPostWindowFrameCallbackByHandleEx()`
- `RLPostWindowFrameCallbackByHandleEx2()`

> [!IMPORTANT]
> `RLInvokeOnWindowRenderThreadByHandle()` 只保证“在目标渲染线程上执行”，  
> 不保证“在固定帧边界执行”。  
> 如果你需要可预测的绘制时机，请使用帧边界安全回调 API。

## 6. 事件线程模式约束

### 当前约束
当 `RL_E_FLAG_WINDOW_EVENT_THREAD` 启用时，当前实现只支持：
- 一条渲染线程对应一个活动窗口

为什么：
- 这是当前分支明确的设计收紧，不是缺陷。
- 这样可以保持窗口归属、消息归属和渲染归属关系稳定且易于推导。

### 非法模式
- 同一条渲染线程在事件线程模式下反复创建多个活动窗口

结果：
- 当前实现会拒绝，不会进入“勉强支持但语义不完整”的状态。

## 7. 共享模式生命周期约束

### 必须在窗口创建前配置
`RLContextSetResourceShareMode()` 只能在 `RLInitWindow()` 之前调用。

### 显式共享必须目标已存在

| 模式                              | 条件                       |
| ------------------------------- | ------------------------ |
| `RL_CONTEXT_SHARE_WITH_PRIMARY` | 主窗口已经创建并初始化              |
| `RL_CONTEXT_SHARE_WITH_CONTEXT` | `shareWith` 对应窗口已经创建并初始化 |

> [!IMPORTANT]
> 当前实现不会自动回退到 `RL_CONTEXT_SHARE_NONE`。  
> 失败就是失败，应由调用方决定是否改用非共享模式重试。

## 8. 模态刷新回调约束

启用条件：
- `RL_E_FLAG_WINDOW_REFRESH_CALLBACK`
- `RLSetWindowRefreshCallback(callback)`

回调约束：
- 只能做绘制逻辑
- 不要自己再开事件循环
- 不要手动再调 `RLBeginDrawing()` / `RLEndDrawing()`

原因：
- 框架已经包裹了正确的 begin/end drawing 过程

## 9. 帧回调的生命周期与用户数据所有权

在 `RLPostWindowFrameCallbackByHandleEx2()` 中，`userDtor` 的语义非常重要。

### 原则
- 如果回调 **没有执行**，框架负责按契约清理 `user`
- 如果回调已经执行，`user` 的所有权转移给回调实现

### 这意味着
框架只会在以下路径调用 `userDtor`：
- 入队失败
- 队列关闭或清空导致回调未执行
- 回调被拒绝而未执行

而不会在“回调已执行完”的成功路径再次调用 `userDtor`。

> [!WARNING]
> 如果在回调成功执行后框架仍自动调用 `userDtor`，会引入：
> - 重复释放
> - 释放后继续使用
> - 入队执行路径与立即执行路径的所有权契约不一致

## 10. 关闭与等待者

### 帧回调队列等待者
当前实现使用有界队列和信号量信用计数。

关闭窗口时：
1. 队列进入关闭或停止状态
2. 队列清理与信用回收开始
3. 等待槽位的生产者线程会被唤醒
4. 它们应失败返回，而不是成功入队

### 共享资源清理等待者
共享对象所有权和共享删除路径通过明确的锁顺序与事务边界保证：
- 不留下半成功元数据
- 不在关闭中偷偷成功提交错误状态

## 11. 推荐检查清单

在你写新的多窗口逻辑前，先问自己：

1. 当前线程是不是正确的当前上下文线程？
2. 这件事需要窗口线程亲和还是渲染线程亲和？
3. 我需要原始渲染线程调用，还是帧边界安全回调？
4. 共享模式是不是在窗口创建前配置？
5. 销毁上下文之前，是不是已经正确关闭窗口？
