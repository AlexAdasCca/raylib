# Resource Sharing And Share-Group Semantics

[English](./resource-sharing.md) | [简体中文](./resource-sharing.zh-CN.md)

## 1. What This Section Solves

Resource sharing in this branch separates two concepts that must not be conflated:
1. share-group lifetime and shared metadata
2. tracked-object scope and tracked metadata

## 2. Two Separate Concepts

### 2.1 Share-group lifetime
Share-group owns:
- object refcounts
- owner/orphan metadata
- pending deletes
- program locations and program-use scopes

### 2.2 Tracked-object scope
Tracked namespace can be:
- `context`
- `share_group`

A context can use internal share-group lifetime management while tracked scope remains context-local.

## 3. Share Modes

### 3.1 `RL_CONTEXT_SHARE_NONE`
No explicit resource sharing with another context.

### 3.2 `RL_CONTEXT_SHARE_WITH_PRIMARY`
Shares with the primary context.

Requirements:
- primary window already initialized
- configured before target `RLInitWindow()`

### 3.3 `RL_CONTEXT_SHARE_WITH_CONTEXT`
Shares with a specific peer context.

Requirements:
- `shareWith` is not `NULL`
- `shareWith` is not self
- target context already owns an initialized window

## 4. Share-Configuration Validation

Public validation APIs:
- `RLContextValidateResourceShareConfig()`
- `RLContextIsResourceShareConfigValid()`
- `RLContextGetResourceShareValidationError()`

### Validation error enum
- `RL_CONTEXT_SHARE_VALIDATION_OK`
- `RL_CONTEXT_SHARE_VALIDATION_CTX_NULL`
- `RL_CONTEXT_SHARE_VALIDATION_INVALID_MODE`
- `RL_CONTEXT_SHARE_VALIDATION_TARGET_NULL`
- `RL_CONTEXT_SHARE_VALIDATION_TARGET_SELF`
- `RL_CONTEXT_SHARE_VALIDATION_TARGET_WINDOW_UNAVAILABLE`
- `RL_CONTEXT_SHARE_VALIDATION_PRIMARY_WINDOW_UNAVAILABLE`

### Recommended usage
1. configure mode
2. validate configuration
3. only then call `RLInitWindow()`

## 5. Tracked-Scope Promotion

### 5.1 Why promotion exists
When a context that has context-scoped tracked entries joins a real shared namespace, tracked entries must migrate to share-group scope.

### 5.2 Promotion is part of bind contract
Bind succeeds only if promotion succeeds.

### 5.3 Sticky upgrade
`context -> share_group` is allowed.
`share_group -> context` is not allowed during that relationship lifetime.

## 6. Shared-Group Lifetime APIs

- `RLSharedRetainShader()` / `RLSharedReleaseShader()`
- `RLSharedRetainBuffer()` / `RLSharedReleaseBuffer()`
- `RLSharedRetainVertexArray()` / `RLSharedReleaseVertexArray()`
- `RLSharedRetainFramebuffer()` / `RLSharedReleaseFramebuffer()`
- `RLSharedRetainFramebufferBase()` / `RLSharedReleaseFramebufferBase()`
- `RLSharedRetainRenderbuffer()` / `RLSharedReleaseRenderbuffer()`
- `RLDeletePendingSharedGpuResources()`
- `RLDeletePendingSharedGpuResourcesByHandle()`

### Typical scenarios
1. shared shader/program lifetime
2. by-handle cleanup on target render thread

## 7. Shared Shader Coordination

### Public APIs
- `RLBeginSharedShaderUse(shader, policy)`
- `RLSharedShaderUseEnd(shader, policy)`
- `RLConfigureSharedShaderFenceWait(waitSliceUs, waitTimeoutUs)`

### Policy enum
| Policy | Meaning |
| --- | --- |
| `RL_SHARED_SHADER_USE_PHASED` | Fair serialized order by ticket |
| `RL_SHARED_SHADER_USE_LOCKED` | Simple mutex serialization |

### Required call order
1. `RLBeginSharedShaderUse()`
2. `RLBeginShaderMode()`
3. draw calls
4. `RLEndShaderMode()`
5. `RLSharedShaderUseEnd()`

## 8. Ownership Metadata

### 8.1 Public ownership APIs
- `RLGetSharedObjectOwnerContext()`
- `RLIsSharedObjectOwnedByCurrentContext()`
- `RLTryTransferSharedObjectOwner()`
- `RLTryAdoptOrphanedSharedObject()`

Generic aliases:
- `RLGetObjectOwnerContext()`
- `RLIsObjectOwnedByCurrentContext()`
- `RLTryTransferObjectOwner()`
- `RLTryAdoptOrphanedObject()`

### 8.2 Transfer semantics
Transfer succeeds only when both metadata layers accept the operation in one transaction.

### 8.3 Orphan adopt semantics
Adopt requires orphaned owner state and same-share-group membership for target context.

## 9. Tracking Mode

`RLSetSharedGpuTrackingMode()` and `RLGetSharedGpuTrackingMode()`:

- `RL_SHARED_GPU_TRACKING_COMPATIBLE`
- `RL_SHARED_GPU_TRACKING_STRICT`

Default is strict.
