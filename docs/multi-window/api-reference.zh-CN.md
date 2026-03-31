# API 参考

[English](./api-reference.md) | [简体中文](./api-reference.zh-CN.md)

> [!IMPORTANT]
> 本文档只描述当前分支中与多窗口、事件线程、Win32 扩展、共享上下文、共享资源和诊断直接相关的 API。  
> 普通 raylib API 请以官方文档和 `src/raylib.h` 为准。

## 1. 阅读须知

本文档尽量按照以下顺序组织每个 API 条目：
- 功能
- 签名
- 参数
- 返回值
- 线程要求
- 失败语义
- Note / Warning
- 最小示例

统一约定：
1. 本文档中的 `wait` 参数是同步标志，不是时间单位。
2. `wait == 0` 表示异步提交后立即返回。
3. `wait != 0` 表示同步等待目标线程执行完成。
4. 超时阈值是内部实现策略，不由这些 API 参数直接传入。

## 2. 相关类型

## 2.1 `RLContext`

类型：
```c
typedef struct RLContext RLContext;
```

说明：
- 不透明句柄
- 表示一个 raylib 上下文
- 可被某个线程通过 `RLSetCurrentContext()` 选为当前上下文

典型用途：
- 作为窗口创建、资源共享和所有权转移的上下文单位

## 2.2 `RLContextResourceShareMode`

此枚举与共享上下文资源有关，指定了共享上下文的类型。
```c
typedef enum RLContextResourceShareMode {
    RL_CONTEXT_SHARE_NONE = 0,
    RL_CONTEXT_SHARE_WITH_PRIMARY = 1,
    RL_CONTEXT_SHARE_WITH_CONTEXT = 2
} RLContextResourceShareMode;
```

| 值                               | 含义                    | 前置条件                         |
| ------------------------------- | --------------------- | ---------------------------- |
| `RL_CONTEXT_SHARE_NONE`         | 不显式共享 GPU 命名空间        | 无                            |
| `RL_CONTEXT_SHARE_WITH_PRIMARY` | 与主窗口对应的上下文共享          | 主窗口已完成初始化                    |
| `RL_CONTEXT_SHARE_WITH_CONTEXT` | 与显式指定的 `RLContext` 共享 | `shareWith` 参数指定的上下文已完成窗口初始化 |

## 2.3 `RLContextResourceShareValidationError`

共享上下文配置有误时，此枚举指示错误的类型。RLContextGetResourceShareValidationError 函数返回最近一次共享上下文配置校验的错误码。

```c
typedef enum RLContextResourceShareValidationError {
    RL_CONTEXT_SHARE_VALIDATION_OK = 0,
    RL_CONTEXT_SHARE_VALIDATION_CTX_NULL = 1,
    RL_CONTEXT_SHARE_VALIDATION_INVALID_MODE = 2,
    RL_CONTEXT_SHARE_VALIDATION_TARGET_NULL = 3,
    RL_CONTEXT_SHARE_VALIDATION_TARGET_SELF = 4,
    RL_CONTEXT_SHARE_VALIDATION_TARGET_WINDOW_UNAVAILABLE = 5,
    RL_CONTEXT_SHARE_VALIDATION_PRIMARY_WINDOW_UNAVAILABLE = 6
} RLContextResourceShareValidationError;
```

| 值                                                        | 含义           |
| -------------------------------------------------------- | ------------ |
| `RL_CONTEXT_SHARE_VALIDATION_OK`                         | 校验通过         |
| `RL_CONTEXT_SHARE_VALIDATION_CTX_NULL`                   | 传入的上下文指针为空指针 |
| `RL_CONTEXT_SHARE_VALIDATION_INVALID_MODE`               | 错误的共享模式      |
| `RL_CONTEXT_SHARE_VALIDATION_TARGET_NULL`                | 目标上下文缺失      |
| `RL_CONTEXT_SHARE_VALIDATION_TARGET_SELF`                | 目标上下文设置成了自身  |
| `RL_CONTEXT_SHARE_VALIDATION_TARGET_WINDOW_UNAVAILABLE`  | 目标上下文尚未创建窗口  |
| `RL_CONTEXT_SHARE_VALIDATION_PRIMARY_WINDOW_UNAVAILABLE` | 主窗口暂不可用      |

## 2.4 `RLSharedShaderUsePolicy`

```c
typedef enum RLSharedShaderUsePolicy {
    RL_SHARED_SHADER_USE_PHASED = 1,
    RL_SHARED_SHADER_USE_LOCKED = 2
} RLSharedShaderUsePolicy;
```

| 值                             | 含义          | 适合场景          |
| ----------------------------- | ----------- | ------------- |
| `RL_SHARED_SHADER_USE_PHASED` | 按先来先服务顺序串行化 | 多线程长期竞争同一着色器  |
| `RL_SHARED_SHADER_USE_LOCKED` | 简单互斥串行化     | 共享着色器数量少，逻辑简单 |

## 2.5 `RLSharedObjectType`

此枚举指示所支持的共享对象的类型。

```c
typedef enum RLSharedObjectType {
    RL_SHARED_OBJECT_TEXTURE = 1,
    RL_SHARED_OBJECT_BUFFER = 2,
    RL_SHARED_OBJECT_VERTEX_ARRAY = 3,
    RL_SHARED_OBJECT_FRAMEBUFFER = 4,
    RL_SHARED_OBJECT_RENDERBUFFER = 5,
    RL_SHARED_OBJECT_PROGRAM = 6
} RLSharedObjectType;
```

| 值 | 表示的对象 |
|---|---|
| `RL_SHARED_OBJECT_TEXTURE` | 纹理 |
| `RL_SHARED_OBJECT_BUFFER` | 缓冲对象 |
| `RL_SHARED_OBJECT_VERTEX_ARRAY` | 顶点数组对象 |
| `RL_SHARED_OBJECT_FRAMEBUFFER` | 帧缓冲对象 |
| `RL_SHARED_OBJECT_RENDERBUFFER` | 渲染缓冲对象 |
| `RL_SHARED_OBJECT_PROGRAM` | 着色器程序 |

## 2.6 `RLWindowRefreshCallback`

```c
typedef void (*RLWindowRefreshCallback)(void);
```

说明：
- Win32 模态循环中的刷新回调
- 由框架在正确的绘制流程内调用

要求：
- 只执行绘制逻辑
- 不需要也不应该由用户在回调中调用 `RLBeginDrawing()` / `RLEndDrawing()`

## 2.7 `RLWin32MessageHook`

```c
typedef int (*RLWin32MessageHook)(void* hwnd,
                                  unsigned int uMsg,
                                  uintptr_t wParam,
                                  intptr_t lParam,
                                  intptr_t* result,
                                  void* user);
```

| 参数       | 类型             | 说明                        |
| -------- | -------------- | ------------------------- |
| `hwnd`   | `void*`        | 目标 Win32 窗口句柄，语义上是 `HWND` |
| `uMsg`   | `unsigned int` | Win32 消息 id               |
| `wParam` | `uintptr_t`    | Win32 `wParam`            |
| `lParam` | `intptr_t`     | Win32 `lParam`            |
| `result` | `intptr_t*`    | 如果 hook 处理了消息，填入返回值       |
| `user`   | `void*`        | 用户数据                      |

返回值：
- 非零：消息已处理，窗口过程使用 `*result`
- 零：继续正常处理

## 2.8 `RLWin32WindowThreadInvoke`

```c
typedef intptr_t (*RLWin32WindowThreadInvoke)(void* hwnd, void* user);
```

说明：
- 在目标窗口所属的 Win32 线程上执行

## 2.9 `RLWindowRenderThreadInvoke`

```c
typedef intptr_t (*RLWindowRenderThreadInvoke)(void* hwnd, void* user);
```

说明：
- 在目标窗口的渲染线程上执行

## 2.10 `RLFrameCallbackKind`

```c
typedef enum RLFrameCallbackKind {
    RL_FRAME_CALLBACK_KIND_NORMAL = 0,
    RL_FRAME_CALLBACK_KIND_CRITICAL = 1
} RLFrameCallbackKind;
```

| 值                                 | 含义         | 当前实现                    |
| --------------------------------- | ---------- | ----------------------- |
| `RL_FRAME_CALLBACK_KIND_NORMAL`   | 普通帧回调      | 有界队列，有限等待               |
| `RL_FRAME_CALLBACK_KIND_CRITICAL` | 生命周期或清理类回调 | 独立有界队列，等待时间更长，保障每帧最低执行数 |

## 2.11 `RLEventThreadDiagStats`

说明：
- 大型诊断结构
- 用于读取当前窗口或当前上下文的事件线程分离模式的统计数据
- 标记为 `(*)` 的字段组是其他诊断子系统的镜像副本；可以通过本结构统一读取，但 `RLResetEventThreadDiagStats*()` 与 `RLEnableEventDiagStats()` / `RLDisableEventDiagStats()` 不会去重置或禁用这些源子系统。关闭 event diag 后，`RLGetEventThreadDiagStats()` 也不会再填充这些镜像 `(*)` 字段；如仍需读取真实值，应改用对应子系统的独立 API

建议关注的字段分组：

| 字段组 | 说明 |
|---|---|
| `payloadAlloc*` / `payloadFree*` | 跨线程消息或输入数据的分配统计 |
| `nativeTaskQueue* (*)` | GLFW Win32 本地任务队列的深度、峰值和丢弃统计 |
| `frameCallbackQueue* (*)` | 帧回调队列的深度、峰值、丢弃、执行、清空和 inline fallback 统计 |
| `pump*` | 事件泵执行次数和耗时 |
| `swapCost*` / `waitCost*` / `frameCpu*` | 帧阶段耗时诊断 |
| `threadMismatch* (*)` | GPU 写入错线程检测与切换统计 |
| `sharedUnregistered* (*)` | shared-GPU 未注册 retain/release 拒绝统计 |

## 2.12 `RLThreadMismatchDiagStats`

说明：
- 用于读取 GPU 写入错线程检测的聚合结果

关键字段：

| 字段 | 含义 |
|---|---|
| `detectedCount` | 检测到的错线程 GPU 写入次数 |
| `handoffAttemptedCount` | 尝试同步切换的次数 |
| `handoffSuccessCount` | 同步切换成功次数 |
| `handoffFailedCount` | 同步切换失败次数 |
| `deferredQueuedCount` | 延迟帧回调切换已入队次数 |
| `deferredExecutedCount` | 延迟切换实际执行次数 |
| `rejectedCount` | 被拒绝的次数 |
| `lastApi` | 最近触发 mismatch 的 API 名 |
| `lastWindowHandle` | 最近相关窗口句柄 |

## 2.13 `RLFrameCallbackQueueStats`

说明：
- 当前窗口 / 当前上下文的 frame-callback 队列轻量快照

`inline fallback` 的含义：
- 调用线程此时已经是目标窗口的 render thread
- 所选 frame-callback 队列当下没有可用槽位
- 系统不会继续等待队列腾出空间，而是直接在当前 render thread 立即执行该回调

注意：
- 进入 inline fallback 的回调会计入 `executed*`
- 同时也会计入 `inlineFallback*`
- 不会真正进入队列，因此不会增加 `queued*`
- 也不算 dropped 或 cleared
- 一旦开始执行，`user` 的所有权仍按“回调已执行成功”的规则转移给回调实现

关键字段：

| 字段 | 含义 |
|---|---|
| `queuedCount` | 当前总排队回调数 |
| `queuedNormalCount` | 当前 normal 回调排队数 |
| `queuedCriticalCount` | 当前 critical 回调排队数 |
| `queuedPeakCount` | 自上次 reset 以来的峰值 |
| `queuedPeakNormalCount` | 自上次 reset 以来的 normal 队列峰值 |
| `queuedPeakCriticalCount` | 自上次 reset 以来的 critical 队列峰值 |
| `droppedCount` | 总丢弃数 |
| `droppedNormalCount` | 普通回调丢弃数 |
| `droppedCriticalCount` | critical 回调丢弃数 |
| `executedCount` | 已执行回调总数 |
| `executedNormalCount` | 已执行 normal 回调数 |
| `executedCriticalCount` | 已执行 critical 回调数 |
| `clearedCount` | 已入队但在执行前被清掉的回调总数 |
| `clearedNormalCount` | 被清掉的 normal 回调数 |
| `clearedCriticalCount` | 被清掉的 critical 回调数 |
| `inlineFallbackCount` | 队列饱和时直接在 render thread 立即执行的回调总数 |
| `inlineFallbackNormalCount` | 通过 inline fallback 执行的 normal 回调数 |
| `inlineFallbackCriticalCount` | 通过 inline fallback 执行的 critical 回调数 |

## 3. 标志位

| 标志位                                 | 含义                  |
| ----------------------------------- | ------------------- |
| `RL_E_FLAG_WINDOW_EVENT_THREAD`     | 为窗口创建专用 Win32 消息线程  |
| `RL_E_FLAG_WINDOW_REFRESH_CALLBACK` | 启用 Win32 模态循环刷新回调   |
| `RL_E_FLAG_WINDOW_BROADCAST_WAKE`   | 关闭或退出时广播唤醒所有窗口的渲染线程 |
请注意：当前设计下，PRIMARY 窗口强制具备 `RL_E_FLAG_WINDOW_BROADCAST_WAKE`，也就是说，主窗口被销毁时，会自动关闭其他窗口。
## 4. Context API

### `RLContext *RLCreateContext(void)`

功能：
- 创建一个 `RLContext`

返回值：
- 成功：`RLContext*`
- 失败：`NULL`

线程要求：
- 任意线程都可以调用，但返回的上下文只有在调用 `RLSetCurrentContext()` 后，才会成为当前线程的当前上下文

示例：
```c
RLContext *ctx = RLCreateContext();
RLSetCurrentContext(ctx);
```

### `void RLDestroyContext(RLContext *ctx)`

功能：
- 销毁 `RLContext` 对象

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `ctx` | `RLContext*` | 要销毁的上下文 |

> [!WARNING]
> 此 API 并不能用于正常关闭窗口。  
> 如果 `ctx` 指定的上下文仍拥有存活的窗口，必须先按正确线程路径调用 `RLCloseWindow()`。

### `void RLSetCurrentContext(RLContext *ctx)`

功能：
- 把 `ctx` 设为当前线程的当前上下文

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `ctx` | `RLContext*` | 要绑定到当前线程的上下文 |

补充说明：
- 该设置只影响调用线程
- 不会同步改变其他线程的当前上下文

### `RLContext *RLGetCurrentContext(void)`

功能：
- 返回当前线程当前绑定的上下文

## 5. 共享配置 API

### `bool RLContextSetResourceShareMode(RLContext* ctx, RLContextResourceShareMode mode, RLContext* shareWith)`

功能：
- 配置该上下文下一次创建窗口时的 GPU 资源共享方式

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `ctx` | `RLContext*` | 要配置的上下文 |
| `mode` | `RLContextResourceShareMode` | 共享模式 |
| `shareWith` | `RLContext*` | 显式共享目标，仅 `WITH_CONTEXT` 使用 |

返回值：
- `true`：配置被接受
- `false`：参数非法、生命周期非法或配置不允许

失败语义：
- 不会偷偷回退到 `RL_CONTEXT_SHARE_NONE`

### `RLContextResourceShareMode RLContextGetResourceShareMode(RLContext* ctx)`

功能：
- 返回当前配置的共享模式

### `RLContext* RLContextGetResourceShareContext(RLContext* ctx)`

功能：
- 返回显式共享目标上下文

### `bool RLContextValidateResourceShareConfig(RLContext* ctx)`

功能：
- 校验当前共享配置，并把最近一次结果记录到上下文内部状态中

建议：
- 在调用 `RLInitWindow()` 前调用

### `bool RLContextIsResourceShareConfigValid(RLContext* ctx)`

功能：
- 返回最近一次缓存的校验结果

### `int RLContextGetResourceShareValidationError(RLContext* ctx)`

功能：
- 返回最近一次缓存的校验错误码

类型：
- `RLContextResourceShareValidationError`

## 6. 共享 GPU 生命周期 API

### `bool RLSharedRetainShader(RLShader shader)` / `bool RLSharedReleaseShader(RLShader shader)`

功能：
- 在共享组范围内保留或释放着色器程序

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `shader` | `RLShader` | 其 `id` 用于识别 program |

Note：
- 此函数只管理共享组范围内的 GPU 对象生存期，不负责高层资源对象（CPU 和内存）的业务语义

### `bool RLSharedRetainBuffer(unsigned int bufferId)` / `bool RLSharedReleaseBuffer(unsigned int bufferId)`

功能：
- 保留或释放图形缓冲区对象

### `bool RLSharedRetainVertexArray(unsigned int vertexArrayId)` / `bool RLSharedReleaseVertexArray(unsigned int vertexArrayId)`

功能：
- 保留或释放顶点数组对象

### `bool RLSharedRetainFramebuffer(unsigned int framebufferId)` / `bool RLSharedReleaseFramebuffer(unsigned int framebufferId)`

功能：
- 保留或释放帧缓冲对象，以及当前由跟踪对象系统管理的附件对象

### `bool RLSharedRetainFramebufferBase(unsigned int framebufferId)` / `bool RLSharedReleaseFramebufferBase(unsigned int framebufferId)`

功能：
- 仅保留或释放帧缓冲对象本身

### `bool RLSharedRetainRenderbuffer(unsigned int renderbufferId)` / `bool RLSharedReleaseRenderbuffer(unsigned int renderbufferId)`

功能：
- 保留或释放渲染缓冲对象

### `bool RLDeletePendingSharedGpuResources(void)`

功能：
- 在当前窗口所属的共享组上清理延迟释放的共享 GPU 对象

线程要求：
- 当前线程必须持有属于该共享组的当前 GL 上下文

### `RLSharedGpuGroupDiagStats`

功能：
- 当前线程当前上下文所属共享组的结构化诊断快照

建议重点关注的字段：

| 字段 | 含义 |
|---|---|
| `hasShareGroup` | 当前上下文是否已绑定共享组 |
| `usesSharedTrackedScope` | tracked-object 作用域是否已从 context 提升为 share-group |
| `contextRefCount` | 当前仍附着在该共享组上的上下文数量 |
| `liveObjectCount` / `pendingDeleteCount` | 当前 live 共享对象数与延迟删除队列长度 |
| `ownerEntryCount` / `orphanedOwnerCount` | 所有权元数据条目数与 orphan 条目数 |
| `framebufferAttachmentMapCount` / `framebufferDepthMapCount` | framebuffer 附件映射表与 depth 映射表的当前条目数 |
| `programLocEntryCount` / `programUseScopeCount` / `pendingProgramFenceCount` | 共享着色器程序协调相关状态 |
| `textureTraceCount` | 纹理追踪元数据条目数 |
| `live*` / `pending*` | 各对象类型的 live / pending 数量 |
| `framebufferMapHitCount` / `framebufferMapMissCount` / `framebufferReleaseSkippedCount` | framebuffer 附件映射命中、缺失、跳过释放统计 |
| `releaseUntrackedCount` | 对未跟踪对象调用 release 的次数 |

适用场景：
- 在 create、retain、unload、flush、close 等关键检查点采集共享 GPU 状态
- 用于自动化日志和断言，而不依赖文本 dump 格式

### `RLSharedGpuGroupDiagStats RLGetCurrentSharedGpuGroupDiagStats(void)` / `RLSharedGpuGroupDiagStats RLGetSharedGpuGroupDiagStatsForContext(RLContext *ctx)`

功能：
- 返回当前上下文共享组或指定上下文共享组的结构化诊断快照

返回语义：
- 如果目标上下文没有绑定共享组，返回零初始化快照
- 如果目标上下文已有共享组，返回当前快照

Note：
- 这是 `RLDebugDumpSharedGpuState()` 的结构化对应接口
- 做自动化判断或稳定日志解析时，应优先使用这个接口
- `live/pending/owner` 相关字段属于即时快照状态
- `releaseUntrackedCount` 和 framebuffer 映射计数属于共享组累计诊断字段；关闭共享 GPU 累计诊断计数后，这些字段会停止增长

### `RLSharedGpuTrackingRejectDiagStats`

功能：
- strict 模式下未注册 retain/release 被拒绝的全局累计统计

### `RLSharedGpuTrackingRejectDiagStats RLGetSharedGpuTrackingRejectDiagStats(void)` / `void RLResetSharedGpuTrackingRejectDiagStats(void)`

功能：
- 读取或重置 strict 模式下未注册 retain/release 被拒绝的全局累计统计

说明：
- 这组 reject 计数是进程级全局统计，不隶属于某一个共享组

### `void RLEnableSharedGpuCumulativeDiagStats(void)` / `void RLDisableSharedGpuCumulativeDiagStats(void)` / `bool RLIsSharedGpuCumulativeDiagStatsEnabled(void)`

功能：
- 在运行时启用、禁用或查询共享 GPU 的累计诊断统计

说明：
- 只有在构建时打开 `RL_SHARED_GPU_DIAG_STATS=1` 时，这组累计统计才真正生效
- 这些接口只控制累计诊断计数，不会关闭共享组所有权和引用计数逻辑
- 这组开关同时影响：
  - 当前共享组的累计诊断字段（`releaseUntrackedCount`、framebuffer 映射计数）
  - strict 模式下未注册 retain/release 的全局 reject 计数
- 启用这组开关不会隐式重置计数

### `void RLResetCurrentSharedGpuGroupDiagStats(void)` / `bool RLResetSharedGpuGroupDiagStatsForContext(RLContext *ctx)`

功能：
- 重置当前上下文所属共享组，或指定上下文所属共享组的累计诊断字段

说明：
- 这组 reset 只作用于共享组自身的累计诊断字段
- 不会重置 strict 模式下未注册 retain/release 的全局 reject 计数

### `void RLDebugDumpSharedGpuState(const char *label)`

功能：
- 将当前共享组快照按人类可读格式输出到 trace log

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `label` | `const char *` | 输出日志时附带的标签，可传 `NULL` |

Note：
- 该接口面向人工排查
- 若需要稳定格式，应使用 `RLGetCurrentSharedGpuGroupDiagStats()` 或 `RLGetSharedGpuGroupDiagStatsForContext()`

## 7. 共享着色器 API

### `bool RLBeginSharedShaderUse(RLShader shader, int policy)`

功能：
- 进入共享着色器的串行化使用范围

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `shader` | `RLShader` | 目标着色器 |
| `policy` | `int` | 语义上是 `RLSharedShaderUsePolicy` |

### `void RLSharedShaderUseEnd(RLShader shader, int policy)`

功能：
- 离开共享着色器使用范围

### `bool RLConfigureSharedShaderFenceWait(unsigned int waitSliceUs, unsigned int waitTimeoutUs)`

功能：
- 配置分阶段共享着色器等待围栏时的轮询与超时参数

参数：

| 参数 | 说明 |
|---|---|
| `waitSliceUs` | 单次 polling slice，单位微秒 |
| `waitTimeoutUs` | 总超时，单位微秒 |

## 8. 轻量 Frame-Callback 队列 API

### `bool RLGetCurrentWindowFrameCallbackQueueStats(RLFrameCallbackQueueStats *outStats)`

功能：
- 获取当前窗口 / 当前上下文的 frame-callback 队列轻量快照

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `outStats` | `RLFrameCallbackQueueStats *` | 输出结构体 |

返回值：
- `true`：当前窗口 / 当前上下文可提供队列统计
- `false`：当前后端或当前窗口模式不支持该统计

说明：
- 这是 `RLEventThreadDiagStats` 中 `frameCallbackQueue*` 字段的轻量对应接口
- 只关心当前队列状态时，优先使用这个接口
- 仅在 Win32 + desktop GLFW 后端可用
- 即使关闭 frame-callback 累计诊断计数，`queuedCount/queuedNormalCount/queuedCriticalCount` 这类当前队列状态仍然有效

### `bool RLGetWindowFrameCallbackQueueStatsByHandle(void* hwnd, RLFrameCallbackQueueStats *outStats)`

功能：
- 获取指定 raylib 窗口的 frame-callback 队列轻量快照

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `hwnd` | `void*` | 目标窗口句柄 |
| `outStats` | `RLFrameCallbackQueueStats *` | 输出结构体 |

返回值：
- `true`：目标窗口可提供队列统计
- `false`：句柄未知，或当前后端 / 窗口模式不支持该统计

说明：
- 适用于查询线程当前没有绑定到目标窗口 / 目标上下文的场景
- 仅在 Win32 + desktop GLFW 后端可用

### `void RLEnableFrameCallbackDiagStats(void)` / `void RLDisableFrameCallbackDiagStats(void)` / `bool RLIsFrameCallbackDiagStatsEnabled(void)`

功能：
- 在运行时启用、禁用或查询 frame-callback 累计诊断统计状态

说明：
- 只有在构建时打开 `RL_FRAME_CALLBACK_DIAG_STATS=1` 时，这组累计统计才真正生效
- 这组开关只影响峰值、dropped、executed、cleared、inline fallback 这类累计统计
- 不会关闭 frame-callback 队列本身

### `void RLResetCurrentWindowFrameCallbackDiagStats(void)` / `int RLResetWindowFrameCallbackDiagStatsByHandle(void* hwnd, int wait)` / `void RLResetAllFrameCallbackDiagStats(void)`

功能：
- 重置当前窗口、指定窗口或全部已跟踪 raylib 窗口的 frame-callback 累计诊断统计

参数（按句柄版本）：

| 参数 | 类型 | 说明 |
|---|---|---|
| `hwnd` | `void*` | 目标窗口句柄 |
| `wait` | `int` | 为保持接口风格保留；当前实现直接完成重置，不使用该值 |

## 8.1 轻量 Native Task Queue API

### `bool RLGetCurrentWindowNativeTaskQueueDiagStats(RLNativeTaskQueueDiagStats *outStats)`

功能：
- 获取当前窗口 / 当前上下文 render thread 的 native task queue 轻量快照

说明：
- 这是 `RLEventThreadDiagStats` 中 `nativeTaskQueue*` 字段的轻量对应接口
- 仅在 Win32 + desktop GLFW 后端可用
- 直接读取 backend 的原生任务队列统计，而不是事件诊断快照中的镜像副本
- 读取动作本身不会再额外投递一条诊断任务，因此不会反过来污染这组队列计数

### `bool RLGetNativeTaskQueueDiagStatsByHandle(void* hwnd, RLNativeTaskQueueDiagStats *outStats)`

功能：
- 获取指定 raylib 窗口 render thread 的 native task queue 轻量快照

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `hwnd` | `void*` | 目标窗口句柄 |
| `outStats` | `RLNativeTaskQueueDiagStats *` | 输出结构体 |

返回值：
- `true`：目标窗口 render thread 可提供队列统计
- `false`：句柄未知，或当前模式下无法安全查询目标线程

### `void RLResetCurrentWindowNativeTaskQueueDiagStats(void)` / `int RLResetNativeTaskQueueDiagStatsByHandle(void* hwnd, int wait)`

功能：
- 重置当前窗口或指定窗口 render thread 的 native task queue 诊断统计

参数（按句柄版本）：

| 参数 | 类型 | 说明 |
|---|---|---|
| `hwnd` | `void*` | 目标窗口句柄 |
| `wait` | `int` | 为与其他按句柄 reset 接口保持一致而保留，当前实现忽略该参数 |

## 9. 所有权与认领 API

### `RLContext* RLGetSharedObjectOwnerContext(RLSharedObjectType type, unsigned int objectId)`

功能：
- 查询共享对象当前的所有者上下文

### `bool RLIsSharedObjectOwnedByCurrentContext(RLSharedObjectType type, unsigned int objectId)`

功能：
- 查询当前上下文是否拥有该对象

### `bool RLTryTransferSharedObjectOwner(RLSharedObjectType type, unsigned int objectId, RLContext* targetCtx)`

功能：
- 把共享对象的所有权转移给 `targetCtx`

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `type` | `RLSharedObjectType` | 对象类型 |
| `objectId` | `unsigned int` | GL 对象 id / program id |
| `targetCtx` | `RLContext*` | 目标上下文 |

返回值：
- `true`：共享元数据与跟踪对象元数据都已成功提交
- `false`：失败

> [!IMPORTANT]
> 当前公开 API 语义下，这个操作是事务性的。  
> 不允许出现“共享所有权元数据已更新，跟踪对象所有权元数据未更新，但 API 仍返回成功”的情况。

### `bool RLTryAdoptOrphanedSharedObject(RLSharedObjectType type, unsigned int objectId, RLContext* targetCtx)`

功能：
- 认领在上下文销毁后变为孤立状态的对象的所有权

前提：
- 对象所有权确实在上下文销毁时变为孤立状态
- `targetCtx` 已属于同一个共享组

### 通用别名

以下函数与上面的 API 语义相同，只是名称更通用：
- `RLGetObjectOwnerContext()`
- `RLIsObjectOwnedByCurrentContext()`
- `RLTryTransferObjectOwner()`
- `RLTryAdoptOrphanedObject()`

## 9. Win32 与窗口相关 API

### `void RLInitWindowEx(int width, int height, const char *title, const char *win32ClassName)`

功能：
- 支持设置 Win32 窗口类名的窗口初始化函数

参数：

| 参数                | 说明       |
| ----------------- | -------- |
| `width`, `height` | 初始窗口尺寸   |
| `title`           | 标题       |
| `win32ClassName`  | Win32 类名 |

### `void *RLGetWindowHandle(void)`

功能：
- 获取当前窗口的平台层句柄

类型说明：
- 在 Win32 上语义上是 `HWND`
- 公开 API 以 `void*` 暴露该句柄

示例：
```c
void *hwnd = RLGetWindowHandle();
if (hwnd == NULL)
{
    // 当前线程没有可用窗口句柄
}
```

### `void RLSetWindowWin32ClassName(const char *win32ClassName)`

功能：
- 设置下一次窗口创建要使用的一次性 Win32 窗口类名，效果类似于使用 `RLInitWindowEx`。

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `win32ClassName` | `const char *` | 一次性类名。传 `NULL` 表示清除待应用值 |

Note：
- 该值仅作用于下一次窗口创建，不会影响已创建窗口。

### `void RLSetWindowRefreshCallback(RLWindowRefreshCallback callback)`

功能：
- 设置 Win32 模态循环期间的刷新回调

使用条件：
- 需要配合 `RL_E_FLAG_WINDOW_REFRESH_CALLBACK` 标识使用

示例：
```c
static void OnRefreshDraw(void)
{
    RLClearBackground(RAYWHITE);
    RLDrawText("modal loop repaint", 20, 20, 20, RED);
}

RLSetConfigFlags(RL_E_FLAG_WINDOW_REFRESH_CALLBACK);
RLInitWindow(800, 450, "refresh");
RLSetWindowRefreshCallback(OnRefreshDraw);
```

## 10. Win32 窗口句柄的属性与消息钩子 API

### `int RLWin32SetWindowProp(const char* name, void* value)`

功能：
- 给当前窗口设置一个 `HWND` 属性

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `name` | `const char*` | 属性键名 |
| `value` | `void*` | 属性值 |

返回值：
- 非零表示成功
- 零表示失败

### `void* RLWin32GetWindowProp(const char* name)`

功能：
- 读取当前窗口的 `HWND` 属性

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `name` | `const char*` | 属性键名 |

返回值：
- 成功返回属性值
- 不存在或失败返回 `NULL`

### `void* RLWin32RemoveWindowProp(const char* name)`

功能：
- 移除当前窗口的 `HWND` 属性

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `name` | `const char*` | 属性键名 |

返回值：
- 返回被移除的旧值
- 不存在或失败返回 `NULL`

### `void* RLWin32AddMessageHook(RLWin32MessageHook hook, void* user)`

功能：
- 为当前窗口注册 Win32 消息钩子

返回值：
- 不透明令牌，用于后续移除

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `hook` | `RLWin32MessageHook` | 钩子函数 |
| `user` | `void*` | 用户数据 |

示例：
```c
static int MsgHook(void* hwnd, unsigned int msg, uintptr_t wParam, intptr_t lParam, intptr_t* result, void* user)
{
    if (msg == WM_NCHITTEST)
    {
        *result = HTCLIENT;
        return 1;
    }
    return 0;
}

void *token = RLWin32AddMessageHook(MsgHook, NULL);
```

### `int RLWin32RemoveMessageHook(void* token)`

功能：
- 移除消息钩子

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `token` | `void*` | `RLWin32AddMessageHook` 返回的令牌 |

返回值：
- 非零表示成功
- 零表示失败

### `int RLWin32GetAllWindowHandles(void** outHwnds, int maxCount)`

功能：
- 枚举当前进程内被该分支跟踪的 raylib 窗口句柄

参数：

| 参数 | 说明 |
|---|---|
| `outHwnds` | 输出数组；如果为 `NULL`，只查询数量 |
| `maxCount` | 输出数组容量 |

### `void* RLWin32GetPrimaryWindowHandle(void)`

功能：
- 返回主窗口句柄

返回值：
- 主窗口存在则返回句柄
- 不存在返回 `NULL`

### `int RLWin32IsKnownWindowHandle(void* hwnd)`
判断某个 HWND 是否是当前分支跟踪的 raylib 窗口。

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `hwnd` | `void*` | 待检查窗口句柄 |

返回值：
- 非零表示是已跟踪窗口
- 零表示否

### By-handle 变体
以下 API 面向指定 `HWND`，而不是当前线程的当前窗口：
- `RLWin32SetWindowPropByHandle()`
- `RLWin32GetWindowPropByHandle()`
- `RLWin32RemoveWindowPropByHandle()`
- `RLWin32AddMessageHookByHandle()`
- `RLWin32RemoveMessageHookByHandle()`

## 11. 跨线程调度 API

### `intptr_t RLWin32InvokeOnWindowThreadByHandle(void* hwnd, RLWin32WindowThreadInvoke fn, void* user, int wait)`

功能：
- 在目标窗口所属的 Win32 线程上执行 `fn`

参数：

| 参数     | 说明                |
| ------ | ----------------- |
| `hwnd` | 目标窗口句柄            |
| `fn`   | 回调函数              |
| `user` | 用户数据              |
| `wait` | 非零表示等待完成；0 表示异步投递 |

典型用途：
- 窗口线程亲和的 UI 操作
- 只能在窗口线程执行的 Win32 逻辑

返回值：
- 回调返回值（成功执行时）
- 失败时返回实现定义的失败值，需结合日志与参数校验判断

示例：
```c
static intptr_t UpdateStyle(void* hwnd, void* user)
{
    LONG_PTR style = GetWindowLongPtrW((HWND)hwnd, GWL_STYLE);
    SetWindowLongPtrW((HWND)hwnd, GWL_STYLE, style | WS_MINIMIZEBOX);
    return 1;
}

intptr_t result = RLWin32InvokeOnWindowThreadByHandle(hwnd, UpdateStyle, NULL, 1);
```

### `intptr_t RLWin32InvokeOnWindowThreadByHandleEx(void* hwnd, RLWin32WindowThreadInvoke fn, void* user, int wait, void (*userDtor)(void*))`

功能：
- 在目标窗口所属的 Win32 线程上执行 `fn`，并显式声明用户数据的所有权接管规则

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `hwnd` | `void*` | 目标窗口句柄 |
| `fn` | `RLWin32WindowThreadInvoke` | 回调函数 |
| `user` | `void*` | 用户数据 |
| `wait` | `int` | 非零表示等待完成；0 表示异步投递 |
| `userDtor` | `void (*)(void*)` | 当回调未执行时用于释放 `user` 的析构函数 |

所有权规则：
- 如果派发在执行前被拒绝，API 返回前调用 `userDtor(user)`
- 如果异步请求已经被接受，但在回调真正执行前又被取消，调用 `userDtor(user)`
- 如果回调真正执行，`user` 的所有权转移给回调实现

说明：
- 这是 `RLWin32InvokeOnWindowThreadByHandle()` 的托管所有权版本
- 可通过 `examples/core/core_event_thread_diagnostics.c` 的 `--invoke-owned-selftest` 自动化验证
- 同步路径会保留回调的真实返回值，包括 `0`
- 一旦回调开始执行，`user` 的生命周期就由回调实现自己负责

### `intptr_t RLInvokeOnWindowRenderThreadByHandle(void* hwnd, RLWindowRenderThreadInvoke fn, void* user, int wait)`

功能：
- 在目标窗口的渲染线程上执行 `fn`

> [!WARNING]
> 这不是帧边界安全 API。  
> 它只保证回调在目标渲染线程执行，不保证执行时机位于固定帧边界。

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `hwnd` | `void*` | 目标窗口句柄 |
| `fn` | `RLWindowRenderThreadInvoke` | 回调函数 |
| `user` | `void*` | 用户数据 |
| `wait` | `int` | 同步标志，非时间单位 |

### `intptr_t RLInvokeOnWindowRenderThreadByHandleEx(void* hwnd, RLWindowRenderThreadInvoke fn, void* user, int wait, void (*userDtor)(void*))`

功能：
- 在目标窗口的渲染线程上执行 `fn`，并显式声明用户数据的所有权接管规则

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `hwnd` | `void*` | 目标窗口句柄 |
| `fn` | `RLWindowRenderThreadInvoke` | 回调函数 |
| `user` | `void*` | 用户数据 |
| `wait` | `int` | 非零表示等待完成；0 表示异步投递 |
| `userDtor` | `void (*)(void*)` | 当回调未执行时用于释放 `user` 的析构函数 |

所有权规则：
- 如果派发在执行前被拒绝，API 返回前调用 `userDtor(user)`
- 如果已入队 invoke 在 close / stop 阶段被拒绝而未真正执行，调用 `userDtor(user)`
- 如果回调真正执行，`user` 的所有权转移给回调实现

说明：
- 这是 `RLInvokeOnWindowRenderThreadByHandle()` 的托管所有权版本
- 可通过 `examples/core/core_event_thread_diagnostics.c` 的 `--invoke-owned-selftest` 自动化验证
- 它补齐了异步 invoke 在入队失败或关闭拒绝路径上的 payload 清理语义
- 一旦回调开始执行，`user` 的生命周期就由回调实现自己负责

## 12. 帧边界安全回调 API

### `int RLPostWindowFrameCallbackByHandle(void* hwnd, RLWindowRenderThreadInvoke fn, void* user)`

功能：
- 向普通队列提交一个帧边界安全回调

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `hwnd` | `void*` | 目标窗口句柄 |
| `fn` | `RLWindowRenderThreadInvoke` | 回调函数 |
| `user` | `void*` | 用户数据 |

返回值：
- `1` 成功
- `0` 失败

### `int RLPostWindowFrameCallbackByHandleEx(void* hwnd, RLWindowRenderThreadInvoke fn, void* user, RLFrameCallbackKind kind)`

功能：
- 提交一个显式指定队列类型的帧边界安全回调

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `hwnd` | `void*` | 目标窗口句柄 |
| `fn` | `RLWindowRenderThreadInvoke` | 回调函数 |
| `user` | `void*` | 用户数据 |
| `kind` | `RLFrameCallbackKind` | 队列类型 |

### `int RLPostWindowFrameCallbackByHandleEx2(void* hwnd, RLWindowRenderThreadInvoke fn, void* user, RLFrameCallbackKind kind, void (*userDtor)(void*))`

功能：
- 提交一个在渲染线程执行的帧边界安全回调，并显式声明用户数据所有权处理方式

参数：

| 参数 | 说明 |
|---|---|
| `hwnd` | 目标窗口句柄 |
| `fn` | 回调函数，本体在目标渲染线程上执行 |
| `user` | 用户数据 |
| `kind` | `NORMAL` 或 `CRITICAL` |
| `userDtor` | 如果回调未执行，队列用于清理 `user` 的析构函数 |

返回值：
- `1`：入队成功
- `0`：失败

失败原因包括：
- 参数非法
- 目标窗口正在关闭
- 模式不支持
- 分配失败
- 等待队列槽位超时

线程要求：
- 调用线程不必是目标窗口的渲染线程
- 如果调用线程正是目标渲染线程，而对应队列已满，当前实现会走“立即执行”旁路，而不会阻塞等待自身释放槽位

所有权规则：

| 情况 | `user` 归谁管 |
|---|---|
| 入队失败 | 如果 `userDtor != NULL`，框架负责清理 |
| 已入队但执行前被清队列/关闭 | 如果 `userDtor != NULL`，框架负责清理 |
| 回调成功执行 | 所有权转移给回调实现 |

> [!IMPORTANT]
> 回调已经成功执行后，再由框架调用 `userDtor`是错误行为。这种操作会破坏 API 的所有权契约。所以在执行成功时，所有权和生命周期交由调用方或回调内部自己管理。

补充说明：
- 如果调用线程已经是目标 render thread，而所选队列当前已满，系统可能直接在当前 render thread 立即执行回调。这种情况就叫 `inline fallback`。
- `inline fallback` 仍然属于“回调执行成功”，因此计入执行统计，不计入 dropped 或 cleared。
- 如果回调最终没有执行，框架可能在入队失败、窗口关闭、停止或队列清理阶段调用 `userDtor(user)`。

## 13. 按句柄触发共享清理

### `int RLDeletePendingSharedGpuResourcesByHandle(void* hwnd, int wait)`

功能：
- 请求目标窗口的渲染线程协助清理延迟释放的共享 GPU 对象

参数：

| 参数 | 说明 |
|---|---|
| `hwnd` | 目标窗口句柄 |
| `wait` | 非零等待完成，0 表示异步投递 |

示例：
```c
// 在窗口关闭前，等待目标渲染线程完成共享资源延迟清理
int ok = RLDeletePendingSharedGpuResourcesByHandle(hwnd, 1);
```

## 14. 诊断 API

### `RLEventThreadDiagStats RLGetEventThreadDiagStats(void)`

功能：
- 获取当前窗口或当前上下文的事件线程诊断统计

适用场景：
- 观察事件线程模式下的队列积压、事件泵耗时和帧阶段耗时

示例：
```c
RLEventThreadDiagStats stats = RLGetEventThreadDiagStats();
printf("frameNormalPeak=%u\\n", stats.frameCallbackQueuePeakNormalCount);
```

### `void RLResetEventThreadDiagStats(void)`

功能：
- 重置当前窗口或当前上下文的事件线程诊断统计

说明：
- 只重置 `RLEventThreadDiagStats` 自身拥有的 core event/task/pump 计数字段
- 标记为 `(*)` 的镜像字段组不会被这个 API 重置

Note：
- 仅重置统计计数，不改变运行模式和调度策略。

### `void RLResetEventThreadDiagStatsForCurrentContext(void)`

功能：
- 重置当前窗口或当前上下文的事件线程诊断

适用场景：
- 需要按上下文粒度对齐一次压测窗口。

说明：
- 只重置 `RLEventThreadDiagStats` 自身拥有的 core event/task/pump 计数字段
- 标记为 `(*)` 的镜像字段组不会被这个 API 重置

### `void RLEnableEventDiagStats(void)` / `void RLDisableEventDiagStats(void)` / `bool RLIsEventDiagStatsEnabled(void)`

功能：
- 在运行时启用、禁用或查询事件诊断统计状态

> [!NOTE]
> 只有在构建时打开宏 `RL_EVENT_DIAG_STATS=1` 时，这些统计才真正有效。

补充说明：
- 这组开关控制的是事件线程核心 task/pump 统计
- 标记为 `(*)` 的镜像字段组继续使用各自独立的运行时开关和 reset API
- 关闭后，`RLGetEventThreadDiagStats()` 不再填充这些镜像 `(*)` 字段

### `void RLEnableThreadMismatchDiagStats(void)` / `void RLDisableThreadMismatchDiagStats(void)` / `bool RLIsThreadMismatchDiagStatsEnabled(void)`

功能：
- 在运行时启用、禁用或查询 GPU 写入线程错误的累计诊断统计

说明：
- 只有在构建时打开 `RL_THREAD_MISMATCH_DIAG_STATS=1` 时，这组统计才真正生效
- 这组开关只影响 mismatch 计数，不会关闭错误线程保护、切换或拒绝逻辑

### `RLThreadMismatchDiagStats RLGetThreadMismatchDiagStats(void)`

功能：
- 获取 GPU 写入线程错误的聚合诊断结果

适用场景：
- 检查 GPU 写入 API 是否在错误线程上被调用
- 检查同步切换与延迟切换路径是否频繁出现

### `void RLResetThreadMismatchDiagStats(void)`

功能：
- 重置 GPU 写入线程错误的诊断数据存储

### `void RLSetTrackedObjectDiagFlags(unsigned int flags)` / `unsigned int RLGetTrackedObjectDiagFlags(void)`

功能：
- 设置或读取跟踪对象诊断标志

参数（设置函数）：

| 参数 | 类型 | 说明 |
|---|---|---|
| `flags` | `unsigned int` | `RL_TRACKED_OBJECT_DIAG_*` 按位或组合 |

说明：
- 只有在构建时打开 `RL_TRACKED_OBJECT_DIAG=1` 时，这些诊断标志和 dump/audit 行为才真正生效

### `void RLDebugDumpTrackedObjectState(const char *label)`

功能：
- 把跟踪对象表的状态输出到日志（控制台）

参数：

| 参数 | 类型 | 说明 |
|---|---|---|
| `label` | `const char *` | 本次转储的标签，用于日志检索与对比 |

### `int RLResetEventThreadDiagStatsByHandle(void* hwnd, int wait)`

功能：
- 按窗口句柄重置目标窗口的事件线程诊断

参数：

| 参数     | 类型      | 说明                    |
| ------ | ------- | --------------------- |
| `hwnd` | `void*` | 目标窗口句柄                |
| `wait` | `int`   | 兼容性保留参数，当前实现忽略该值 |

返回值：
- 非零表示目标句柄有效，且 core event/task/pump 计数已重置
- 零表示失败

说明：
- 该 API 会先校验句柄是否属于已知 raylib 窗口
- 只重置 `RLEventThreadDiagStats` 自身拥有的 core event/task/pump 计数字段
- 标记为 `(*)` 的镜像字段组不会被这个 API 重置

示例：
```c
int ok = RLResetEventThreadDiagStatsByHandle(hwnd, 1);
if (!ok)
{
    TRACELOG(RL_LOG_WARNING, "diag reset by handle failed");
}
```
