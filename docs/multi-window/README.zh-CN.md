# 多窗口支持文档（Win32/GLFW）

[English](./README.md) | [简体中文](./README.zh-CN.md)

> [!IMPORTANT]
> 本文档系列仅描述当前分支在 `PLATFORM_DESKTOP_GLFW WIN32` 上已经实现并验证过的多窗口、事件线程、跨线程调度、共享上下文与诊断能力。
>
> 它不是通用 raylib 文档，也不自动代表其他平台或其他后端具备相同语义。

## 目标

这套文档用于回答四类问题：

1. 这个分支到底支持什么，不支持什么。
2. 每个 API 应该在什么线程、什么生命周期阶段调用。
3. 共享资源、跨线程调度、队列、诊断这些机制的契约是什么。
4. 代码实现分布在哪些文件，哪些属于稳定接口，哪些只是当前实现细节。

## 建议阅读顺序

1. [overview.zh-CN.md](./overview.zh-CN.md)
2. [threading-and-lifecycle.zh-CN.md](./threading-and-lifecycle.zh-CN.md)
3. [resource-sharing.zh-CN.md](./resource-sharing.zh-CN.md)
4. [api-reference.zh-CN.md](./api-reference.zh-CN.md)
5. [runtime-scenarios.zh-CN.md](./runtime-scenarios.zh-CN.md)
6. [examples-and-diagnostics.zh-CN.md](./examples-and-diagnostics.zh-CN.md)
7. [implementation-notes.zh-CN.md](./implementation-notes.zh-CN.md)
8. [references.zh-CN.md](./references.zh-CN.md)

## 文档分工

| 文档                                  | 作用                      | 读者      |
| ----------------------------------- | ----------------------- | ------- |
| `overview.zh-CN.md`                 | 快速理解支持范围、限制、推荐用法        | 所有人     |
| `threading-and-lifecycle.zh-CN.md`  | 线程契约、生命周期、合法/非法调用模式     | 使用者、审阅者 |
| `resource-sharing.zh-CN.md`         | 共享上下文、共享组、所有权、作用域提升     | 使用者、维护者 |
| `api-reference.zh-CN.md`            | API、枚举、结构、回调类型的详细说明     | 使用者、维护者 |
| `runtime-scenarios.zh-CN.md`        | 关键时序场景、操作步骤、预期行为        | 使用者、测试者 |
| `examples-and-diagnostics.zh-CN.md` | 示例程序的索引、诊断方法、回归测试       | 测试者、维护者 |
| `implementation-notes.zh-CN.md`     | 实现分层、锁、队列、同步的底层原理以及源码位置 | 维护者、审阅者 |
| `references.zh-CN.md`               | 参考的官方文档与模板来源            | 维护者     |

## 快速结论

### 当前最重要的稳定规则

1. `RLContextSetResourceShareMode()` 必须在 `RLInitWindow()` / `RLInitWindowEx()` 之前调用。且对端上下文必须已经创建且持有窗口（即RLInitWindow或类似调用已经成功，且窗口存活）。
2. 在 `RLContextSetResourceShareMode()` 上使用 `RL_CONTEXT_SHARE_WITH_PRIMARY` 和 `RL_CONTEXT_SHARE_WITH_CONTEXT` 失败时，不会自动回退到 `RL_CONTEXT_SHARE_NONE`。API 会明确返回错误号码。
3. `RLDestroyContext()` 不会隐式调用 `RLCloseWindow()`，所以用户必须在结束上下文之前调用 `RLCloseWindow()` 关闭窗口。
4. `RLInvokeOnWindowRenderThreadByHandle()` 是将任务调度到渲染线程的接口，但其不保证调用发生在固定帧边界内。
5. `RLPostWindowFrameCallbackByHandle*()` 保证在目标窗口 `RLEndDrawing()` 以内固定点执行的帧边界安全回调接口。
6. `RL_E_FLAG_WINDOW_EVENT_THREAD` 模式下，一个渲染线程只支持创建一个活动窗口。
7. 在公共 API 中，共享资源所有权的转移与认领是原子事务，共享元数据与跟踪元数据在迁移失败时会立即失败并回滚，操作是原子性和同步的。

### 快速入门示例

| 需求                          | 示例                                                       |
| --------------------------- | -------------------------------------------------------- |
| 共享上下文、共享着色器、所有权转移、日志回调      | `examples/core/core_shared_gpu_context.c`                |
| 事件线程、队列压力测试、队列关闭等待者、UI 性能测试 | `examples/core/core_event_thread_diagnostics.c`          |
| Win32 模态循环刷新回调              | `examples/core/core_glfw_refresh_callback_diagnostics.c` |

## 文档风格约定

本文档系列统一使用：
- GitHub Markdown 表格
- GitHub 警示块，例如 `> [!IMPORTANT]`、`> [!WARNING]`、`> [!NOTE]`
- 顶部语言切换选项

## 下一步

查看以下文档页，快速开始：
- [overview.zh-CN.md](./overview.zh-CN.md)
- [api-reference.zh-CN.md](./api-reference.zh-CN.md)

查看以下文档页，深入了解底层细节：
- [threading-and-lifecycle.zh-CN.md](./threading-and-lifecycle.zh-CN.md)
- [resource-sharing.zh-CN.md](./resource-sharing.zh-CN.md)
- [implementation-notes.zh-CN.md](./implementation-notes.zh-CN.md)
