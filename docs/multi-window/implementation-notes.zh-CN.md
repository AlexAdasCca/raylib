# 实现说明

[English](./implementation-notes.md) | [简体中文](./implementation-notes.zh-CN.md)

> [!IMPORTANT]
> 本页描述的是当前实现细节，不等同于公开稳定契约。  
> 公开契约请以 `raylib.h` 注释和本目录的 `overview.zh-CN.md`、`api-reference.zh-CN.md` 为准。

## 1. 实现分层与文件定位

| 层 | 主要文件 | 说明 |
| --- | --- | --- |
| 公共契约层 | `src/raylib.h` | 公开类型、枚举、函数声明、契约注释 |
| 上下文模型层 | `src/rl_context.h`, `src/rl_context.cpp` | `RLContext` 生命周期、共享配置状态、当前上下文模型 |
| 共享 GPU 层 | `src/rl_shared_gpu.h`, `src/rl_shared_gpu.cpp` | 共享组、共享对象生命周期、共享着色器协调、所有权元数据 |
| 跟踪对象层 | `src/rcore.c`, `src/rl_object_tracker.h` | 跟踪表、作用域提升、所有权跟踪、诊断输出 |
| 平台集成层 | `src/platforms/rcore_desktop_glfw.c` | 事件线程、渲染线程调度、帧回调队列、Win32 线程亲和包装 |
| Win32 原生队列层 | `src/external/glfw/src/win32_window.c`, `src/external/glfw/src/win32_platform.h` | 原生线程任务队列、消息唤醒与任务投递 |
| 同步原语层 | `src/rglfwglobal.h`, `src/rglfwglobal.cpp` | 线程、事件、信号量、全局锁、关闭唤醒语义 |

## 2. 诊断测试宏与预处理器开关

### 2.1 构建期宏

| 宏 | 默认值 | 定义位置 | 作用 |
| --- | --- | --- | --- |
| `RL_EVENT_DIAG_STATS` | `1` | [`src/config.h:315`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:315) | 编译事件线程诊断统计代码。关闭后相关 `RL_DIAG_*` 宏退化为空操作。 |
| `RL_EVENTTHREAD_COALESCE_STATE` | `1` | [`src/config.h:326`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:326) | 启用高频输入状态合并（邮箱语义），降低任务队列压力。 |
| `RLGLFW_DIAGNOSTICS` | 构建相关 | [`src/platforms/rcore_desktop_glfw.c:116`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:116) | 启用平台层诊断断言。 |

> [!NOTE]
> `RL_EVENT_DIAG_STATS` 是编译期开关。  
> `RLEnableEventDiagStats()` 和 `RLDisableEventDiagStats()` 是运行期开关，只有在编译期已开启诊断统计时才生效。

### 2.2 帧回调队列调优宏

定义位置：[`src/platforms/rcore_desktop_glfw.c:128`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:128)

| 宏 | 默认值 | 作用 |
| --- | --- | --- |
| `RL_FRAME_CALLBACK_NORMAL_QUEUE_CAPACITY` | `1024` | 普通队列容量 |
| `RL_FRAME_CALLBACK_CRITICAL_QUEUE_CAPACITY` | `256` | 关键队列容量 |
| `RL_FRAME_CALLBACKS_PER_FRAME_LIMIT` | `64` | 每帧最多执行的回调总数 |
| `RL_FRAME_CALLBACKS_BATCH_POP_LIMIT` | `8` | 单次加锁最多弹出的批量数量 |
| `RL_FRAME_CALLBACKS_CRITICAL_MIN_PER_FRAME` | `8` | 每帧关键回调最小目标执行数 |
| `RL_FRAME_CALLBACK_WAIT_CRITICAL_MS` | `50` | 关键队列槽位等待超时，单位毫秒 |
| `RL_FRAME_CALLBACK_WAIT_NORMAL_MS` | `10` | 普通队列槽位等待超时，单位毫秒 |

### 2.3 内部诊断宏与代码上下文

宏定义位置：[`src/rcore.c:2175`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rcore.c:2175)

```c
#define RL_DIAG_TASK_POSTED()      RLDiag_OnTaskPosted()
#define RL_DIAG_TASK_POST_FAILED() RLDiag_OnTaskPostFailed()
#define RL_DIAG_TASK_EXECUTED()    RLDiag_OnTaskExecuted()
#define RL_DIAG_PUMP_BEGIN()       RLDiag_PumpBegin()
#define RL_DIAG_PUMP_END()         RLDiag_PumpEnd()
```

平台层调用上下文：[`src/platforms/rcore_desktop_glfw.c:899`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:899)

```c
RL_DIAG_PUMP_BEGIN();                           // 进入一次事件泵统计区间
...                                             // 执行任务和消息泵
unsigned int executedTaskCount = RL_DIAG_PUMP_END();
RL_DIAG_ON_PUMP(RLGetTime() - pumpStartTime, executedTaskCount); // 记录耗时与任务数
```

负载类型枚举位置：[`src/rcore.c:1921`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rcore.c:1921)

主要类别包括：
- `RL_DIAG_PAYLOAD_MOUSEMOVE`
- `RL_DIAG_PAYLOAD_MOUSEWHEEL`
- `RL_DIAG_PAYLOAD_MOUSEBUTTON`
- `RL_DIAG_PAYLOAD_KEY`
- `RL_DIAG_PAYLOAD_CHAR`
- `RL_DIAG_PAYLOAD_WINPOS`
- `RL_DIAG_PAYLOAD_FBSIZE`
- `RL_DIAG_PAYLOAD_SCALE`
- `RL_DIAG_PAYLOAD_DROP`
- `RL_DIAG_PAYLOAD_WINCLOSE`
- `RL_DIAG_PAYLOAD_OTHER`

### 2.4 诊断宏与开关的“定义位置 + 使用位置”总览

| 分类   | 名称                                 | 定义位置                                                                                    | 主要使用位置                                                                                                                                                                                                                                                                 | 说明                       |
| ---- | ---------------------------------- | --------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------ |
| 构建开关 | `RL_EVENT_DIAG_STATS`              | [`src/config.h:315`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:315)   | [`src/rcore.c:1886`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rcore.c:1886), [`src/platforms/rcore_desktop_glfw.c:899`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:899)                                               | 控制事件线程诊断统计代码是否编译进入二进制    |
| 构建开关 | `RL_EVENTTHREAD_COALESCE_STATE`    | [`src/config.h:326`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:326)   | [`src/platforms/rcore_desktop_glfw.c:4097`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:4097)                                                                                                                                    | 控制高频状态事件是否按“合并/覆盖”策略入队   |
| 诊断标志 | `RL_TRACKED_OBJECT_DIAG_*`         | [`src/raylib.h:2163`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/raylib.h:2163) | [`src/rcore.c:5236`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rcore.c:5236)                                                                                                                                                                                  | 控制 tracked-object 诊断输出级别 |
| 内部宏  | `RL_DIAG_TASK_*`, `RL_DIAG_PUMP_*` | [`src/rcore.c:2175`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rcore.c:2175)   | [`src/platforms/rcore_desktop_glfw.c:899`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:899), [`src/platforms/rcore_desktop_glfw.c:2552`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:2552) | 统计任务投递、失败、执行与消息泵耗时       |
| 负载分类 | `RL_DIAG_PAYLOAD_*`                | [`src/rcore.c:1921`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rcore.c:1921)   | [`src/platforms/rcore_desktop_glfw.c:4033`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:4033)                                                                                                                                    | 统一统计输入和窗口事件类别            |

## 3. 诊断测试入口与定位

| 启动参数 | 示例程序 | 定义位置 | 验证目标 |
| --- | --- | --- | --- |
| `--trace-reentry-selftest` | `core_shared_gpu_context` | [`examples/core/core_shared_gpu_context.c:578`](G:/C、C++ Programing/jackalclient/raylib/raylib/examples/core/core_shared_gpu_context.c:578) | 验证日志回调重入隔离是否正确。 |
| `--queue-saturation-selftest` | `core_event_thread_diagnostics` | [`examples/core/core_event_thread_diagnostics.c:1980`](G:/C、C++ Programing/jackalclient/raylib/raylib/examples/core/core_event_thread_diagnostics.c:1980) | 验证有界队列在压力下的等待、失败和关闭行为。 |
| `--semaphore-selftest` | `core_event_thread_diagnostics` | [`examples/core/core_event_thread_diagnostics.c:1985`](G:/C、C++ Programing/jackalclient/raylib/raylib/examples/core/core_event_thread_diagnostics.c:1985) | 验证 `ReleaseOne` 与 `Close` 的唤醒语义。 |

## 4. 队列实现细节

### 4.1 Win32 原生任务队列

核心实现位置：[`src/external/glfw/src/win32_window.c:538`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/external/glfw/src/win32_window.c:538)

要点：
1. 队列是有界的，失败路径有明确计数。
2. 任务带类别元数据，用于压力下行为决策。
3. 通过派发消息唤醒阻塞等待和模态循环。

代码上下文（带行注释）：

```c
// 1) 先尝试放入队列，失败时由调用者拿到明确返回值
int enqueued = _glfwPostTaskWin32Ex(window, &task, taskClass, options);
if (!enqueued) return 0;

// 2) 投递唤醒消息，驱动目标线程处理任务队列
PostMessageW(window->win32.handle, WM_NULL, 0, 0);
```

说明：
1. 当前实现已避免“静默成功但任务丢失”的语义。
2. 队列压力必须通过返回值和诊断统计向上反馈。

### 4.2 帧回调队列

状态定义位置：[`src/platforms/rcore_desktop_glfw.c:225`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:225)

关键结构：
1. `normalFrameCallbackRing`
2. `criticalFrameCallbackRing`
3. `normalFrameCallbackSlotsAvailableSemaphore`
4. `criticalFrameCallbackSlotsAvailableSemaphore`
5. `frameCallbackQueuedHint`
6. `frameCallbackCriticalQueuedHint`

排空逻辑位置：[`RLGlfwDrainFrameCallbacksCurrentContext`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:2949)

代码上下文示例：

```c
unsigned int callbacksCriticalTarget = ...;     // 本帧关键回调最小目标
while (callbacksExecuted < RL_FRAME_CALLBACKS_PER_FRAME_LIMIT)
{
    RLRenderFrameCallbackSlot callbackBatch[RL_FRAME_CALLBACKS_BATCH_POP_LIMIT] = { 0 };
    ...
    RLGlfwGlobalLock();                         // 仅在加锁区弹出任务
    while (callbackBatchCount < batchBudget)
    {
        // 关键回调配额未满足时，优先弹出关键回调
        // 正常情况下优先弹出普通回调
        // 普通为空时回退到关键回调
    }
    RLGlfwGlobalUnlock();

    // 锁外执行回调，避免把用户回调放在全局锁作用域内
}
```

补充说明：
1. “批量弹出 + 锁外执行”是为了缩小全局锁作用域，降低锁竞争和回调重入风险。
2. `NORMAL` 与 `CRITICAL` 使用独立有界队列，避免关键清理任务被普通负载长期阻塞。
3. 每个队列有独立槽位信号量，等待语义不再依赖单一事件对象。

### 4.3 关键内部路径：按句柄提交帧回调

位置：[`src/platforms/rcore_desktop_glfw.c:3030`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:3030)

代码上下文（简化，带说明）：

```c
// 1) 计算队列类型和等待预算
unsigned int waitTimeoutMs = RLGlfwGetFrameCallbackWaitTimeoutMs(kind);

// 2) 队列满时，先等本队列槽位；等待失败则返回 0
if (!RLSemaphoreWaitTimed(*slotsSemaphore, waitTimeoutMs)) return 0;

// 3) 入队时仅做数据搬运和计数更新，不执行用户回调
ring->slots[ring->tail] = slot;
ring->tail = (ring->tail + 1U) % ring->capacity;
ring->count++;

// 4) 设置 backlog hint，提示渲染线程需要 drain
atomic_store_explicit(hint, 1, memory_order_release);
```

该路径的第一性约束：
1. 调用者拿到成功返回值，必须意味着该回调已进入目标队列。
2. 回调执行时机固定在渲染线程帧边界，不在提交线程直接执行（自阻塞旁路除外）。
3. 失败路径必须保持 `user/userDtor` 契约完整。

## 5. 共享资源实现关键点

### 5.1 `gpuShareGroup` 生命周期与跟踪作用域策略解耦

位置：[`src/rl_shared_gpu.cpp:104`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rl_shared_gpu.cpp:104)

结论：
- 共享组存在不等于跟踪作用域已经是 `share_group`。
- 生命周期 pin 与作用域策略是两套机制。

### 5.2 作用域提升纳入绑定契约

位置：[`src/rl_shared_gpu.cpp:623`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rl_shared_gpu.cpp:623)

结论：
- 提升失败即绑定失败。
- 不允许“GPU 共享已建立但跟踪作用域未迁移”的半成功状态。

### 5.3 所有权转移与认领是事务提交

位置：[`src/rcore.c:3752`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rcore.c:3752)

结论：
- 共享元数据与跟踪元数据同步提交。
- 不会出现单边成功后 API 仍返回成功的情况。

## 6. 日志回调隔离

隔离入口：
- [`RLTraceCallbackIsolationEnter`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rl_context.h:142)
- [`RLTraceCallbackIsolationLeave`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rl_context.h:143)

覆盖锁域：
1. 跟踪对象锁
2. 共享组锁
3. 绑定锁
4. GLFW 全局锁

作用：
- 锁内日志不进入用户回调，避免重入和死锁。

## 7. 建议重点审查点

1. 混合负载下的队列饱和行为。
2. 关闭期间等待者唤醒与失败路径完整性。
3. 上下文销毁并发时的所有权事务一致性。
4. 作用域提升冲突日志与绑定失败传播。
5. 高频输入下诊断统计开销对帧稳定性的影响。
