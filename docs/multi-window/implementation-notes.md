# Implementation Notes

[English](./implementation-notes.md) | [简体中文](./implementation-notes.zh-CN.md)

> [!WARNING]
> This page describes current implementation details, not all public contract guarantees.

## 1. Layering

1. Public API contract layer:
`src/raylib.h`
2. Context model and lifecycle layer:
`src/rl_context.cpp`, `src/rcore.c`
3. Shared GPU lifetime and ownership layer:
`src/rl_shared_gpu.cpp`
4. Tracked-object layer:
`src/rcore.c`, `src/rl_object_tracker.h`
5. Platform integration layer:
`src/platforms/rcore_desktop_glfw.c`
6. Win32 native queue layer:
`src/external/glfw/src/win32_window.c`
7. Shared synchronization primitives:
`src/rglfwglobal.cpp`, `src/rglfwglobal.h`

## 2. Diagnostic Build Switches And Macros

### 2.1 Preprocessor switches

| Macro | Default | Defined at | Purpose |
| --- | --- | --- | --- |
| `RL_EVENT_DIAG_STATS` | `1` | [`src/config.h`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:315) | Enables event-thread diagnostics counters and related runtime APIs. |
| `RL_EVENTTHREAD_COALESCE_STATE` | `1` | [`src/config.h`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:326) | Enables mailbox-style coalescing for high-frequency state callbacks. |
| `RLGLFW_DIAGNOSTICS` | Build-dependent | [`src/platforms/rcore_desktop_glfw.c`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:116) | Enables GLFW-side assertions in diagnostics builds. |

> [!NOTE]
> `RL_EVENT_DIAG_STATS` is a compile-time switch.  
> `RLEnableEventDiagStats()` and `RLDisableEventDiagStats()` are runtime switches that are effective only when diagnostics code is compiled in.

### 2.2 Frame-callback queue tuning macros

All are defined in [`src/platforms/rcore_desktop_glfw.c`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:128).

| Macro | Default | Meaning |
| --- | --- | --- |
| `RL_FRAME_CALLBACK_NORMAL_QUEUE_CAPACITY` | `1024` | Normal callback queue capacity. |
| `RL_FRAME_CALLBACK_CRITICAL_QUEUE_CAPACITY` | `256` | Critical callback queue capacity. |
| `RL_FRAME_CALLBACKS_PER_FRAME_LIMIT` | `64` | Max callbacks executed per frame drain pass. |
| `RL_FRAME_CALLBACKS_BATCH_POP_LIMIT` | `8` | Max callbacks popped per lock-acquire batch. |
| `RL_FRAME_CALLBACKS_CRITICAL_MIN_PER_FRAME` | `8` | Minimum critical callbacks targeted per frame. |
| `RL_FRAME_CALLBACK_WAIT_CRITICAL_MS` | `50` | Queue slot wait timeout for critical enqueue. |
| `RL_FRAME_CALLBACK_WAIT_NORMAL_MS` | `10` | Queue slot wait timeout for normal enqueue. |

### 2.3 Internal diagnostics macros and context

Macro wrappers are defined in [`src/rcore.c`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rcore.c:2175), and expanded to no-op when diagnostics are compiled out.

```c
#define RL_DIAG_TASK_POSTED()      RLDiag_OnTaskPosted()
#define RL_DIAG_TASK_POST_FAILED() RLDiag_OnTaskPostFailed()
#define RL_DIAG_TASK_EXECUTED()    RLDiag_OnTaskExecuted()
#define RL_DIAG_PUMP_BEGIN()       RLDiag_PumpBegin()
#define RL_DIAG_PUMP_END()         RLDiag_PumpEnd()
```

Usage in platform code:
[`src/platforms/rcore_desktop_glfw.c`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:899)

```c
RL_DIAG_PUMP_BEGIN();                            // Begin one native pump section
...                                              // Pump tasks/messages
unsigned int executedTaskCount = RL_DIAG_PUMP_END();
RL_DIAG_ON_PUMP(RLGetTime() - pumpStartTime, executedTaskCount); // Record cost and volume
```

Payload classification enum is defined in [`src/rcore.c`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rcore.c:1921), and includes:
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

## 3. Runtime Self-Test Entry Points

| Self-test flag | Example | Location | Purpose |
| --- | --- | --- | --- |
| `--trace-reentry-selftest` | `core_shared_gpu_context` | `examples/core/core_shared_gpu_context.c` | Verifies trace callback isolation and no recursive callback entry under internal locks. |
| `--shared-gpu-diag-scope-selftest` | `core_shared_gpu_context` | `examples/core/core_shared_gpu_context.c` | Verifies current-group diagnostics vs global strict reject counters and the corresponding reset APIs. |
| `--queue-saturation-selftest` | `core_event_thread_diagnostics` | `examples/core/core_event_thread_diagnostics.c` | Verifies bounded-queue behavior under pressure and close-state transitions. |
| `--semaphore-selftest` | `core_event_thread_diagnostics` | `examples/core/core_event_thread_diagnostics.c` | Verifies waiter wake, release semantics, and close-time failure behavior. |
| `--invoke-owned-selftest` | `core_event_thread_diagnostics` | `examples/core/core_event_thread_diagnostics.c` | Verifies owned-payload destructor handling on invoke success and failure paths. |
| `--native-task-queue-selftest` | `core_event_thread_diagnostics` | `examples/core/core_event_thread_diagnostics.c` | Verifies native task queue lightweight getters and reset APIs without self-perturbation. |
| `--frame-callback-reset-all-selftest` | `core_event_thread_diagnostics` | `examples/core/core_event_thread_diagnostics.c` | Verifies that all tracked windows' frame-callback cumulative counters are reset together. |

## 4. Queue Design: Code-Level Details

### 4.1 Win32 native task queue

Core implementation:
[`src/external/glfw/src/win32_window.c`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/external/glfw/src/win32_window.c:538)

Key points:
1. Bounded queue model with explicit failure accounting.
2. Task class metadata supports pressure-aware handling.
3. Wake-up uses dispatch helper messages to break waits and modal loops.

### 4.2 Frame callback queue

Queue state is stored in `PlatformData`:
[`src/platforms/rcore_desktop_glfw.c`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:225)

Separate queues:
1. `normalFrameCallbackRing`
2. `criticalFrameCallbackRing`

Per-kind slot credits:
1. `normalFrameCallbackSlotsAvailableSemaphore`
2. `criticalFrameCallbackSlotsAvailableSemaphore`

Drain implementation:
[`RLGlfwDrainFrameCallbacksCurrentContext`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/platforms/rcore_desktop_glfw.c:2949)

Code context with inline comments:

```c
unsigned int callbacksCriticalTarget = ...;  // Per-frame critical minimum target
while (callbacksExecuted < RL_FRAME_CALLBACKS_PER_FRAME_LIMIT)
{
    RLRenderFrameCallbackSlot callbackBatch[RL_FRAME_CALLBACKS_BATCH_POP_LIMIT] = { 0 };
    ...
    RLGlfwGlobalLock();
    while (callbackBatchCount < batchBudget)
    {
        // Pop critical first if critical quota is not yet met
        // Otherwise pop normal; fallback to critical if normal is empty
    }
    RLGlfwGlobalUnlock();

    // Execute popped callbacks outside global lock
}
```

## 5. Shared-Resource Core Points

### 5.1 `gpuShareGroup` lifetime handoff and tracked-scope policy are decoupled
No raw pointer handoff without pinning:
`PinnedGroup` in [`src/rl_shared_gpu.cpp`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rl_shared_gpu.cpp:104).

### 5.2 Promotion is part of bind contract
Bind fails when tracked promotion fails:
[`src/rl_shared_gpu.cpp`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rl_shared_gpu.cpp:623).

### 5.3 Transfer/adopt are transactional
Transaction path in public API:
[`src/rcore.c`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rcore.c:3752).

## 6. Trace Callback Isolation

Lock scopes that block user trace callback reentry:
1. tracked-object lock scope
2. shared-group lock scope
3. binding lock scope
4. GLFW global lock scope

Entry points:
- [`RLTraceCallbackIsolationEnter`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rl_context.h:142)
- [`RLTraceCallbackIsolationLeave`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/rl_context.h:143)

## 7. Current Review Hotspots

1. Queue saturation behavior under mixed producer rates.
2. Close-time waiter wake correctness.
3. Shared ownership transfer/adopt under concurrent context teardown.
4. Promotion conflict diagnostics and bind-failure propagation.
5. Event-thread diagnostics overhead under high-frequency input.
