# 运行时场景

[English](./runtime-scenarios.md) | [简体中文](./runtime-scenarios.zh-CN.md)

## 1. 本章目的

本章给出典型运行时场景，按以下结构展开：
1. 操作步骤
2. 预期行为
3. 失败点
4. 代码上下文

> [!NOTE]
> 本章中所有 `wait` 参数都表示“是否同步等待完成”，不是时间单位。  
> 内部等待时长由实现宏控制，例如 `RL_FRAME_CALLBACK_WAIT_NORMAL_MS`。

## 2. 场景：创建独立事件线程工作窗口

### 步骤
1. 工作线程创建 `RLContext`。
2. 调用 `RLSetCurrentContext()`。
3. 在 `RLInitWindow()` 前设置 `RL_E_FLAG_WINDOW_EVENT_THREAD`。
4. 调用 `RLInitWindow()` 并进入绘制循环。

### 预期行为
1. Win32 消息循环运行在事件线程。
2. 绘制仍由工作线程执行。

### 代码示例
```c
RLContext *workerContext = RLCreateContext();
RLSetCurrentContext(workerContext);
RLSetConfigFlags(RL_E_FLAG_WINDOW_EVENT_THREAD | RL_E_FLAG_WINDOW_RESIZABLE);
RLInitWindow(640, 360, "worker");
```

## 3. 场景：创建共享主窗口资源的次窗口

### 步骤
1. 先创建并初始化主窗口。
2. 创建次窗口上下文。
3. 调用 `RLContextSetResourceShareMode(...)`。
4. 调用 `RLContextValidateResourceShareConfig(...)`。
5. 校验通过后再调用 `RLInitWindow()`。

### 预期行为
1. 对端条件满足时，绑定成功。
2. 作用域提升失败时，绑定失败。
3. 不会自动回退到 `RL_CONTEXT_SHARE_NONE`。

### 代码示例
```c
RLContextSetResourceShareMode(workerContext, RL_CONTEXT_SHARE_WITH_PRIMARY, NULL);
if (!RLContextValidateResourceShareConfig(workerContext))
{
    // 校验失败，停止初始化
    return;
}
RLInitWindow(640, 360, "worker(shared)");
```

### 失败点
1. 目标窗口尚未创建。
2. `shareWith` 指向自身。
3. 绑定时发生 tracked scope promotion 冲突。

## 4. 场景：向 Win32 窗口线程投递工作

### API
`RLWin32InvokeOnWindowThreadByHandle(hwnd, fn, user, wait)`

### `wait` 语义
1. `wait == 0`：异步投递并立即返回。
2. `wait != 0`：同步等待执行完成。

### 适用场景
- HWND 线程亲和的 Win32 操作。

### 代码示例（带注释）
```c
static intptr_t SetWindowTextOnOwnerThread(void* hwnd, void* user)
{
    const wchar_t* title = (const wchar_t*)user;
    SetWindowTextW((HWND)hwnd, title);
    return 1;
}

// wait=1：调用线程同步等待目标窗口线程执行完成
intptr_t ok = RLWin32InvokeOnWindowThreadByHandle(
    hwnd,
    SetWindowTextOnOwnerThread,
    (void*)L"worker-ready",
    1);
```

## 5. 场景：向渲染线程投递原始工作

### API
`RLInvokeOnWindowRenderThreadByHandle(hwnd, fn, user, wait)`

### 预期行为
1. 回调在目标渲染线程执行。
2. 该路径不保证帧边界安全。

### 失败点
1. 目标窗口句柄无效。
2. 目标窗口正在关闭。
3. 当前线程模型不允许该路径。

## 6. 场景：向目标窗口投递帧边界安全回调

### API
`RLPostWindowFrameCallbackByHandleEx2(...)`

### 预期行为
1. 回调在目标窗口 `RLEndDrawing()` 内固定点执行。
2. 队列有界，入队可能失败。
3. 未执行回调的 `user` 由 `userDtor` 按契约清理。

### 最小示例
```c
RLPostWindowFrameCallbackByHandleEx2(
    hwnd,
    DrawOverlayOnTarget,
    userPayload,
    RL_FRAME_CALLBACK_KIND_NORMAL,
    FreeUserPayload);
```

### 代码上下文（实现流程说明）
```c
// RLEndDrawing() 内部：
// 1) 读取 backlog hint；若无排队任务可直接返回
// 2) 在全局锁内批量弹出最多 N 个回调
// 3) 解锁后执行回调，避免在锁内运行用户代码
RLGlfwDrainFrameCallbacksCurrentContext();
```

## 7. 场景：帧回调队列饱和

### 步骤
1. 生产者尝试入队。
2. 按队列类型等待槽位信号量，等待时间有上限。
3. 超时则失败返回，并更新失败统计。
4. 若回调未执行，按契约清理 `user`。

### 代码上下文
等待时间选择逻辑：
[`src/platforms/rcore_desktop_glfw.c:2770`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:2770)

```c
return (kind == RL_FRAME_CALLBACK_KIND_CRITICAL)
    ? RL_FRAME_CALLBACK_WAIT_CRITICAL_MS
    : RL_FRAME_CALLBACK_WAIT_NORMAL_MS;
```

### 特例：生产者即目标渲染线程
为避免自阻塞死锁，队列饱和时允许“立即执行”旁路。

### 代码示例（自阻塞保护）
```c
if (isCurrentThreadTargetRenderThread && queueFull)
{
    callback(hwnd, user);   // 避免等待自己释放槽位
    return 1;
}
```

## 8. 场景：上下文销毁后的孤立对象认领

### 步骤
1. 上下文 A 销毁后，所有权进入孤立状态。
2. 同共享组上下文 B 调用认领 API。

### 预期行为
只有共享元数据和跟踪元数据都接受认领时才成功。

### 代码示例
```c
bool adopted = RLTryAdoptOrphanedSharedObject(
    RL_SHARED_OBJECT_TEXTURE,
    textureId,
    targetCtx);
```

## 9. 场景：最小化后恢复

### 现象
恢复后 FPS 可能先低后高。

### 常见原因
1. 回调积压按预算逐帧排空。
2. FPS 指标本身有平滑窗口。

### 这是否是底层 bug
多数情况下属于可预期现象，不等于功能错误。  
若长期恢复缓慢，应结合诊断统计继续排查。

### 排查建议
同时观察：
1. `frameCallbackQueueDepthPeak` 是否持续增长。
2. `frameCallbackQueuePostFailed*` 是否持续增加。
3. `pump*` 与 `swapCost*` / `waitCost*` 是否异常。

## 10. 场景：关闭窗口时仍有生产者等待帧回调槽位

### 步骤
1. 生产者在槽位信号量上等待。
2. 窗口进入关闭状态。
3. 队列清理并关闭信号量。
4. 等待者被唤醒并失败返回。

### 正确结果
1. 不会出现关闭后仍成功入队。
2. 不会出现等待者卡死。

### 代码上下文（简化）
```c
queue->closing = 1;
RLSemaphoreClose(queue->slotsAvailableSemaphore);

if (queue->closing) return 0;
```

### 对应自测
`core_event_thread_diagnostics --queue-saturation-selftest`
