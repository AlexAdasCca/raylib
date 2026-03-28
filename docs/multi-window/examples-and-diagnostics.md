# Examples And Diagnostics

[English](./examples-and-diagnostics.md) | [简体中文](./examples-and-diagnostics.zh-CN.md)

## 1. Example Index

- `examples/core/core_shared_gpu_context.c`
- `examples/core/core_event_thread_diagnostics.c`
- `examples/core/core_glfw_event_thread_diagnostics.cpp`
- `examples/core/core_glfw_refresh_callback_diagnostics.c`

## 2. Key Self-Test Entrypoints

### `core_shared_gpu_context --trace-reentry-selftest`
Validates trace callback reentry isolation under internal locks.

### `core_event_thread_diagnostics --queue-saturation-selftest`
Validates bounded queue behavior under pressure.

### `core_event_thread_diagnostics --semaphore-selftest`
Validates semaphore wait/release/close behavior for waiter wake and clean failure.

## 3. How to Use Diagnostics APIs

### Event-thread diagnostics
Use:
- `RLGetEventThreadDiagStats()`
- `RLResetEventThreadDiagStats()`
- `RLResetEventThreadDiagStatsForCurrentContext()`
- `RLResetEventThreadDiagStatsByHandle()`

Focus metrics:
- task post failure counts
- queue drop counts
- queue pressure and saturation behavior

### Tracked-object diagnostics
Use:
- `RLSetTrackedObjectDiagFlags()`
- `RLGetTrackedObjectDiagFlags()`
- `RLDebugDumpTrackedObjectState()`

Focus metrics:
- scope values and policy
- active/tombstone distribution
- release misses and promotion audit output

### Thread-mismatch diagnostics
Use:
- `RLGetThreadMismatchDiagStats()`
- `RLResetThreadMismatchDiagStats()`

Focus metrics:
- wrong-thread API call counts
- call-site attribution

## 4. Diagnostics Macros And Build Switches

### Compile-time switches

| Switch | Default | Defined at | Effect |
| --- | --- | --- | --- |
| `RL_EVENT_DIAG_STATS` | `1` | [`src/config.h`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:315) | Enables event-thread diagnostics code paths and counters. |
| `RL_EVENTTHREAD_COALESCE_STATE` | `1` | [`src/config.h`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:326) | Enables mailbox-style coalescing for high-frequency callbacks. |

### Tracked-object diagnostics flags

Defined in:
[`src/raylib.h`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/raylib.h:2163)

Flags:
1. `RL_TRACKED_OBJECT_DIAG_LOG_RELEASE_CALLS`
2. `RL_TRACKED_OBJECT_DIAG_DUMP_STATE_ON_RELEASE_MISS`
3. `RL_TRACKED_OBJECT_DIAG_INCLUDE_TOMBSTONES`
4. `RL_TRACKED_OBJECT_DIAG_LOG_PROMOTIONS`
5. `RL_TRACKED_OBJECT_DIAG_AUDIT_PROMOTIONS`

Typical setup code:

```c
RLSetTrackedObjectDiagFlags(
    RL_TRACKED_OBJECT_DIAG_LOG_RELEASE_CALLS |
    RL_TRACKED_OBJECT_DIAG_DUMP_STATE_ON_RELEASE_MISS |
    RL_TRACKED_OBJECT_DIAG_INCLUDE_TOMBSTONES |
    RL_TRACKED_OBJECT_DIAG_LOG_PROMOTIONS |
    RL_TRACKED_OBJECT_DIAG_AUDIT_PROMOTIONS);
```

## 5. Regression Checklist

1. Run trace callback reentry self-test.
2. Run queue saturation self-test.
3. Run semaphore self-test.
4. Re-run shared-resource examples and verify no ownership split warnings.
5. Re-run event-thread examples and verify no close-waiter enqueue succeeds.
