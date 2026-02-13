#include "rl_context.h"

#include <cstdlib>   // calloc/free

// Per-thread current context (Route2 Stage-A)
static thread_local RLContext *gRlCurrentContext = nullptr;

static RLContext *RLAllocContext(void)
{
    RLContext *ctx = (RLContext *)std::calloc(1, sizeof(RLContext));
    if (ctx)
    {
        // Reasonable defaults for module legacy statics (the rest is initialized by modules).
        ctx->lfRlCullDistanceNear = 0.0;
        ctx->lfRlCullDistanceFar = 0.0;
        ctx->bIsGpuReady = false;

        ctx->bDefaultFontReady = false;
        ctx->nTextLineSpacing = 2;

        ctx->bIsShapesTextureReady = false;
        ctx->stShapesTexture = { 0 };
        ctx->stShapesTextureRec = { 0.0f, 0.0f, 0.0f, 0.0f };
    }

    return ctx;
}

extern "C" RLContext *RLGetCurrentContext(void)
{
    if (!gRlCurrentContext) gRlCurrentContext = RLAllocContext();
    return gRlCurrentContext;
}

extern "C" void RLSetCurrentContext(RLContext *ctx)
{
    gRlCurrentContext = ctx;
}

extern "C" RLContext *RLCreateContext(void)
{
    return RLAllocContext();
}

extern "C" void RLDestroyContext(RLContext *ctx)
{
    if (!ctx) return;

    // If destroying current, clear first to avoid modules accidentally using freed memory.
    if (ctx == gRlCurrentContext) gRlCurrentContext = nullptr;

    // Let core module release internal allocations/resources for this ctx if they exist.
    RLContextOnDestroy(ctx);

    std::free(ctx);
}
