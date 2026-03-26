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
        // Resource sharing defaults
        ctx->resourceShareMode = RL_CONTEXT_SHARE_NONE;
        ctx->resourceShareWith = nullptr;
        ctx->resourceShareValidated = 0;
        ctx->resourceShareValidationError = RL_CONTEXT_SHARE_VALIDATION_OK;
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

extern "C" bool RLContextSetResourceShareMode(RLContext *ctx, RLContextResourceShareMode mode, RLContext *shareWith)
{
    if (!ctx)
    {
        RLTraceLog(RL_E_LOG_ERROR, "SHARED_GPU: RLContextSetResourceShareMode failed: context is null");
        return false;
    }
    if ((mode != RL_CONTEXT_SHARE_NONE) &&
        (mode != RL_CONTEXT_SHARE_WITH_PRIMARY) &&
        (mode != RL_CONTEXT_SHARE_WITH_CONTEXT))
    {
        RLTraceLog(RL_E_LOG_ERROR, "SHARED_GPU: RLContextSetResourceShareMode failed: invalid mode=%d", (int)mode);
        return false;
    }
    if ((mode == RL_CONTEXT_SHARE_WITH_CONTEXT) && (shareWith == nullptr))
    {
        RLTraceLog(RL_E_LOG_ERROR, "SHARED_GPU: RLContextSetResourceShareMode failed: WITH_CONTEXT requires non-null target context");
        return false;
    }
    if ((mode == RL_CONTEXT_SHARE_WITH_CONTEXT) && (shareWith == ctx))
    {
        RLTraceLog(RL_E_LOG_ERROR, "SHARED_GPU: RLContextSetResourceShareMode failed: target context cannot be self");
        return false;
    }
    if (RLContextHasReadyWindow(ctx))
    {
        RLTraceLog(RL_E_LOG_ERROR, "SHARED_GPU: RLContextSetResourceShareMode failed: mode can only change before window creation");
        return false;
    }
    ctx->resourceShareMode = (int)mode;
    ctx->resourceShareWith = (mode == RL_CONTEXT_SHARE_WITH_CONTEXT) ? shareWith : nullptr;
    ctx->resourceShareValidated = 0;
    ctx->resourceShareValidationError = RL_CONTEXT_SHARE_VALIDATION_OK;
    return true;
}

extern "C" RLContextResourceShareMode RLContextGetResourceShareMode(RLContext *ctx)
{
    if (!ctx) return RL_CONTEXT_SHARE_NONE;
    return (RLContextResourceShareMode)ctx->resourceShareMode;
}

extern "C" RLContext *RLContextGetResourceShareContext(RLContext *ctx)
{
    if (!ctx) return nullptr;
    return (RLContext *)ctx->resourceShareWith;
}

extern "C" bool RLContextValidateResourceShareConfig(RLContext *ctx)
{
    if (ctx == nullptr) return false;

    RLContextResourceShareMode mode = (RLContextResourceShareMode)ctx->resourceShareMode;
    RLContext *targetContext = (RLContext *)ctx->resourceShareWith;

    ctx->resourceShareValidated = 1;
    ctx->resourceShareValidationError = RL_CONTEXT_SHARE_VALIDATION_OK;

    if ((mode != RL_CONTEXT_SHARE_NONE) &&
        (mode != RL_CONTEXT_SHARE_WITH_PRIMARY) &&
        (mode != RL_CONTEXT_SHARE_WITH_CONTEXT))
    {
        ctx->resourceShareValidationError = RL_CONTEXT_SHARE_VALIDATION_INVALID_MODE;
        RLTraceLog(RL_E_LOG_ERROR, "SHARED_GPU: share config validation failed: invalid mode=%d", (int)mode);
        return false;
    }

    if (mode == RL_CONTEXT_SHARE_WITH_CONTEXT)
    {
        if (targetContext == nullptr)
        {
            ctx->resourceShareValidationError = RL_CONTEXT_SHARE_VALIDATION_TARGET_NULL;
            RLTraceLog(RL_E_LOG_ERROR, "SHARED_GPU: share config validation failed: WITH_CONTEXT target context is null");
            return false;
        }

        if (targetContext == ctx)
        {
            ctx->resourceShareValidationError = RL_CONTEXT_SHARE_VALIDATION_TARGET_SELF;
            RLTraceLog(RL_E_LOG_ERROR, "SHARED_GPU: share config validation failed: WITH_CONTEXT target context is self");
            return false;
        }

        if (!RLContextHasReadyWindow(targetContext))
        {
            ctx->resourceShareValidationError = RL_CONTEXT_SHARE_VALIDATION_TARGET_WINDOW_UNAVAILABLE;
            RLTraceLog(RL_E_LOG_ERROR, "SHARED_GPU: share config validation failed: WITH_CONTEXT target window is unavailable");
            return false;
        }
    }

    return true;
}

extern "C" bool RLContextIsResourceShareConfigValid(RLContext *ctx)
{
    if (ctx == nullptr) return false;
    return (ctx->resourceShareValidated != 0) && (ctx->resourceShareValidationError == RL_CONTEXT_SHARE_VALIDATION_OK);
}

extern "C" int RLContextGetResourceShareValidationError(RLContext *ctx)
{
    if (ctx == nullptr) return RL_CONTEXT_SHARE_VALIDATION_CTX_NULL;
    return ctx->resourceShareValidationError;
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
