# 参考资料

[English](./references.md) | [简体中文](./references.zh-CN.md)

## 1. 为什么要参考外部文档

这次重写不是为了“把旧文档翻译一遍”，而是为了把文档组织方式提升到更稳定的工程文档水准。  
因此参考了两类来源：

1. 文档结构模板
2. 真正处理窗口、线程和多视口问题的项目文档

## 2. 文档结构参考

### arc42
用途：
- 学习如何把范围、约束、运行时场景、实现视图拆开

链接：
- https://arc42.org/overview
- https://docs.arc42.org/section-6/

### MADR / ADR
用途：
- 把“为什么这样设计”与“怎么使用 API”分开

链接：
- https://adr.github.io/madr/
- https://github.com/joelparkerhenderson/architecture-decision-record

### Software design document template
用途：
- 参考 API 说明、运行时场景和实现说明如何分节

链接：
- https://github.com/jam01/SDD-Template

## 3. 库与官方文档参考

### GLFW
用途：
- 参考其 guide / reference / limitations 的分层方式
- 参考 context / thread affinity 的写法

链接：
- https://www.glfw.org/docs/3.3/
- https://www.glfw.org/docs/latest/context_guide.html
- https://www.glfw.org/docs/3.2/intro_guide.html

### SDL3 Wiki
用途：
- 参考 per-function thread-safety 说明方式

链接：
- https://wiki.libsdl.org/SDL3/SDL_CreateWindow
- https://wiki.libsdl.org/SDL3/SDL_PumpEvents

### Dear ImGui Multi-Viewports
用途：
- 参考多窗口/多视口功能文档如何用较少篇幅先给用户明确操作步骤和 backend 注意事项

链接：
- https://github.com/ocornut/imgui/wiki/Multi-Viewports

## 4. 本项目的权威来源

无论文档怎么整理，当前分支的权威实现仍然是源码本身，尤其是：
- `src/raylib.h`
- `src/rl_context.h`
- `src/rl_shared_gpu.h`
- `src/rl_shared_gpu.cpp`
- `src/platforms/rcore_desktop_glfw.c`
- `src/external/glfw/src/win32_window.c`
- `src/rglfwglobal.h`
- `src/rglfwglobal.cpp`
