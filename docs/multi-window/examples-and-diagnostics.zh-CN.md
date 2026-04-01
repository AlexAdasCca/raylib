# 示例与诊断

[English](./examples-and-diagnostics.md) | [简体中文](./examples-and-diagnostics.zh-CN.md)

## 1. 示例程序索引

| 示例 | 主要验证点 |
| --- | --- |
| `examples/core/core_shared_gpu_context.c` | 共享上下文、共享着色器、所有权转移与认领、日志回调重入隔离 |
| `examples/core/core_event_thread_diagnostics.c` | 事件线程、队列压力、关闭等待者、信号量行为 |
| `examples/core/core_memdiag_event_automation.c` | 自动化输入回放下的内存诊断统计与关闭阶段泄漏检查 |
| `examples/core/core_long_path_utf8_fileio.c` | 长路径、UTF-8 路径、文件读写与路径解析辅助函数 |
| `examples/audio/audio_module_playing.c` | 音频模块在多窗口/多线程场景下的窗口、消息钩子和关闭行为 |
| `examples/core/core_glfw_refresh_callback_diagnostics.c` | Win32 模态循环刷新回调 |

## 2. 关键自测入口

### `core_shared_gpu_context --trace-reentry-selftest`
验证：
1. 用户日志回调是否递归重入。
2. 锁内日志是否正确回退到默认输出通道。

### `core_shared_gpu_context --shared-gpu-diag-scope-selftest`
验证：
1. 当前共享组快照接口是否能稳定读取。
2. strict 模式全局 reject 计数是否独立于当前共享组累计字段。
3. 当前共享组 reset 与 reject reset 是否各自只作用自己的作用域。

### `core_event_thread_diagnostics --queue-saturation-selftest`
验证：
1. 原生任务队列在压力下的行为。
2. 帧回调双队列在压力和关闭过程中的行为。

### `core_event_thread_diagnostics --semaphore-selftest`
验证：
1. `RLSemaphoreReleaseOne()` 是否单次唤醒一个等待者。
2. `Close()` 是否广播唤醒并使等待者失败返回。

### `core_event_thread_diagnostics --invoke-owned-selftest`
验证：
1. window-thread invoke 的 payload 所有权转移是否正确。
2. render-thread invoke 的 payload 所有权转移是否正确。
3. 失败路径是否正确调用用户析构回调。

### `core_event_thread_diagnostics --native-task-queue-selftest`
验证：
1. 轻量 native task queue getter 是否返回一致结果。
2. by-handle reset 是否能清零对应 render-thread 队列统计。
3. 读取动作本身不会污染队列计数。

### `core_event_thread_diagnostics --frame-callback-reset-all-selftest`
验证：
1. 两个 event-thread 窗口的 frame-callback 累计统计都能增长。
2. `RLResetAllFrameCallbackDiagStats()` 会同时清零两边窗口的累计统计。

### `core_memdiag_event_automation`
验证：
1. 自动化输入脚本驱动下的内存分配/释放统计。
2. 关闭和销毁阶段是否最终输出 `NO_LEAK_DETECTED`。

### `core_long_path_utf8_fileio [rootOverride] [uncRootOverride]`
验证：
1. 长路径目录创建。
2. UTF-8 路径下的文本和二进制读写。
3. 长路径目录枚举。
4. 长路径重命名与移动。
5. 文件/目录存在性检查、工作目录切换和路径解析辅助函数。
6. 可选 UNC 根路径访问。

说明：
- 程序会输出一行 JSON 汇总结果。
- `renameOk`、`moveOk` 和 `listCountOk` 现在都属于严格通过字段。
- `movedPathLen` 表示 rename + move 完成后的目标路径长度。

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

### Shared-GPU 诊断
读取：
```c
RLSharedGpuGroupDiagStats groupStats = RLGetCurrentSharedGpuGroupDiagStats();
RLSharedGpuTrackingRejectDiagStats rejectStats = RLGetSharedGpuTrackingRejectDiagStats();
```

建议关注：
- 当前共享组的 `live*` / `pending*` / `owner*`
- 共享组累计字段：
  - `releaseUntrackedCount`
  - `framebufferMapHitCount`
  - `framebufferMapMissCount`
  - `framebufferReleaseSkippedCount`
- 全局 reject 字段：
  - `unregisteredRetainRejectCount`
  - `unregisteredReleaseRejectCount`

## 4. 诊断测试宏与预处理器开关

### 构建开关

注意：诊断宏开启后，代码中将包含大量诊断代码和数据存储，会导致内存占用上升并轻微影响性能。为了性能，不要频繁使用 API 获取诊断信息。

| 宏 | 默认值 | 定义位置 | 作用 |
| --- | --- | --- | --- |
| `RL_MEM_DIAG` | 当前诊断构建通常为 `1` | 编译参数 / [`src/raylib.h`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/raylib.h) | 开启内存诊断记录和 `RLMemoryDiagStats` API。 |
| `RL_MEM_DIAG_FILELINE` | 当前诊断构建通常为 `1` | 编译参数 / [`src/raylib.h`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/raylib.h) | 为内存诊断记录补充文件/行号来源。 |
| `RL_EVENT_DIAG_STATS` | `1` | [`src/config.h:315`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:315) | 开启事件线程诊断统计实现。 |
| `RL_EVENTTHREAD_COALESCE_STATE` | `1` | [`src/config.h:326`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:326) | 开启高频输入合并策略。 |
| `RL_THREAD_MISMATCH_DIAG_STATS` | `1` | [`src/config.h:321`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:321) | 开启错线程 GPU 写入诊断统计。 |
| `RL_FRAME_CALLBACK_DIAG_STATS` | `1` | [`src/config.h:327`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:327) | 开启 frame-callback 累计诊断统计。 |
| `RL_SHARED_GPU_DIAG_STATS` | `1` | [`src/config.h:333`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:333) | 开启 shared-GPU 累计诊断统计。 |
| `RL_TRACKED_OBJECT_DIAG` | `1` | [`src/config.h:339`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:339) | 开启 tracked-object dump / audit / log 诊断辅助。 |

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

每次修改多窗口实现后，需要执行以下测试：
1. `core_shared_gpu_context --trace-reentry-selftest`
2. `core_shared_gpu_context --shared-gpu-diag-scope-selftest`
3. `core_event_thread_diagnostics --queue-saturation-selftest`
4. `core_event_thread_diagnostics --semaphore-selftest`
5. `core_event_thread_diagnostics --invoke-owned-selftest`
6. `core_event_thread_diagnostics --native-task-queue-selftest`
7. `core_event_thread_diagnostics --frame-callback-reset-all-selftest`
8. `core_memdiag_event_automation`
9. `core_long_path_utf8_fileio`

需要手动验证以下功能：
1. 事件线程模式下的创建与关闭。
2. 共享窗口创建与关闭。
3. 所有权转移与反向转移。
4. 最小化与恢复行为。
5. 模态循环刷新回调。
6. 音频示例的主窗口/线程窗口启动与关闭。
