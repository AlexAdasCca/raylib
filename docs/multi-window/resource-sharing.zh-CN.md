# 资源共享与共享组语义

[English](./resource-sharing.md) | [简体中文](./resource-sharing.zh-CN.md)

## 1. 这部分到底解决什么问题

共享上下文绝不是在 GLFW 创建窗口时传入一个共享窗口句柄这么简单。

该功能需要同时满足：
1. GPU 对象共享命名空间
2. 共享组级别的资源生命周期
3. 延迟删除
4. 跟踪对象的本地作用域与共享作用域切换
5. 所有权转移与孤立对象认领
6. 跨线程共享着色器使用时的同步问题

## 2. 两个必须分开的概念

### 2.1 共享组生命周期

内部共享组负责：
- 共享对象引用计数
- 共享所有权元数据
- 孤立对象的所有权元数据
- 延迟释放
- 共享着色器使用范围和围栏
- 其他共享组级状态

### 2.2 跟踪对象作用域

对象跟踪器关心的是：
- 当前对象是按 `context` 作用域还是按 `share_group` 作用域进行跟踪

> [!IMPORTANT]
> 这两个概念相关，但不是一回事。  
> 存在内部共享组不等于跟踪作用域一定是共享组。

## 3. 共享模式

## 3.1 `RL_CONTEXT_SHARE_NONE`

含义：
- 不与其他上下文显式共享 GPU 命名空间

注意：
- 当前实现下，内部仍可能创建共享组结构来管理延迟删除和生命周期。
- 这不代表跟踪对象命名空间已经共享。
- 跟踪作用域仍然是 `context` 级，直到它真正加入共享资源命名空间。

## 3.2 `RL_CONTEXT_SHARE_WITH_PRIMARY`

含义：
- 与进程中的主窗口上下文共享 GPU 资源

前置条件：
- 主窗口已经存在并初始化完成

失败语义：
- 函数返回失败，需要用户自己处理是实验独立上下文重试还是直接报错。

## 3.3 `RL_CONTEXT_SHARE_WITH_CONTEXT`

含义：
- 与指定 `RLContext` 对应的窗口共享 GPU 资源

前置条件：
- `shareWith != NULL`
- `shareWith != ctx`
- `shareWith` 已拥有已初始化的窗口

失败语义：
- 函数返回失败，需要用户自己处理是实验独立上下文重试还是直接报错。

## 4. 共享配置校验

公开校验 API：
- `RLContextValidateResourceShareConfig()`
- `RLContextIsResourceShareConfigValid()`
- `RLContextGetResourceShareValidationError()`

### 校验错误码

| 错误码                                                      | 含义                |
| -------------------------------------------------------- | ----------------- |
| `RL_CONTEXT_SHARE_VALIDATION_OK`                         | 校验通过              |
| `RL_CONTEXT_SHARE_VALIDATION_CTX_NULL`                   | `ctx == NULL`     |
| `RL_CONTEXT_SHARE_VALIDATION_INVALID_MODE`               | 非法共享模式            |
| `RL_CONTEXT_SHARE_VALIDATION_TARGET_NULL`                | 显式共享但目标为空         |
| `RL_CONTEXT_SHARE_VALIDATION_TARGET_SELF`                | 目标上下文被错误地设置为自身    |
| `RL_CONTEXT_SHARE_VALIDATION_TARGET_WINDOW_UNAVAILABLE`  | 要请求共享的目标上下文尚未创建窗口 |
| `RL_CONTEXT_SHARE_VALIDATION_PRIMARY_WINDOW_UNAVAILABLE` | 主窗口暂不可用           |

### 推荐使用方式

```c
RLContextSetResourceShareMode(workerContext, RL_CONTEXT_SHARE_WITH_PRIMARY, NULL);

if (!RLContextValidateResourceShareConfig(workerContext))
{
    int errorCode = RLContextGetResourceShareValidationError(workerContext);
    // 根据错误码决定是否终止创建，或切换到 SHARE_NONE 后重试
}
```

## 5. 跟踪作用域提升

## 5.1 为什么需要作用域提升

典型场景：
1. 主窗口先以 `RL_CONTEXT_SHARE_NONE` 创建
2. 已经产生了一批跟踪对象
3. 后面另一个窗口通过 `WITH_PRIMARY` 加入共享命名空间

如果此时旧对象仍然留在 `context` 作用域：
- 新共享窗口按 `share_group` 作用域看不到它们
- 所有权、释放与认领语义就会分裂

所以需要做作用域提升：
- 把旧的跟踪条目从 `context` 作用域迁移到 `share_group` 作用域

## 5.2 作用域提升契约

当前实现中：
- 作用域提升是绑定成功的一部分
- 如果作用域提升失败，绑定失败
- 不允许出现“GPU 共享已经建立，但跟踪作用域仍停留在 `context`”的半成功状态

> [!WARNING]
> 请注意在现阶段，此设定是刻意的。 

## 5.3 单向升级

作用域策略一旦升级到 `share_group`，不会在该生命周期内回退到 `context`。

原因：
- 回退会迫使跟踪条目来回迁移
- 会引入历史对象、孤立所有权、所有者和延迟删除的解释歧义

## 6. 共享组生命周期 API

这些 API 操作的是共享组级别的 GPU 对象生命周期，而不是高层 CPU 侧对象：

- `RLSharedRetainShader()` / `RLSharedReleaseShader()`
- `RLSharedRetainBuffer()` / `RLSharedReleaseBuffer()`
- `RLSharedRetainVertexArray()` / `RLSharedReleaseVertexArray()`
- `RLSharedRetainFramebuffer()` / `RLSharedReleaseFramebuffer()`
- `RLSharedRetainFramebufferBase()` / `RLSharedReleaseFramebufferBase()`
- `RLSharedRetainRenderbuffer()` / `RLSharedReleaseRenderbuffer()`
- `RLDeletePendingSharedGpuResources()`
- `RLDeletePendingSharedGpuResourcesByHandle()`

### 使用场景

#### 共享着色器
一个着色器还会被另一个共享窗口继续使用时：
```c
RLSharedRetainShader(shader);
```

当一个窗口不再需要它：
```c
RLSharedReleaseShader(shader);
```

#### 按句柄清理
当你需要让指定窗口的渲染线程协助（清理待删除的共享 GPU 对象）时：
```c
RLDeletePendingSharedGpuResourcesByHandle(targetHwnd, 1);
```

## 7. 共享着色器协调

### 公开 API
- `RLBeginSharedShaderUse()`
- `RLSharedShaderUseEnd()`
- `RLConfigureSharedShaderFenceWait()`

### 策略枚举

| 值 | 含义 |
|---|---|
| `RL_SHARED_SHADER_USE_PHASED` | 公平串行化，按票据顺序 |
| `RL_SHARED_SHADER_USE_LOCKED` | 简单互斥串行化 |

### 正确的调用顺序

```c
if (RLBeginSharedShaderUse(shader, RL_SHARED_SHADER_USE_LOCKED))
{
    RLBeginShaderMode(shader);
    // draw...
    RLEndShaderMode();
    RLSharedShaderUseEnd(shader, RL_SHARED_SHADER_USE_LOCKED);
}
```

> [!IMPORTANT]
> `RLBeginSharedShaderUse()` / `RLSharedShaderUseEnd()` 不是普通的函数。  
> 对跨上下文、跨线程共享的着色器，这种写法是保障并发上下文安全的标准方式。

## 8. 所有权元数据

当前分支有两层所有权元数据：

1. 共享组内部的共享所有权元数据
2. 对象跟踪器的跟踪所有权元数据

如果两层不一致，就会导致：
- 共享层查询说对象属于 A
- 跟踪查询却显示对象属于 B 或处于孤立状态

所以，现阶段公共 API 会尽力保证二者数据一致，后期我们可能重新设计这里的实现。

## 8.1 所有权相关 API

- `RLGetSharedObjectOwnerContext()`
- `RLIsSharedObjectOwnedByCurrentContext()`
- `RLTryTransferSharedObjectOwner()`
- `RLTryAdoptOrphanedSharedObject()`

这些 API 的同语义别名如下：
- `RLGetObjectOwnerContext()`
- `RLIsObjectOwnedByCurrentContext()`
- `RLTryTransferObjectOwner()`
- `RLTryAdoptOrphanedObject()`

## 8.2 转移语义

要成功转移对象所有权，至少要满足：
- 对象存在
- 目标上下文与当前上下文在同一个共享组
- 共享元数据允许转移
- 跟踪对象元数据也允许转移

当前公开 API 语义：
- 要么共享层和跟踪对象层一起成功
- 要么失败

## 8.3 孤立对象认领语义

要成功认领，至少要满足：
- 该对象的所有权确实在销毁时变为孤立状态
- 目标上下文在同一个共享组
- 共享元数据与跟踪元数据都接受认领

## 9. 跟踪模式

相关 API：
- `RLSetSharedGpuTrackingMode()`
- `RLGetSharedGpuTrackingMode()`

枚举值：

| 值                                   | 含义                   |
| ----------------------------------- | -------------------- |
| `RL_SHARED_GPU_TRACKING_COMPATIBLE` | 对未注册的保留和释放操作采用兼容回退行为 |
| `RL_SHARED_GPU_TRACKING_STRICT`     | 对未注册的保留和释放直接拒绝并报告    |

默认值：
- strict

推荐：
- 开发和诊断阶段使用 strict
- 只有明确需要兼容历史路径时才考虑 compatible
