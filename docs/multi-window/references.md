# References

[English](./references.md) | [简体中文](./references.zh-CN.md)

This document set was reorganized with explicit reference to better-structured project documentation and architecture-document patterns.

## Documentation Structure References

### arc42
Why it matters:
- clear separation of scope, constraints, runtime view, and implementation view

References:
- https://arc42.org/overview
- https://docs.arc42.org/section-6/

### Software design document template
Why it matters:
- reinforces stable sectioning for architecture, runtime behavior, and API-level explanation

Reference:
- https://github.com/jam01/SDD-Template

### MADR / ADR style
Why it matters:
- design decisions such as queue policy or bind semantics should not live inside the main user guide

References:
- https://adr.github.io/madr/
- https://github.com/joelparkerhenderson/architecture-decision-record

## Library Documentation References

### GLFW documentation
Why it matters:
- separates guides, thread/guarantee notes, and reference
- useful model for documenting thread affinity and context rules

References:
- https://www.glfw.org/docs/3.3/
- https://www.glfw.org/docs/latest/context_guide.html
- https://www.glfw.org/docs/3.2/intro_guide.html

### SDL3 wiki
Why it matters:
- per-function thread-safety notes are explicit and easy to audit

References:
- https://wiki.libsdl.org/SDL3/SDL_CreateWindow
- https://wiki.libsdl.org/SDL3/SDL_PumpEvents

### Dear ImGui Multi-Viewports
Why it matters:
- demonstrates concise feature docs that explain enablement, required main-loop changes, and backend notes without dumping all internals into one page

Reference:
- https://github.com/ocornut/imgui/wiki/Multi-Viewports

## Project-Specific Source References

The authoritative implementation for this branch is still the source tree, especially:
- `src/raylib.h`
- `src/rl_context.h`
- `src/rl_shared_gpu.h`
- `src/rl_shared_gpu.cpp`
- `src/platforms/rcore_desktop_glfw.c`
- `src/external/glfw/src/win32_window.c`
- `src/rglfwglobal.h`
- `src/rglfwglobal.cpp`
