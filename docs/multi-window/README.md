# Multi-Window Support Documentation (Win32/GLFW)

[English](./README.md) | [简体中文](./README.zh-CN.md)

> [!IMPORTANT]
> This document set describes only what is implemented and verified in this branch for `PLATFORM_DESKTOP_GLFW` on `_WIN32`.
>
> It is not general raylib documentation, and it does not imply the same behavior on other platforms or backends.

## Goals

This document set answers four questions:

1. What this branch supports and does not support.
2. Which thread and lifecycle stage each API requires.
3. The contracts for resource sharing, cross-thread dispatch, queues, and diagnostics.
4. Where implementation code lives, and which parts are stable contract versus current implementation detail.

## Recommended Reading Order

1. [overview.md](./overview.md)
2. [threading-and-lifecycle.md](./threading-and-lifecycle.md)
3. [resource-sharing.md](./resource-sharing.md)
4. [api-reference.md](./api-reference.md)
5. [runtime-scenarios.md](./runtime-scenarios.md)
6. [examples-and-diagnostics.md](./examples-and-diagnostics.md)
7. [implementation-notes.md](./implementation-notes.md)
8. [references.md](./references.md)

## Document Responsibilities

| Document | Purpose | Audience |
| --- | --- | --- |
| `overview.md` | Scope, limits, and quick usage decisions | All readers |
| `threading-and-lifecycle.md` | Threading contract, lifecycle order, legal and illegal call patterns | Users, reviewers |
| `resource-sharing.md` | Share modes, share-group semantics, ownership, tracked-scope promotion | Users, maintainers |
| `api-reference.md` | Function, enum, callback, and structure reference | Users, maintainers |
| `runtime-scenarios.md` | Step-by-step runtime flows and expected behavior | Users, testers |
| `examples-and-diagnostics.md` | Example index, diagnostics usage, regression self-tests | Testers, maintainers |
| `implementation-notes.md` | Internal layering, lock model, queue model, source map | Maintainers, reviewers |
| `references.md` | External standards and official references | Maintainers |

## Quick Conclusions

### Most Important Stable Rules

1. `RLContextSetResourceShareMode()` must be called before `RLInitWindow()` or `RLInitWindowEx()`. For explicit sharing, the target context must already have a live initialized window.
2. `RL_CONTEXT_SHARE_WITH_PRIMARY` and `RL_CONTEXT_SHARE_WITH_CONTEXT` do not auto-fallback to `RL_CONTEXT_SHARE_NONE` on failure.
3. `RLDestroyContext()` does not implicitly call `RLCloseWindow()`. Close first, then destroy.
4. `RLInvokeOnWindowRenderThreadByHandle()` is render-thread-affine, but not frame-boundary-safe.
5. `RLPostWindowFrameCallbackByHandle*()` executes at a stable point inside target-window `RLEndDrawing()`.
6. In `RL_E_FLAG_WINDOW_EVENT_THREAD` mode, one render thread supports one active window.
7. Public shared-object transfer and adopt APIs are transactional across shared metadata and tracked metadata.

### Quick Entry Examples

| Need | Example |
| --- | --- |
| Shared context, shared shader, ownership transfer, trace callback isolation | `examples/core/core_shared_gpu_context.c` |
| Event thread, queue saturation, queue close waiters, UI performance diagnostics | `examples/core/core_event_thread_diagnostics.c` |
| Win32 modal-loop refresh callback | `examples/core/core_glfw_refresh_callback_diagnostics.c` |

## Documentation Style

This document set uses:
- GitHub Markdown tables
- GitHub alert blocks (`IMPORTANT`, `WARNING`, `NOTE`)
- Language switch links at the top of every page

## Next

Start with:
- [overview.md](./overview.md)
- [api-reference.md](./api-reference.md)

Then go deeper with:
- [threading-and-lifecycle.md](./threading-and-lifecycle.md)
- [resource-sharing.md](./resource-sharing.md)
- [implementation-notes.md](./implementation-notes.md)
