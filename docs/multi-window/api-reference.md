# API Reference

[English](./api-reference.md) | [简体中文](./api-reference.zh-CN.md)

This section documents the feature-scoped APIs added or materially changed by the multi-window, event-thread, Win32, and shared-resource work in this branch.

It is not a replacement for `src/raylib.h`. It is the contract-level companion for this feature set.

## Reading Notes

1. Signature and parameter names are kept identical to `src/raylib.h`.
2. `wait` parameters in this page are synchronization flags, not time units:
- `wait == 0`: asynchronous post and immediate return
- `wait != 0`: synchronous wait until completion
3. Queue wait timeout values are internal policy constants and are not provided through these `wait` parameters.

## Related Types

### `RLContext`
Opaque handle representing one raylib context.

Use it to:
- select the current context on a thread
- configure resource sharing before window creation
- refer to a target context in ownership and share operations

### `RLContextResourceShareMode`
Share mode for the next window created on a context.

Values:
- `RL_CONTEXT_SHARE_NONE`
- `RL_CONTEXT_SHARE_WITH_PRIMARY`
- `RL_CONTEXT_SHARE_WITH_CONTEXT`

### `RLContextResourceShareValidationError`
Validation result for explicit share configuration.

Use case:
- Read exact validation failure reason before window initialization.

### `RLSharedShaderUsePolicy`
Concurrent shared-shader use policy.

Values:
- `RL_SHARED_SHADER_USE_PHASED`
- `RL_SHARED_SHADER_USE_LOCKED`

Notes:
- `RL_SHARED_SHADER_USE_PHASED`: ticket-ordered fair serialization.
- `RL_SHARED_SHADER_USE_LOCKED`: plain mutex serialization.

### `RLSharedObjectType`
Shared object kind used by ownership APIs.

Values:
- `RL_SHARED_OBJECT_TEXTURE`
- `RL_SHARED_OBJECT_BUFFER`
- `RL_SHARED_OBJECT_VERTEX_ARRAY`
- `RL_SHARED_OBJECT_FRAMEBUFFER`
- `RL_SHARED_OBJECT_RENDERBUFFER`
- `RL_SHARED_OBJECT_PROGRAM`

### `RLWin32MessageHook`
```c
typedef int (*RLWin32MessageHook)(void* hwnd,
                                  unsigned int uMsg,
                                  uintptr_t wParam,
                                  intptr_t lParam,
                                  intptr_t* result,
                                  void* user);
```

Parameters:
- `hwnd`: target native window handle
- `uMsg`: Win32 message id
- `wParam`, `lParam`: message payload
- `result`: output location for the return value if the hook handles the message
- `user`: caller-supplied payload

Return value:
- non-zero: message handled, `*result` becomes the window-proc return value
- zero: continue normal processing

### `RLWin32WindowThreadInvoke`
```c
typedef intptr_t (*RLWin32WindowThreadInvoke)(void* hwnd, void* user);
```
Runs on the window's Win32 thread.

### `RLWindowRenderThreadInvoke`
```c
typedef intptr_t (*RLWindowRenderThreadInvoke)(void* hwnd, void* user);
```
Runs on the target window's render thread.

### `RLFrameCallbackKind`
Queue class for frame-safe render callbacks.

Values:
- `RL_FRAME_CALLBACK_KIND_NORMAL`
- `RL_FRAME_CALLBACK_KIND_CRITICAL`

Use case:
- `NORMAL`: routine frame-boundary work.
- `CRITICAL`: lifecycle or cleanup work that should use the dedicated critical queue.

### `RLWindowRefreshCallback`
```c
typedef void (*RLWindowRefreshCallback)(void);
```
Draw-only callback used during Win32 modal refresh ticks.

### `RLEventThreadDiagStats`
Large diagnostics structure for event-thread mode.

Notable field groups:
- payload allocation counters
- native Win32 task queue depth and dropped counts
- frame callback queue depth, peak, dropped, executed, cleared, and inline-fallback counts
- pump timing
- swap and wait costs
- thread-mismatch counters

### `RLThreadMismatchDiagStats`
Compact diagnostics structure summarizing render-thread mismatch handling for GPU-write APIs.

### `RLFrameCallbackQueueStats`
Lightweight queue snapshot for the current window/context frame-callback queues.

Inline fallback means:
- the caller is already on the target render thread
- the selected frame-callback queue has no free slot at that moment
- instead of waiting for queue space, the callback executes immediately on that render thread

Accounting rules:
- inline-fallback callbacks count as `executed*`
- they also count as `inlineFallback*`
- they do not become queued entries, so they do not increase `queued*`
- they are not counted as dropped or cleared
- once execution begins, ownership of `user` follows the normal callback-executed rule

Key fields:
- `queuedCount`: total queued callbacks
- `queuedNormalCount`: queued normal callbacks
- `queuedCriticalCount`: queued critical callbacks
- `queuedPeakCount`: peak queued count since last reset
- `queuedPeakNormalCount`: peak queued normal count since last reset
- `queuedPeakCriticalCount`: peak queued critical count since last reset
- `droppedCount`: total dropped callbacks
- `droppedNormalCount`: dropped normal callbacks
- `droppedCriticalCount`: dropped critical callbacks
- `executedCount`: total executed callbacks
- `executedNormalCount`: executed normal callbacks
- `executedCriticalCount`: executed critical callbacks
- `clearedCount`: queued callbacks cleared before execution
- `clearedNormalCount`: cleared normal callbacks
- `clearedCriticalCount`: cleared critical callbacks
- `inlineFallbackCount`: queue-saturated callbacks executed immediately on the render thread
- `inlineFallbackNormalCount`: normal callbacks executed via inline fallback
- `inlineFallbackCriticalCount`: critical callbacks executed via inline fallback

Use case:
- Poll only frame-callback queue state without reading the full `RLEventThreadDiagStats` snapshot.

## Feature Flags

### `RL_E_FLAG_WINDOW_EVENT_THREAD`
Create a dedicated Win32 message/event thread for this window.

### `RL_E_FLAG_WINDOW_REFRESH_CALLBACK`
Enable OS-driven refresh ticks during Win32 modal loops.

### `RL_E_FLAG_WINDOW_BROADCAST_WAKE`
Broadcast wake to all windows' render threads on shutdown/close.

## Context APIs

### `RLContext *RLCreateContext(void)`
Create a context object.

Returns:
- opaque `RLContext*`

Notes:
- Context selection is per-thread.
- A per-thread default context may also be created implicitly on first use by other code paths, but explicit creation is recommended for multi-window work.

### `void RLDestroyContext(RLContext *ctx)`
Destroy the context object.

Parameters:
- `ctx`: context to destroy

Notes:
- Does not implicitly close a live window.
- If shared owner metadata belongs to `ctx`, destroy may orphan ownership state that another context may later adopt.

### `void RLSetCurrentContext(RLContext *ctx)`
Select the current context for the calling thread.

Parameters:
- `ctx`: context to become current on this thread

### `RLContext *RLGetCurrentContext(void)`
Return the current context for the calling thread.

## Share Configuration APIs

### `bool RLContextSetResourceShareMode(RLContext* ctx, RLContextResourceShareMode mode, RLContext* shareWith)`
Configure how the next window created on `ctx` should share GPU resources.

Parameters:
- `ctx`: context being configured
- `mode`: one of `RLContextResourceShareMode`
- `shareWith`: target context when `mode == RL_CONTEXT_SHARE_WITH_CONTEXT`, otherwise `NULL`

Returns:
- `true` on accepted configuration
- `false` on invalid request or illegal timing

Notes:
- Must be called before `RLInitWindow()` or `RLInitWindowEx()`.
- Explicit share modes require the target window to exist at creation time.

### `RLContextResourceShareMode RLContextGetResourceShareMode(RLContext* ctx)`
Return the configured share mode.

Parameters:
- `ctx`: context to query

Use case:
- Validate context setup path before calling `RLInitWindow()`.

### `RLContext* RLContextGetResourceShareContext(RLContext* ctx)`
Return the explicit target share context, if any.

Parameters:
- `ctx`: context to query

### `bool RLContextValidateResourceShareConfig(RLContext* ctx)`
Validate the currently configured share mode and store the latest validation result inside the context.

Parameters:
- `ctx`: context to validate

Returns:
- `true` when configuration is valid
- `false` otherwise

### `bool RLContextIsResourceShareConfigValid(RLContext* ctx)`
Return the latest cached validation result.

Parameters:
- `ctx`: context to query

### `int RLContextGetResourceShareValidationError(RLContext* ctx)`
Return the latest validation error code.

Type:
- `RLContextResourceShareValidationError`

Parameters:
- `ctx`: context to query

## Shared GPU Lifetime APIs

### `bool RLSharedRetainShader(RLShader shader)` / `bool RLSharedReleaseShader(RLShader shader)`
Retain or release a shader program at share-group scope.

Parameters:
- `shader`: `RLShader` whose `id` identifies the GL program

Returns:
- `true` on accepted operation
- `false` on invalid/untracked/rejected operation in strict mode

Use case:
- Keep shader lifetime valid when sharing program usage across contexts.

### `bool RLSharedRetainBuffer(unsigned int bufferId)` / `bool RLSharedReleaseBuffer(unsigned int bufferId)`
Retain or release a GL buffer id at share-group scope.

Parameters:
- `bufferId`: GL buffer id

### `bool RLSharedRetainVertexArray(unsigned int vertexArrayId)` / `bool RLSharedReleaseVertexArray(unsigned int vertexArrayId)`
Retain or release a GL vertex array id.

Parameters:
- `vertexArrayId`: GL vertex array id

### `bool RLSharedRetainFramebuffer(unsigned int framebufferId)` / `bool RLSharedReleaseFramebuffer(unsigned int framebufferId)`
Retain or release a framebuffer and currently tracked attachments associated with it.

Parameters:
- `framebufferId`: GL framebuffer id

### `bool RLSharedRetainFramebufferBase(unsigned int framebufferId)` / `bool RLSharedReleaseFramebufferBase(unsigned int framebufferId)`
Retain or release only the framebuffer object itself.

Parameters:
- `framebufferId`: GL framebuffer id

### `bool RLSharedRetainRenderbuffer(unsigned int renderbufferId)` / `bool RLSharedReleaseRenderbuffer(unsigned int renderbufferId)`
Retain or release a renderbuffer object.

Parameters:
- `renderbufferId`: GL renderbuffer id

### `bool RLDeletePendingSharedGpuResources(void)`
Drain pending deferred deletes for the current share-group on the current render thread.

Notes:
- A current GL context belonging to the group is required.
- `RLCloseWindow()` also performs the required cleanup drain for the current window.

Use case:
- Explicitly force deferred shared-GPU deletion at controlled points.

### `RLSharedGpuDiagStats`
Structured diagnostics snapshot for the current context's share-group.

Key fields:
- `hasShareGroup`: whether the current context is currently bound to a share-group
- `usesSharedTrackedScope`: whether tracked-object scope has been promoted from per-context to share-group scope
- `contextRefCount`: number of contexts still attached to the group
- `liveObjectCount` / `pendingDeleteCount`: live shared objects and deferred-delete queue depth
- `ownerEntryCount` / `orphanedOwnerCount`: current owner metadata entries and orphaned owner entries
- `framebufferAttachmentMapCount` / `framebufferDepthMapCount`: current framebuffer attachment/depth mapping table sizes
- `programLocEntryCount` / `programUseScopeCount` / `pendingProgramFenceCount`: shader-program coordination state
- `textureTraceCount`: number of texture trace metadata records
- `live*` / `pending*`: per-object-type live and pending counts
- `framebufferMapHitCount` / `framebufferMapMissCount` / `framebufferReleaseSkippedCount`: framebuffer mapping effectiveness counters
- `releaseUntrackedCount`: number of release calls observed on untracked objects
- `unregisteredRetainRejectCount` / `unregisteredReleaseRejectCount`: strict-mode reject counters for unregistered retain/release

Use case:
- Capture a machine-readable share-group snapshot before or after create/unload/flush checkpoints.

### `RLSharedGpuDiagStats RLGetSharedGpuDiagStats(void)`
Return the current share-group diagnostics snapshot.

Returns:
- zero-filled snapshot when no share-group is currently bound on this thread
- populated snapshot for the current context's share-group otherwise

Notes:
- This is the structured counterpart to `RLDebugDumpSharedGpuState()`.
- The returned data is suitable for logs, assertions, or automated tests.

### `void RLDebugDumpSharedGpuState(const char *label)`
Dump the current share-group diagnostics snapshot to the trace log.

Parameters:
- `label`: optional label included in the log output

Notes:
- Output format is intended for human diagnosis.
- For stable programmatic checks, prefer `RLGetSharedGpuDiagStats()`.

## Shared Shader Coordination APIs

### `bool RLBeginSharedShaderUse(RLShader shader, int policy)`
Enter a serialized shared-shader use scope.

Parameters:
- `shader`: shader program to protect
- `policy`: `RLSharedShaderUsePolicy`

Returns:
- `true` if the scope was entered
- `false` if setup or lookup failed

### `void RLSharedShaderUseEnd(RLShader shader, int policy)`
Leave the serialized shared-shader use scope.

Parameters:
- `shader`: shader program
- `policy`: same policy passed to begin

### `bool RLConfigureSharedShaderFenceWait(unsigned int waitSliceUs, unsigned int waitTimeoutUs)`
Configure polling slice and timeout used by the phased shared-shader fence wait logic.

Parameters:
- `waitSliceUs`: microseconds per poll slice
- `waitTimeoutUs`: total timeout in microseconds

## Lightweight Frame-Callback Queue API

### `bool RLGetCurrentWindowFrameCallbackQueueStats(RLFrameCallbackQueueStats *outStats)`
Get a lightweight frame-callback queue snapshot for the current window/context.

Parameters:
- `outStats`: destination structure

Returns:
- `true` when queue stats are available for the current window/context
- `false` when the current backend/window mode does not expose these stats

Notes:
- This is the lightweight companion to the `frameCallbackQueue*` fields in `RLEventThreadDiagStats`.
- Prefer this API when you only need current frame-callback queue state.
- Available on the Win32 + desktop GLFW backend.

### `bool RLGetWindowFrameCallbackQueueStatsByHandle(void* hwnd, RLFrameCallbackQueueStats *outStats)`
Get a lightweight frame-callback queue snapshot for a specific raylib window.

Parameters:
- `hwnd`: target window handle
- `outStats`: destination structure

Returns:
- `true` when queue stats are available for the target window
- `false` when the handle is unknown or the current backend/window mode does not expose these stats

Notes:
- Use this when the querying thread is not currently bound to the target window/context.
- Available on the Win32 + desktop GLFW backend.

## Shared Ownership APIs

### `RLContext* RLGetSharedObjectOwnerContext(RLSharedObjectType type, unsigned int objectId)`
Return the current owner context for the shared object.

Parameters:
- `type`: shared object kind
- `objectId`: GL object id or program id

### `bool RLIsSharedObjectOwnedByCurrentContext(RLSharedObjectType type, unsigned int objectId)`
Return whether the calling thread's current context owns the object.

Parameters:
- `type`: shared object kind
- `objectId`: GL object id or program id

### `bool RLTryTransferSharedObjectOwner(RLSharedObjectType type, unsigned int objectId, RLContext* targetCtx)`
Attempt to move ownership of a shared object to `targetCtx`.

Parameters:
- `type`: `RLSharedObjectType`
- `objectId`: GL object id or program id
- `targetCtx`: target context in the same share-group

Returns:
- `true` if the transfer committed on both shared metadata and tracked metadata
- `false` otherwise

Notes:
- This is an atomic owner transaction at the API level.
- It will not report success after updating only one metadata layer.

### `bool RLTryAdoptOrphanedSharedObject(RLSharedObjectType type, unsigned int objectId, RLContext* targetCtx)`
Attempt to adopt an orphaned shared object.

Notes:
- Only succeeds when the object owner was orphaned during context destroy.
- Target context must already belong to the same share-group.

### Generic aliases
These have the same semantics as the shared-owner names:
- `RLGetObjectOwnerContext()`
- `RLIsObjectOwnedByCurrentContext()`
- `RLTryTransferObjectOwner()`
- `RLTryAdoptOrphanedObject()`

## Window Creation And Refresh APIs

### `void RLInitWindowEx(int width, int height, const char *title, const char *win32ClassName)`
Variant of `RLInitWindow()` with an explicit one-shot Win32 class name.

Parameters:
- `width`, `height`: initial logical window size
- `title`: UTF-8 title
- `win32ClassName`: optional Win32 class name

### `void *RLGetWindowHandle(void)`
Return the native window handle for the current window.

Type:
- on Win32 this is an `HWND` stored as `void*`

Use case:
- Use returned handle with by-handle APIs.

### `void RLSetWindowWin32ClassName(const char *win32ClassName)`
Set the one-shot Win32 class name that will be used by the next window creation on desktop GLFW Windows.

Parameters:
- `win32ClassName`: optional class name for next create call

### `void RLSetWindowRefreshCallback(RLWindowRefreshCallback callback)`
Install the modal-loop refresh callback.

Parameters:
- `callback`: draw-only callback function

Requirements:
- `RL_E_FLAG_WINDOW_REFRESH_CALLBACK` must be enabled for the window mode to use it meaningfully.

## Win32 Property And Hook APIs

### `int RLWin32SetWindowProp(const char* name, void* value)`
Set a named HWND property on the current window.

Parameters:
- `name`: property name
- `value`: opaque pointer value

### `void* RLWin32GetWindowProp(const char* name)`
Get a named HWND property from the current window.

Parameters:
- `name`: property name

### `void* RLWin32RemoveWindowProp(const char* name)`
Remove and return a named HWND property from the current window.

Parameters:
- `name`: property name

### `void* RLWin32AddMessageHook(RLWin32MessageHook hook, void* user)`
Register a Win32 message hook on the current window.

Returns:
- opaque token used by `RLWin32RemoveMessageHook()`

### `int RLWin32RemoveMessageHook(void* token)`
Remove a message hook previously registered on the current window.

Parameters:
- `token`: hook token returned by add API

### `int RLWin32GetAllWindowHandles(void** outHwnds, int maxCount)`
Enumerate tracked raylib HWNDs.

Parameters:
- `outHwnds`: output array, or `NULL` to query count only
- `maxCount`: capacity of `outHwnds`

Return value:
- number of tracked windows, or number written when output storage is provided

### `void* RLWin32GetPrimaryWindowHandle(void)`
Return the current primary window handle.

Use case:
- Configure secondary contexts with `RL_CONTEXT_SHARE_WITH_PRIMARY`.

### `int RLWin32IsKnownWindowHandle(void* hwnd)`
Return whether `hwnd` belongs to a tracked raylib window.

### By-handle variants
These operate on a specific raylib window identified by HWND:
- `RLWin32SetWindowPropByHandle()`
- `RLWin32GetWindowPropByHandle()`
- `RLWin32RemoveWindowPropByHandle()`
- `RLWin32AddMessageHookByHandle()`
- `RLWin32RemoveMessageHookByHandle()`

## Cross-Thread Invoke APIs

### `intptr_t RLWin32InvokeOnWindowThreadByHandle(void* hwnd, RLWin32WindowThreadInvoke fn, void* user, int wait)`
Run `fn` on the target window's Win32 thread.

Parameters:
- `hwnd`: target window handle
- `fn`: callback to execute on the window thread
- `user`: opaque payload
- `wait`: synchronization flag; `0` posts asynchronously, non-zero waits until completion

Use this for:
- Win32 UI operations
- HWND-affine state changes

### `intptr_t RLWin32InvokeOnWindowThreadByHandleEx(void* hwnd, RLWin32WindowThreadInvoke fn, void* user, int wait, void (*userDtor)(void*))`
Run `fn` on the target window's Win32 thread with explicit payload ownership transfer.

Parameters:
- `hwnd`: target window handle
- `fn`: callback to execute on the window thread
- `user`: opaque payload
- `wait`: synchronization flag; `0` posts asynchronously, non-zero waits until completion
- `userDtor`: optional destructor for `user`

Ownership rules:
- if dispatch is rejected before execution, `userDtor(user)` is called before return
- if an asynchronous request is accepted but later canceled before callback execution, `userDtor(user)` is called
- if the callback executes, ownership transfers to the callback implementation

Notes:
- This is the ownership-managed companion to `RLWin32InvokeOnWindowThreadByHandle()`.
- Automated coverage is available in `examples/core/core_event_thread_diagnostics.c` via `--invoke-owned-selftest`.
- The synchronous path preserves the callback return value, including zero.
- The callback implementation becomes responsible for `user` as soon as execution begins.

### `intptr_t RLInvokeOnWindowRenderThreadByHandle(void* hwnd, RLWindowRenderThreadInvoke fn, void* user, int wait)`
Run `fn` on the target window's render thread.

Parameters:
- `hwnd`: target window handle
- `fn`: render-thread callback
- `user`: opaque payload
- `wait`: synchronization flag; `0` posts asynchronously, non-zero waits until completion

Important note:
- This is not frame-safe.
- In non-event-thread mode it only works when called from the same thread that owns the target GL context.

### `intptr_t RLInvokeOnWindowRenderThreadByHandleEx(void* hwnd, RLWindowRenderThreadInvoke fn, void* user, int wait, void (*userDtor)(void*))`
Run `fn` on the target window's render thread with explicit payload ownership transfer.

Parameters:
- `hwnd`: target window handle
- `fn`: render-thread callback
- `user`: opaque payload
- `wait`: synchronization flag; `0` posts asynchronously, non-zero waits until completion
- `userDtor`: optional destructor for `user`

Ownership rules:
- if dispatch is rejected before execution, `userDtor(user)` is called before return
- if a queued invoke is later rejected during close/stop before callback execution, `userDtor(user)` is called
- if the callback executes, ownership transfers to the callback implementation

Notes:
- This is the ownership-managed companion to `RLInvokeOnWindowRenderThreadByHandle()`.
- Automated coverage is available in `examples/core/core_event_thread_diagnostics.c` via `--invoke-owned-selftest`.
- It closes the common leak-prone path where asynchronous invoke payloads otherwise need ad-hoc cleanup on enqueue failure.
- The callback implementation becomes responsible for `user` as soon as execution begins.

## Frame-Safe Render Callback APIs

### `int RLPostWindowFrameCallbackByHandle(void* hwnd, RLWindowRenderThreadInvoke fn, void* user)`
Queue a normal frame-safe callback.

Parameters:
- `hwnd`: target window handle
- `fn`: callback executed on target render thread
- `user`: opaque payload

Use case:
- Minimal frame-boundary-safe callback posting with default `NORMAL` queue kind.

### `int RLPostWindowFrameCallbackByHandleEx(void* hwnd, RLWindowRenderThreadInvoke fn, void* user, RLFrameCallbackKind kind)`
Queue a callback with explicit queue kind.

Parameters:
- `hwnd`: target window handle
- `fn`: callback executed on target render thread
- `user`: opaque payload
- `kind`: `NORMAL` or `CRITICAL`

Use case:
- Select critical queue for lifecycle-sensitive work.

### `int RLPostWindowFrameCallbackByHandleEx2(void* hwnd, RLWindowRenderThreadInvoke fn, void* user, RLFrameCallbackKind kind, void (*userDtor)(void*))`
Queue a callback with explicit payload ownership semantics.

Parameters:
- `hwnd`: target window handle
- `fn`: callback executed on target render thread
- `user`: opaque payload
- `kind`: `RL_FRAME_CALLBACK_KIND_NORMAL` or `RL_FRAME_CALLBACK_KIND_CRITICAL`
- `userDtor`: optional destructor for `user` if the queue takes ownership but the callback never executes

Return value:
- `1` on successful enqueue
- `0` on invalid arguments, close/stop state, unsupported mode, allocation failure, or queue wait timeout

Notes:
- Callback executes at a fixed point inside `RLEndDrawing()` before final batch flush/swap.
- If the caller is already on the target render thread and the selected queue is saturated, the callback may execute immediately as inline fallback instead of waiting for queue space.
- Inline fallback still counts as successful execution, not as drop or clear.
- If the callback never executes, the framework may call `userDtor(user)` during enqueue failure, close, stop, or queue cleanup.
- If the callback executes, ownership of `user` transfers to the callback implementation.
- The queue is bounded; enqueue may fail under pressure.

### `int RLDeletePendingSharedGpuResourcesByHandle(void* hwnd, int wait)`
Request pending shared-GPU delete drain on the target render thread.

Parameters:
- `hwnd`: target window handle
- `wait`: synchronization flag; `0` posts asynchronously, non-zero waits until completion

## Diagnostics APIs

### `RLEventThreadDiagStats RLGetEventThreadDiagStats(void)`
Return event-thread diagnostics for the current context/window.

Use case:
- Read counters after stress run and compare with expected invariants.

### `void RLResetEventThreadDiagStats(void)`
Reset event-thread diagnostics for the current context/window.

Use case:
- Reset baseline before scenario replay.

### `void RLResetEventThreadDiagStatsForCurrentContext(void)`
Reset event-thread diagnostics and safely reset native queue stats for the current context/window.

Use case:
- Clear diagnostics between two test phases in the same window.

### `void RLEnableEventDiagStats(void)` / `void RLDisableEventDiagStats(void)` / `bool RLIsEventDiagStatsEnabled(void)`
Runtime switch for event diagnostics counting.

Notes:
- Effective only when the library is built with `RL_EVENT_DIAG_STATS=1`.

### `RLThreadMismatchDiagStats RLGetThreadMismatchDiagStats(void)`
Return diagnostics for render-thread mismatch handling of GPU-write APIs.

Use case:
- Detect wrong-thread GPU-write call sites during integration tests.

### `void RLResetThreadMismatchDiagStats(void)`
Reset mismatch diagnostics.

Use case:
- Reset mismatch counters at test phase boundaries.

### `void RLSetTrackedObjectDiagFlags(unsigned int flags)` / `unsigned int RLGetTrackedObjectDiagFlags(void)`
Configure or query tracked-object diagnostics flags.

Flags:
- `RL_TRACKED_OBJECT_DIAG_LOG_RELEASE_CALLS`
- `RL_TRACKED_OBJECT_DIAG_DUMP_STATE_ON_RELEASE_MISS`
- `RL_TRACKED_OBJECT_DIAG_INCLUDE_TOMBSTONES`
- `RL_TRACKED_OBJECT_DIAG_LOG_PROMOTIONS`
- `RL_TRACKED_OBJECT_DIAG_AUDIT_PROMOTIONS`

Use case:
- Enable only required diagnostics classes to control log volume.

### `void RLDebugDumpTrackedObjectState(const char *label)`
Dump tracked-object table state to the trace log.

### `int RLResetEventThreadDiagStatsByHandle(void* hwnd, int wait)`
By-handle reset variant for the target window.

Parameters:
- `hwnd`: target window handle
- `wait`: synchronization flag; `0` posts asynchronously, non-zero waits until completion
