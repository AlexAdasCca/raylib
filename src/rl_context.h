#ifndef RL_CONTEXT_H
#define RL_CONTEXT_H

// Internal context support for multi-window / multi-thread.
//
// Route2 Stage-A:
// - A per-thread "current context" is selected using RLSetCurrentContext().
// - Internal modules read/write state through RLGetCurrentContext().
// - Some legacy file-static state (rlgl, default font, shapes texture, etc.)
//   is moved into RLContext fields.
//
// The public API exposes RLContext as an opaque handle (declared in raylib.h).

#include "raylib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Full definition (internal). Public header keeps it opaque.
struct RLContext
{
    // Opaque pointers to internal module storage (allocated lazily by modules that know the types)
    void *core;        // CoreData*
    void *platformData;    // PlatformData* (GLFW desktop)
    void *rlgl;        // rlglData*

    // rlgl module legacy statics
    double lfRlCullDistanceNear;
    double lfRlCullDistanceFar;
    bool bIsGpuReady;

    // rtext module legacy statics
    RLFont stDefaultFont;
    bool bDefaultFontReady;
    int nTextLineSpacing;

    // rshapes module legacy statics
    RLTexture2D stShapesTexture;
    RLRectangle stShapesTextureRec;
    bool bIsShapesTextureReady;

    // One-shot Win32 class name override for the next GLFW window created
    // (desktop+GLFW on Windows only). Empty string means default.
    char win32ClassName[256];


    // GPU resource sharing configuration for the next window created by this context.
    // See RLContextSetResourceShareMode() in raylib.h.
    int resourceShareMode;
    struct RLContext *resourceShareWith;
};

// Internal helper called by rl_context.cpp before freeing RLContext.
// Implemented in rcore.c (has access to CoreData/PlatformData/rlglData).
void RLContextOnDestroy(RLContext *ctx);

#ifdef __cplusplus
}
#endif

#endif  // RL_CONTEXT_H
