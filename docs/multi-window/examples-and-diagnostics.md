# Examples And Diagnostics

[English](./examples-and-diagnostics.md) | [简体中文](./examples-and-diagnostics.zh-CN.md)

## 1. Example Index

- `examples/core/core_shared_gpu_context.c`
- `examples/core/core_event_thread_diagnostics.c`
- `examples/core/core_memdiag_event_automation.c`
- `examples/core/core_long_path_utf8_fileio.c`
- `examples/audio/audio_module_playing.c`
- `examples/core/core_glfw_event_thread_diagnostics.cpp`
- `examples/core/core_glfw_refresh_callback_diagnostics.c`

## 2. Key Self-Test Entrypoints

### `core_shared_gpu_context --trace-reentry-selftest`
Validates trace callback reentry isolation under internal locks.

### `core_shared_gpu_context --shared-gpu-diag-scope-selftest`
Validates shared-GPU diagnostics scope separation:
- current-group snapshots
- global strict reject counters
- current-group reset vs reject reset behavior

### `core_event_thread_diagnostics --queue-saturation-selftest`
Validates bounded queue behavior under pressure.

### `core_event_thread_diagnostics --semaphore-selftest`
Validates semaphore wait/release/close behavior for waiter wake and clean failure.

### `core_event_thread_diagnostics --invoke-owned-selftest`
Validates owned-payload lifetime transfer and failure-path destructor handling for window-thread and render-thread invoke APIs.

### `core_event_thread_diagnostics --native-task-queue-selftest`
Validates lightweight native task queue getters and reset APIs without perturbing queue counters.

### `core_event_thread_diagnostics --frame-callback-reset-all-selftest`
Validates `RLResetAllFrameCallbackDiagStats()` by creating two event-thread windows and asserting both windows' cumulative counters are cleared together.

### `core_memdiag_event_automation`
Validates automated input playback under memory diagnostics and checks shutdown leak summary output.

### `core_long_path_utf8_fileio [rootOverride] [uncRootOverride]`
Validates long-path and UTF-8 file I/O behavior for:
- directory creation
- text and binary save/load
- directory enumeration
- rename and move
- file existence and directory existence
- working-directory changes
- path parsing helpers
- optional UNC root path access

Notes:
- The program prints a single JSON summary line.
- `renameOk`, `moveOk`, and `listCountOk` are strict pass fields in the JSON summary.
- `movedPathLen` reports the final moved destination path length after the rename + move chain.

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

### Shared-GPU diagnostics
Use:
- `RLGetCurrentSharedGpuGroupDiagStats()`
- `RLGetSharedGpuGroupDiagStatsForContext()`
- `RLGetSharedGpuTrackingRejectDiagStats()`
- `RLResetCurrentSharedGpuGroupDiagStats()`
- `RLResetSharedGpuGroupDiagStatsForContext()`
- `RLResetSharedGpuTrackingRejectDiagStats()`

Focus metrics:
- current share-group live/pending/owner snapshot
- group-scoped cumulative counters (`releaseUntrackedCount`, framebuffer-map counters)
- process-wide strict reject counters

## 4. Diagnostics Macros And Build Switches

### Compile-time switches

| Switch | Default | Defined at | Effect |
| --- | --- | --- | --- |
| `RL_MEM_DIAG` | `1` in this diagnostics build | public compile definition / [`src/raylib.h`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/raylib.h) | Enables memory diagnostics bookkeeping and `RLMemoryDiagStats` APIs. |
| `RL_MEM_DIAG_FILELINE` | `1` in this diagnostics build | public compile definition / [`src/raylib.h`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/raylib.h) | Expands memory diagnostics records with file/line capture. |
| `RL_EVENT_DIAG_STATS` | `1` | [`src/config.h`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:315) | Enables event-thread diagnostics code paths and counters. |
| `RL_EVENTTHREAD_COALESCE_STATE` | `1` | [`src/config.h`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:326) | Enables mailbox-style coalescing for high-frequency callbacks. |
| `RL_FRAME_CALLBACK_DIAG_STATS` | `1` | [`src/config.h`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:327) | Enables frame-callback cumulative diagnostics counters. |
| `RL_SHARED_GPU_DIAG_STATS` | `1` | [`src/config.h`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:333) | Enables shared-GPU cumulative diagnostics counters. |
| `RL_THREAD_MISMATCH_DIAG_STATS` | `1` | [`src/config.h`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:321) | Enables wrong-thread GPU-write diagnostics counters. |
| `RL_TRACKED_OBJECT_DIAG` | `1` | [`src/config.h`](G:/C、C++ Programing/jackalclient/raylib/raylib/src/config.h:339) | Enables tracked-object dump/audit/log diagnostics helpers. |

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
2. Run shared-GPU diagnostics scope self-test.
3. Run queue saturation self-test.
4. Run semaphore self-test.
5. Run invoke-owned self-test.
6. Run native-task-queue self-test.
7. Run frame-callback reset-all self-test.
8. Re-run shared-resource examples and verify no ownership split warnings.
9. Re-run event-thread examples and verify no close-waiter enqueue succeeds.
10. Re-run memory-diagnostics automation example and verify `NO_LEAK_DETECTED`.
11. Run the long-path / UTF-8 file I/O example and inspect the JSON summary for failed stages.
12. Start the audio multi-window example and verify secondary-thread window creation and shutdown still work.
