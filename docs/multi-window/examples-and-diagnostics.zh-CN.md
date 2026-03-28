# 示例与诊断

[English](./examples-and-diagnostics.md) | [简体中文](./examples-and-diagnostics.zh-CN.md)

## 1. 示例程序索引

| 示例 | 主要验证点 |
| --- | --- |
| `examples/core/core_shared_gpu_context.c` | 共享上下文、共享着色器、所有权转移与认领、日志回调重入隔离 |
| `examples/core/core_event_thread_diagnostics.c` | 事件线程、队列压力、关闭等待者、信号量行为 |
| `examples/core/core_glfw_refresh_callback_diagnostics.c` | Win32 模态循环刷新回调 |

## 2. 关键自测入口

### `core_shared_gpu_context --trace-reentry-selftest`
验证：
1. 用户日志回调是否递归重入。
2. 锁内日志是否正确回退到默认输出通道。

### `core_event_thread_diagnostics --queue-saturation-selftest`
验证：
1. 原生任务队列在压力下的行为。
2. 帧回调双队列在压力和关闭过程中的行为。

### `core_event_thread_diagnostics --semaphore-selftest`
验证：
1. `RLSemaphoreReleaseOne()` 是否单次唤醒一个等待者。
2. `Close()` 是否广播唤醒并使等待者失败返回。

## 3. 诊断 API 用法

### 事件线程诊断
启用：
```c
RLEnableEventDiagStats();
```

读取：
```c
RLEventThreadDiagStats stats = RLGetEventThreadDiagStats();
```

建议重点字段：
- `nativeTaskQueue*`
- `frameCallbackQueue*`
- `pump*`
- `swapCost*`
- `waitCost*`
- `frameCpu*`

### 跟踪对象诊断
配置示例：
```c
RLSetTrackedObjectDiagFlags(
    RL_TRACKED_OBJECT_DIAG_LOG_RELEASE_CALLS |
    RL_TRACKED_OBJECT_DIAG_DUMP_STATE_ON_RELEASE_MISS |
    RL_TRACKED_OBJECT_DIAG_INCLUDE_TOMBSTONES |
    RL_TRACKED_OBJECT_DIAG_LOG_PROMOTIONS |
    RL_TRACKED_OBJECT_DIAG_AUDIT_PROMOTIONS);
```

转储：
```c
RLDebugDumpTrackedObjectState("before-close");
```

### 线程错配诊断
```c
RLThreadMismatchDiagStats stats = RLGetThreadMismatchDiagStats();
```

建议关注：
- `detectedCount`
- `handoff*`
- `deferred*`
- `lastApi`

## 4. 诊断测试宏与预处理器开关

### 构建开关

| 宏 | 默认值 | 定义位置 | 作用 |
| --- | --- | --- | --- |
| `RL_EVENT_DIAG_STATS` | `1` | [`src/config.h:315`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:315) | 开启事件线程诊断统计实现。 |
| `RL_EVENTTHREAD_COALESCE_STATE` | `1` | [`src/config.h:326`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:326) | 开启高频输入合并策略。 |

### 跟踪对象诊断标志

定义位置：[`src/raylib.h:2163`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/raylib.h:2163)

1. `RL_TRACKED_OBJECT_DIAG_LOG_RELEASE_CALLS`
2. `RL_TRACKED_OBJECT_DIAG_DUMP_STATE_ON_RELEASE_MISS`
3. `RL_TRACKED_OBJECT_DIAG_INCLUDE_TOMBSTONES`
4. `RL_TRACKED_OBJECT_DIAG_LOG_PROMOTIONS`
5. `RL_TRACKED_OBJECT_DIAG_AUDIT_PROMOTIONS`

### 内部诊断宏

定义位置：[`src/rcore.c:2175`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rcore.c:2175)

```c
#define RL_DIAG_TASK_POSTED()      RLDiag_OnTaskPosted()
#define RL_DIAG_TASK_POST_FAILED() RLDiag_OnTaskPostFailed()
#define RL_DIAG_TASK_EXECUTED()    RLDiag_OnTaskExecuted()
#define RL_DIAG_PUMP_BEGIN()       RLDiag_PumpBegin()
#define RL_DIAG_PUMP_END()         RLDiag_PumpEnd()
```

平台层调用上下文：[`src/platforms/rcore_desktop_glfw.c:899`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:899)

## 5. 回归建议

每次修改以下模块后，至少执行：
1. `core_shared_gpu_context --trace-reentry-selftest`
2. `core_event_thread_diagnostics --queue-saturation-selftest`
3. `core_event_thread_diagnostics --semaphore-selftest`

并手动验证：
1. 事件线程模式下的创建与关闭。
2. 共享窗口创建与关闭。
3. 所有权转移与反向转移。
4. 最小化与恢复行为。
5. 模态循环刷新回调。
