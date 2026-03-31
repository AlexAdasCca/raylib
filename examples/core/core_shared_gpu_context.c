/*******************************************************************************************
*
*   raylib [core] example - shared GPU context resources (multi-window + multi-thread)
*
*   This example validates share-group wide GPU resource lifetime management:
*   - Two OpenGL contexts (windows) share the same GL object namespace (GLFW share).
*   - Resources (texture/shader/rendertexture) are referenced from multiple contexts.
*   - One context unloads/releases while the other continues to use the objects.
*   - A window/context is closed and later a new window/context is created again on the
*     same thread, allocating and freeing resources, checking for leaks.
*
*   Notes:
*   - This example is intended for Desktop OpenGL backend (GLFW).
*   - Cross-context synchronization is the application's responsibility. We avoid
*     concurrent read/write to the same GPU object across threads.
*
********************************************************************************************/

#include "raylib.h"
#include "rlgl.h"
#include "../../src/rglfwglobal.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
    #include <windows.h>
    #include <process.h> // _beginthreadex
#endif

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION 330
#else
    #define GLSL_VERSION 100
#endif

typedef struct SharedPack {
    RLTexture2D tex;
    RLShader shader;
    RLRenderTexture2D rt;
} SharedPack;

typedef enum SharedShaderTestMode {
    SHARED_SHADER_MODE_LOCAL_PER_WINDOW = 1,   // Share texture/FBO only, each window has its own shader
    SHARED_SHADER_MODE_PHASED_SHARED = 2,      // Share shader, but only one window uses it at a time
    SHARED_SHADER_MODE_LOCKED_SHARED = 3       // Share shader, protect concurrent usage with lock
} SharedShaderTestMode;

static SharedPack gPack = { 0 };
static RLContext *gMainCtx = NULL;
static RLContext *gWorkerCtx = NULL;
static SharedShaderTestMode gShaderMode = SHARED_SHADER_MODE_LOCAL_PER_WINDOW;
static RLShader LoadTintShaderFromMemory(void);
static void FlushPendingSharedDeletesForCurrentWindow(const char *tag);
static bool HasCommandLineFlag(int argc, char **argv, const char *flagText);
static int RunTraceReentrySelfTest(void);
static int RunSharedGpuDiagScopeSelfTest(void);
#if defined(_WIN32)
static volatile LONG gOwnerTransferMainToWorkerDone = 0;
#endif

typedef struct TraceReentryTestState
{
    bool reentryAttempted;
    bool callbackReentered;
    bool glfwLockedTraceSuppressed;
    int callbackDepth;
    int maxCallbackDepth;
    int callbackCount;
    int nestedTrackedDumpCount;
    int nestedSharedDumpCount;
    int nestedDirectTraceCount;
} TraceReentryTestState;

static TraceReentryTestState gTraceReentryTestState = { 0 };

static unsigned long long GetMemDiagTotalAllocCalls(const RLMemoryDiagStats *stats)
{
    if (stats == NULL) return 0ull;
    return stats->allocCount + stats->callocCount + stats->reallocCount;
}

static void PrintMemDiagStatsLine(const char *stageName, RLMemoryDiagStats stats)
{
    printf("MEMDIAG_AUTOTEST: %s outstanding_bytes=%llu alloc=%llu free=%llu peak=%llu alloc_fail=%llu realloc_fail=%llu\n",
        (stageName != NULL) ? stageName : "(unknown)",
        stats.currentOutstandingBytes,
        GetMemDiagTotalAllocCalls(&stats),
        stats.freeCount,
        stats.peakOutstandingBytes,
        stats.allocFailCount,
        stats.reallocFailCount);
}

static void PrintSharedGpuDiagStatsLine(const char *stageName)
{
    RLSharedGpuGroupDiagStats sharedGpuGroupDiagStats = RLGetCurrentSharedGpuGroupDiagStats();
    RLSharedGpuTrackingRejectDiagStats sharedGpuRejectDiagStats = RLGetSharedGpuTrackingRejectDiagStats();
    const char *trackedScopeText = sharedGpuGroupDiagStats.usesSharedTrackedScope ? "share_group" : "context";

    printf("SHARED_GPU_DIAG: %s has_group=%d tracked_scope=%s ctx_refs=%u live=%llu pending=%llu owners=%llu orphaned=%llu untracked_release=%llu reject_retain=%llu reject_release=%llu\n",
        (stageName != NULL) ? stageName : "(unknown)",
        sharedGpuGroupDiagStats.hasShareGroup,
        trackedScopeText,
        sharedGpuGroupDiagStats.contextRefCount,
        sharedGpuGroupDiagStats.liveObjectCount,
        sharedGpuGroupDiagStats.pendingDeleteCount,
        sharedGpuGroupDiagStats.ownerEntryCount,
        sharedGpuGroupDiagStats.orphanedOwnerCount,
        sharedGpuGroupDiagStats.releaseUntrackedCount,
        sharedGpuRejectDiagStats.unregisteredRetainRejectCount,
        sharedGpuRejectDiagStats.unregisteredReleaseRejectCount);

    printf("SHARED_GPU_DIAG: %s live_tex=%llu live_buf=%llu live_vao=%llu live_fbo=%llu live_rbo=%llu live_prog=%llu pending_tex=%llu pending_buf=%llu pending_vao=%llu pending_fbo=%llu pending_rbo=%llu pending_prog=%llu map_attach=%llu map_depth=%llu map_hit=%llu map_miss=%llu map_release_skip=%llu prog_locs=%llu prog_scopes=%llu pending_fences=%llu texture_trace=%llu\n",
        (stageName != NULL) ? stageName : "(unknown)",
        sharedGpuGroupDiagStats.liveTextureCount,
        sharedGpuGroupDiagStats.liveBufferCount,
        sharedGpuGroupDiagStats.liveVertexArrayCount,
        sharedGpuGroupDiagStats.liveFramebufferCount,
        sharedGpuGroupDiagStats.liveRenderbufferCount,
        sharedGpuGroupDiagStats.liveProgramCount,
        sharedGpuGroupDiagStats.pendingTextureCount,
        sharedGpuGroupDiagStats.pendingBufferCount,
        sharedGpuGroupDiagStats.pendingVertexArrayCount,
        sharedGpuGroupDiagStats.pendingFramebufferCount,
        sharedGpuGroupDiagStats.pendingRenderbufferCount,
        sharedGpuGroupDiagStats.pendingProgramCount,
        sharedGpuGroupDiagStats.framebufferAttachmentMapCount,
        sharedGpuGroupDiagStats.framebufferDepthMapCount,
        sharedGpuGroupDiagStats.framebufferMapHitCount,
        sharedGpuGroupDiagStats.framebufferMapMissCount,
        sharedGpuGroupDiagStats.framebufferReleaseSkippedCount,
        sharedGpuGroupDiagStats.programLocEntryCount,
        sharedGpuGroupDiagStats.programUseScopeCount,
        sharedGpuGroupDiagStats.pendingProgramFenceCount,
        sharedGpuGroupDiagStats.textureTraceCount);
}

static const char *GetShaderModeLabel(SharedShaderTestMode mode)
{
    switch (mode)
    {
        case SHARED_SHADER_MODE_LOCAL_PER_WINDOW: return "Mode1 local-shader-per-window";
        case SHARED_SHADER_MODE_PHASED_SHARED: return "Mode2 phased-shared-shader";
        case SHARED_SHADER_MODE_LOCKED_SHARED: return "Mode3 locked-shared-shader";
        default: return "Mode? unknown";
    }
}

static SharedShaderTestMode ParseShaderModeFromArgv(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        if ((strcmp(argv[i], "--mode=1") == 0) || (strcmp(argv[i], "-m1") == 0)) return SHARED_SHADER_MODE_LOCAL_PER_WINDOW;
        if ((strcmp(argv[i], "--mode=2") == 0) || (strcmp(argv[i], "-m2") == 0)) return SHARED_SHADER_MODE_PHASED_SHARED;
        if ((strcmp(argv[i], "--mode=3") == 0) || (strcmp(argv[i], "-m3") == 0)) return SHARED_SHADER_MODE_LOCKED_SHARED;
    }
    return SHARED_SHADER_MODE_LOCAL_PER_WINDOW;
}

static bool HasCommandLineFlag(int argc, char **argv, const char *flagText)
{
    if ((argv == NULL) || (flagText == NULL)) return false;

    for (int argIndex = 1; argIndex < argc; argIndex++)
    {
        if ((argv[argIndex] != NULL) && (strcmp(argv[argIndex], flagText) == 0)) return true;
    }

    return false;
}

static void TraceReentryTestCallback(int logLevel, const char *text, va_list args)
{
    (void)logLevel;
    (void)text;
    (void)args;

    gTraceReentryTestState.callbackCount++;
    gTraceReentryTestState.callbackDepth++;
    if (gTraceReentryTestState.callbackDepth > gTraceReentryTestState.maxCallbackDepth)
    {
        gTraceReentryTestState.maxCallbackDepth = gTraceReentryTestState.callbackDepth;
    }

    if (gTraceReentryTestState.callbackDepth > 1)
    {
        gTraceReentryTestState.callbackReentered = true;
    }
    else if (!gTraceReentryTestState.reentryAttempted)
    {
        gTraceReentryTestState.reentryAttempted = true;

        gTraceReentryTestState.nestedDirectTraceCount++;
        RLTraceLog(RL_E_LOG_INFO, "trace-reentry-test: nested direct trace from callback");

        gTraceReentryTestState.nestedTrackedDumpCount++;
        RLDebugDumpTrackedObjectState("trace-reentry-test: nested tracked dump");

        gTraceReentryTestState.nestedSharedDumpCount++;
        RLDebugDumpSharedGpuState("trace-reentry-test: nested shared dump");
    }

    gTraceReentryTestState.callbackDepth--;
}

static int RunTraceReentrySelfTest(void)
{
    RLContext *context = RLCreateContext();
    if (context == NULL)
    {
        RLTraceLog(RL_E_LOG_WARNING, "trace-reentry-test: failed to create context");
        return 2;
    }

    RLSetCurrentContext(context);
    RLSetConfigFlags(RL_E_FLAG_WINDOW_RESIZABLE | RL_E_FLAG_WINDOW_EVENT_THREAD);
    RLInitWindow(360, 220, "raylib [trace-reentry] selftest");
    if (!RLIsWindowReady())
    {
        RLTraceLog(RL_E_LOG_WARNING, "trace-reentry-test: window init failed");
        RLDestroyContext(context);
        return 2;
    }

    RLImage image = RLGenImageChecked(96, 96, 12, 12,
        (RLColor){ 220, 120, 90, 255 },
        (RLColor){ 80, 140, 220, 255 });
    RLTexture2D texture = RLLoadTextureFromImage(image);
    RLUnloadImage(image);
    RLShader shader = LoadTintShaderFromMemory();
    RLRenderTexture2D renderTexture = RLLoadRenderTexture(96, 96);

    memset(&gTraceReentryTestState, 0, sizeof(gTraceReentryTestState));
    RLSetTraceLogCallback(TraceReentryTestCallback);
    RLTraceLog(RL_E_LOG_INFO, "trace-reentry-test: outer trigger");

    int callbackCountBeforeGlfwLockedTrace = gTraceReentryTestState.callbackCount;
    RLGlfwGlobalLock();
    RLTraceLog(RL_E_LOG_INFO, "trace-reentry-test: glfw-locked trigger");
    RLGlfwGlobalUnlock();
    gTraceReentryTestState.glfwLockedTraceSuppressed =
        (gTraceReentryTestState.callbackCount == callbackCountBeforeGlfwLockedTrace);

    RLSetTraceLogCallback(NULL);

    bool callbackSafe = gTraceReentryTestState.reentryAttempted &&
                        !gTraceReentryTestState.callbackReentered &&
                        (gTraceReentryTestState.maxCallbackDepth == 1) &&
                        (gTraceReentryTestState.callbackCount == 1) &&
                        gTraceReentryTestState.glfwLockedTraceSuppressed;

    RLTraceLog(RL_E_LOG_INFO,
               "trace-reentry-test: summary callbackCount=%d maxDepth=%d callbackReentered=%d trackedDumps=%d sharedDumps=%d nestedDirectTraces=%d",
               gTraceReentryTestState.callbackCount,
               gTraceReentryTestState.maxCallbackDepth,
               gTraceReentryTestState.callbackReentered ? 1 : 0,
               gTraceReentryTestState.glfwLockedTraceSuppressed ? 1 : 0,
               gTraceReentryTestState.nestedTrackedDumpCount,
               gTraceReentryTestState.nestedSharedDumpCount,
               gTraceReentryTestState.nestedDirectTraceCount);

    RLUnloadTexture(texture);
    RLUnloadShader(shader);
    RLUnloadRenderTexture(renderTexture);
    FlushPendingSharedDeletesForCurrentWindow("trace-reentry-test");
    RLCloseWindow();
    RLDestroyContext(context);

    if (!callbackSafe)
    {
        RLTraceLog(RL_E_LOG_WARNING, "trace-reentry-test: FAILED");
        return 3;
    }

    RLTraceLog(RL_E_LOG_INFO, "trace-reentry-test: PASSED");
    return 0;
}

static bool SharedGpuGroupDiagStatsEqual(RLSharedGpuGroupDiagStats left, RLSharedGpuGroupDiagStats right)
{
    return (left.hasShareGroup == right.hasShareGroup) &&
           (left.usesSharedTrackedScope == right.usesSharedTrackedScope) &&
           (left.contextRefCount == right.contextRefCount) &&
           (left.liveObjectCount == right.liveObjectCount) &&
           (left.pendingDeleteCount == right.pendingDeleteCount) &&
           (left.ownerEntryCount == right.ownerEntryCount) &&
           (left.orphanedOwnerCount == right.orphanedOwnerCount) &&
           (left.framebufferAttachmentMapCount == right.framebufferAttachmentMapCount) &&
           (left.framebufferDepthMapCount == right.framebufferDepthMapCount) &&
           (left.programLocEntryCount == right.programLocEntryCount) &&
           (left.programUseScopeCount == right.programUseScopeCount) &&
           (left.pendingProgramFenceCount == right.pendingProgramFenceCount) &&
           (left.textureTraceCount == right.textureTraceCount) &&
           (left.liveTextureCount == right.liveTextureCount) &&
           (left.liveBufferCount == right.liveBufferCount) &&
           (left.liveVertexArrayCount == right.liveVertexArrayCount) &&
           (left.liveFramebufferCount == right.liveFramebufferCount) &&
           (left.liveRenderbufferCount == right.liveRenderbufferCount) &&
           (left.liveProgramCount == right.liveProgramCount) &&
           (left.pendingTextureCount == right.pendingTextureCount) &&
           (left.pendingBufferCount == right.pendingBufferCount) &&
           (left.pendingVertexArrayCount == right.pendingVertexArrayCount) &&
           (left.pendingFramebufferCount == right.pendingFramebufferCount) &&
           (left.pendingRenderbufferCount == right.pendingRenderbufferCount) &&
           (left.pendingProgramCount == right.pendingProgramCount) &&
           (left.releaseUntrackedCount == right.releaseUntrackedCount) &&
           (left.framebufferMapHitCount == right.framebufferMapHitCount) &&
           (left.framebufferMapMissCount == right.framebufferMapMissCount) &&
           (left.framebufferReleaseSkippedCount == right.framebufferReleaseSkippedCount);
}

static int RunSharedGpuDiagScopeSelfTest(void)
{
    RLContext *context = RLCreateContext();
    RLSharedGpuTrackingMode originalTrackingMode;
    bool cumulativeEnabledBefore = false;
    RLSharedGpuGroupDiagStats currentGroupStats = { 0 };
    RLSharedGpuGroupDiagStats contextGroupStats = { 0 };
    RLSharedGpuTrackingRejectDiagStats rejectStats = { 0 };
    RLRenderTexture2D renderTexture = { 0 };
    int result = 2;

    if (context == NULL)
    {
        RLTraceLog(RL_E_LOG_WARNING, "shared-gpu-diag-scope-selftest: failed to create context");
        return 2;
    }

    RLSetCurrentContext(context);
    RLSetConfigFlags(RL_E_FLAG_WINDOW_RESIZABLE | RL_E_FLAG_WINDOW_EVENT_THREAD);
    RLInitWindow(360, 220, "raylib [shared-gpu-diag] selftest");
    if (!RLIsWindowReady())
    {
        RLTraceLog(RL_E_LOG_WARNING, "shared-gpu-diag-scope-selftest: window init failed");
        RLDestroyContext(context);
        return 2;
    }

    renderTexture = RLLoadRenderTexture(64, 64);
    originalTrackingMode = (RLSharedGpuTrackingMode)RLGetSharedGpuTrackingMode();
    cumulativeEnabledBefore = RLIsSharedGpuCumulativeDiagStatsEnabled();

    RLEnableSharedGpuCumulativeDiagStats();
    RLResetCurrentSharedGpuGroupDiagStats();
    RLResetSharedGpuTrackingRejectDiagStats();

    if (!RLIsSharedGpuCumulativeDiagStatsEnabled())
    {
        RLTraceLog(RL_E_LOG_WARNING, "shared-gpu-diag-scope-selftest: cumulative diagnostics switch did not enable");
        goto cleanup;
    }

    currentGroupStats = RLGetCurrentSharedGpuGroupDiagStats();
    contextGroupStats = RLGetSharedGpuGroupDiagStatsForContext(context);
    if (!currentGroupStats.hasShareGroup || !contextGroupStats.hasShareGroup)
    {
        RLTraceLog(RL_E_LOG_WARNING, "shared-gpu-diag-scope-selftest: expected current context to have a share-group");
        goto cleanup;
    }

    if (!SharedGpuGroupDiagStatsEqual(currentGroupStats, contextGroupStats))
    {
        RLTraceLog(RL_E_LOG_WARNING, "shared-gpu-diag-scope-selftest: current/for-context group snapshots diverged");
        goto cleanup;
    }

    RLSetSharedGpuTrackingMode(RL_SHARED_GPU_TRACKING_STRICT);
    (void)RLSharedReleaseBuffer(123456u);

    rejectStats = RLGetSharedGpuTrackingRejectDiagStats();
    if ((rejectStats.unregisteredRetainRejectCount != 0ull) ||
        (rejectStats.unregisteredReleaseRejectCount == 0ull))
    {
        RLTraceLog(RL_E_LOG_WARNING,
                   "shared-gpu-diag-scope-selftest: strict reject counters failed invariants (retain=%llu release=%llu)",
                   rejectStats.unregisteredRetainRejectCount,
                   rejectStats.unregisteredReleaseRejectCount);
        goto cleanup;
    }

    if (!RLResetSharedGpuGroupDiagStatsForContext(context))
    {
        RLTraceLog(RL_E_LOG_WARNING, "shared-gpu-diag-scope-selftest: group reset for context failed");
        goto cleanup;
    }

    rejectStats = RLGetSharedGpuTrackingRejectDiagStats();
    if (rejectStats.unregisteredReleaseRejectCount == 0ull)
    {
        RLTraceLog(RL_E_LOG_WARNING,
                   "shared-gpu-diag-scope-selftest: group reset incorrectly cleared global reject counters");
        goto cleanup;
    }

    RLResetSharedGpuTrackingRejectDiagStats();
    rejectStats = RLGetSharedGpuTrackingRejectDiagStats();
    if ((rejectStats.unregisteredRetainRejectCount != 0ull) ||
        (rejectStats.unregisteredReleaseRejectCount != 0ull))
    {
        RLTraceLog(RL_E_LOG_WARNING,
                   "shared-gpu-diag-scope-selftest: reject reset failed (retain=%llu release=%llu)",
                   rejectStats.unregisteredRetainRejectCount,
                   rejectStats.unregisteredReleaseRejectCount);
        goto cleanup;
    }

    RLTraceLog(RL_E_LOG_INFO, "shared-gpu-diag-scope-selftest: PASSED");
    result = 0;

cleanup:
    if (renderTexture.id != 0u) RLUnloadRenderTexture(renderTexture);
    RLSetSharedGpuTrackingMode((int)originalTrackingMode);
    if (!cumulativeEnabledBefore) RLDisableSharedGpuCumulativeDiagStats();
    RLCloseWindow();
    RLDestroyContext(context);
    return result;
}

static void FlushPendingSharedDeletesForCurrentWindow(const char *tag)
{
    void *hwnd = RLGetWindowHandle();
    if (hwnd != NULL)
    {
        if (!RLDeletePendingSharedGpuResourcesByHandle(hwnd, 1))
        {
            RLTraceLog(RL_E_LOG_WARNING, "%s: RLDeletePendingSharedGpuResourcesByHandle failed", tag);
        }
    }
    else
    {
        if (!RLDeletePendingSharedGpuResources())
        {
            RLTraceLog(RL_E_LOG_WARNING, "%s: RLDeletePendingSharedGpuResources failed", tag);
        }
    }
}

static void ValidateSharedObjectIdApis(void)
{
    // Create one object for each new low-level shared API and verify retain/release pairs.
    float triangle[9] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    unsigned int vboId = rlLoadVertexBuffer(triangle, (int)sizeof(triangle), false);
    unsigned int vaoId = rlLoadVertexArray();
    unsigned int fboId = rlLoadFramebuffer();
    unsigned int rboId = rlLoadTextureDepth(64, 64, true);   // returns renderbuffer id when useRenderBuffer=true

    if ((vboId != 0) && (!RLSharedRetainBuffer(vboId) || !RLSharedReleaseBuffer(vboId)))
        RLTraceLog(RL_E_LOG_WARNING, "verify: buffer retain/release validation failed (id=%u)", vboId);
    if ((vaoId != 0) && (!RLSharedRetainVertexArray(vaoId) || !RLSharedReleaseVertexArray(vaoId)))
        RLTraceLog(RL_E_LOG_WARNING, "verify: vertex-array retain/release validation failed (id=%u)", vaoId);
    if ((fboId != 0) && (!RLSharedRetainFramebuffer(fboId) || !RLSharedReleaseFramebuffer(fboId)))
        RLTraceLog(RL_E_LOG_WARNING, "verify: framebuffer retain/release validation failed (id=%u)", fboId);
    if ((rboId != 0) && (!RLSharedRetainRenderbuffer(rboId) || !RLSharedReleaseRenderbuffer(rboId)))
        RLTraceLog(RL_E_LOG_WARNING, "verify: renderbuffer retain/release validation failed (id=%u)", rboId);

    RLDebugDumpSharedGpuState("recreate: after low-level retain/release validation");
    PrintSharedGpuDiagStatsLine("recreate_after_low_level_validation");

    if (vaoId != 0) rlUnloadVertexArray(vaoId);
    if (vboId != 0) rlUnloadVertexBuffer(vboId);
    if (fboId != 0) rlUnloadFramebuffer(fboId);
    if (rboId != 0) RLSharedReleaseRenderbuffer(rboId); // depth renderbuffer created by rlLoadTextureDepth()
}

static void ValidateSharedOwnerApisMainToWorker(void)
{
    if ((gMainCtx == NULL) || (gWorkerCtx == NULL) || (gPack.tex.id == 0))
    {
        RLTraceLog(RL_E_LOG_WARNING, "owner-api: skipped (missing context or texture)");
        return;
    }

    RLContext *ownerBefore = RLGetSharedObjectOwnerContext(RL_SHARED_OBJECT_TEXTURE, gPack.tex.id);
    RLContext *objOwnerBefore = RLGetTextureObjectOwnerContext(gPack.tex);
    RLTraceLog(RL_E_LOG_INFO,
               "owner-api: before transfer (tex=%u) sharedOwner=%p objectOwner=%p",
               gPack.tex.id, (void *)ownerBefore, (void *)objOwnerBefore);

    bool sharedTransferOk = RLTryTransferSharedObjectOwner(RL_SHARED_OBJECT_TEXTURE, gPack.tex.id, gWorkerCtx);
    if (!sharedTransferOk)
    {
        RLTraceLog(RL_E_LOG_WARNING,
                   "owner-api: transfer main->worker failed (tex=%u)",
                   gPack.tex.id);
        return;
    }

    RLContext *ownerAfter = RLGetSharedObjectOwnerContext(RL_SHARED_OBJECT_TEXTURE, gPack.tex.id);
    RLContext *objOwnerAfter = RLGetTextureObjectOwnerContext(gPack.tex);
    if ((ownerAfter == gWorkerCtx) && (objOwnerAfter == gWorkerCtx))
    {
        RLTraceLog(RL_E_LOG_INFO,
                   "owner-api: after transfer main->worker (tex=%u) sharedOwner=%p objectOwner=%p",
                   gPack.tex.id, (void *)ownerAfter, (void *)objOwnerAfter);
        RLTraceLog(RL_E_LOG_INFO, "owner-api: transfer main->worker success (tex=%u)", gPack.tex.id);
#if defined(_WIN32)
        InterlockedExchange(&gOwnerTransferMainToWorkerDone, 1);
#endif
    }
    else
    {
        RLTraceLog(RL_E_LOG_WARNING,
                   "owner-api: owner mismatch after transfer (tex=%u, sharedOwner=%p objectOwner=%p)",
                   gPack.tex.id, (void *)ownerAfter, (void *)objOwnerAfter);
    }
}

static void ValidateSharedOwnerApisWorkerToMain(void)
{
    if ((gMainCtx == NULL) || (gWorkerCtx == NULL) || (gPack.tex.id == 0))
    {
        RLTraceLog(RL_E_LOG_WARNING, "owner-api: worker->main skipped (missing context or texture)");
        return;
    }

    RLContext *ownerBefore = RLGetSharedObjectOwnerContext(RL_SHARED_OBJECT_TEXTURE, gPack.tex.id);
    RLContext *objOwnerBefore = RLGetTextureObjectOwnerContext(gPack.tex);
    RLTraceLog(RL_E_LOG_INFO,
               "owner-api: worker->main before transfer (tex=%u) sharedOwner=%p objectOwner=%p",
               gPack.tex.id, (void *)ownerBefore, (void *)objOwnerBefore);

    bool sharedTransferOk = RLTryTransferSharedObjectOwner(RL_SHARED_OBJECT_TEXTURE, gPack.tex.id, gMainCtx);
    if (!sharedTransferOk)
    {
        RLTraceLog(RL_E_LOG_WARNING,
                   "owner-api: transfer worker->main failed (tex=%u)",
                   gPack.tex.id);
        return;
    }

    RLContext *ownerAfter = RLGetSharedObjectOwnerContext(RL_SHARED_OBJECT_TEXTURE, gPack.tex.id);
    RLContext *objOwnerAfter = RLGetTextureObjectOwnerContext(gPack.tex);
    if ((ownerAfter == gMainCtx) && (objOwnerAfter == gMainCtx))
    {
        RLTraceLog(RL_E_LOG_INFO,
                   "owner-api: after transfer worker->main (tex=%u) sharedOwner=%p objectOwner=%p",
                   gPack.tex.id, (void *)ownerAfter, (void *)objOwnerAfter);
        RLTraceLog(RL_E_LOG_INFO, "owner-api: transfer worker->main success (tex=%u)", gPack.tex.id);
#if defined(_WIN32)
        InterlockedExchange(&gOwnerTransferMainToWorkerDone, 0);
#endif
    }
    else
    {
        RLTraceLog(RL_E_LOG_WARNING,
                   "owner-api: worker->main owner mismatch after transfer (tex=%u, sharedOwner=%p objectOwner=%p)",
                   gPack.tex.id, (void *)ownerAfter, (void *)objOwnerAfter);
    }
}

#if defined(_WIN32)
static HANDLE gEvtReady = NULL;
static HANDLE gEvtWorkerExit = NULL;
static HANDLE gEvtWorkerDone = NULL;
static HANDLE gEvtWorkerHeld = NULL;

static bool BeginSharedShaderSection(int isWorker, RLShader shader)
{
    (void)isWorker;
    if (gShaderMode == SHARED_SHADER_MODE_PHASED_SHARED) return RLBeginSharedShaderUse(shader, RL_SHARED_SHADER_USE_PHASED);
    if (gShaderMode == SHARED_SHADER_MODE_LOCKED_SHARED) return RLBeginSharedShaderUse(shader, RL_SHARED_SHADER_USE_LOCKED);
    RLBeginShaderMode(shader);
    return true;
}

static void EndSharedShaderSection(int isWorker)
{
    (void)isWorker;
    RLEndShaderMode();
    if (gShaderMode == SHARED_SHADER_MODE_PHASED_SHARED) RLSharedShaderUseEnd(gPack.shader, RL_SHARED_SHADER_USE_PHASED);
    else if (gShaderMode == SHARED_SHADER_MODE_LOCKED_SHARED) RLSharedShaderUseEnd(gPack.shader, RL_SHARED_SHADER_USE_LOCKED);
}

static unsigned __stdcall WorkerThread(void *arg)
{
    (void)arg;

    RLContext *ctx = RLCreateContext();
    RLSetCurrentContext(ctx);
    gWorkerCtx = ctx;
    if (!RLContextSetResourceShareMode(ctx, RL_CONTEXT_SHARE_WITH_PRIMARY, NULL))
    {
        RLTraceLog(RL_E_LOG_WARNING, "worker: RLContextSetResourceShareMode failed");
    }

    RLSetConfigFlags(RL_E_FLAG_WINDOW_RESIZABLE | RL_E_FLAG_WINDOW_EVENT_THREAD);
    RLInitWindow(640, 360, "raylib [shared-gpu] worker (shared context)");
    RLSetTargetFPS(60);

    if (!RLIsWindowReady())
    {
        RLTraceLog(RL_E_LOG_WARNING, "worker: window init failed");
        if (gEvtWorkerDone) SetEvent(gEvtWorkerDone);
        RLDestroyContext(ctx);
        return 0;
    }

    WaitForSingleObject(gEvtReady, INFINITE);

    const bool workerUsesSharedShader = (gShaderMode != SHARED_SHADER_MODE_LOCAL_PER_WINDOW);
    RLShader workerLocalShader = { 0 };

    // Each context that holds a long-lived reference must retain it.
    if (!RLRetainTextureObject(gPack.tex)) RLTraceLog(RL_E_LOG_WARNING, "worker: RLRetainTextureObject failed");
    if (workerUsesSharedShader)
    {
        if (!RLSharedRetainShader(gPack.shader)) RLTraceLog(RL_E_LOG_WARNING, "worker: RLSharedRetainShader failed");
    }
    else
    {
        workerLocalShader = LoadTintShaderFromMemory();
    }
    if (!RLRetainRenderTextureObject(gPack.rt)) RLTraceLog(RL_E_LOG_WARNING, "worker: RLRetainRenderTextureObject failed");

    if (gEvtWorkerHeld) SetEvent(gEvtWorkerHeld);

    RLDebugDumpSharedGpuState("worker: after retain");
    PrintSharedGpuDiagStatsLine("worker_after_retain");

    bool unloaded = false;
    double workerStartTime = RLGetTime();

    while (!RLWindowShouldClose() && WaitForSingleObject(gEvtWorkerExit, 0) == WAIT_TIMEOUT)
    {
        double elapsedTime = RLGetTime() - workerStartTime;

        // Use the shared render texture ONLY in this thread to avoid cross-thread hazards.
        if (!unloaded)
        {
            RLBeginTextureMode(gPack.rt);
            RLClearBackground((RLColor){ 20, 20, 30, 255 });
            RLDrawCircle(128 + (int)(80.0*sin(elapsedTime*2.0)), 96, 42, (RLColor){ 80, 160, 255, 255 });
            RLDrawText("RenderTexture updated by worker", 10, 10, 16, RAYWHITE);
            RLEndTextureMode();
        }

        RLBeginDrawing();
        RLClearBackground((RLColor){ 30, 30, 30, 255 });

        if (!unloaded)
        {
            RLDrawText(GetShaderModeLabel(gShaderMode), 20, 20, 18, RAYWHITE);
            RLDrawText("Worker window", 20, 44, 16, LIGHTGRAY);

            if (!workerUsesSharedShader)
            {
                RLBeginShaderMode(workerLocalShader);
                RLDrawTexture(gPack.tex, 20, 70, RAYWHITE);
                RLEndShaderMode();
            }
            else
            {
                bool began = BeginSharedShaderSection(1, gPack.shader);
                if (began)
                {
                    RLDrawTexture(gPack.tex, 20, 70, RAYWHITE);
                    EndSharedShaderSection(1);
                }
                else
                {
                    RLDrawTexture(gPack.tex, 20, 70, (RLColor){ 190, 190, 190, 255 });
                }
            }

            RLDrawTextureRec(gPack.rt.texture,
                             (RLRectangle){ 0, 0, (float)gPack.rt.texture.width, -(float)gPack.rt.texture.height },
                             (RLVector2){ 260, 70 }, RAYWHITE);

            RLDrawText("At ~4s worker unloads refs", 20, 320, 14, LIGHTGRAY);
        }
        else
        {
            RLDrawText("Worker: unloaded its refs; main should keep resources alive", 20, 20, 18, RAYWHITE);
            RLDrawText("Closing in ~2s...", 20, 50, 14, LIGHTGRAY);
        }

        RLEndDrawing();

        // After ~4 seconds, unload from this context (decrements share-group refcount).
        if (!unloaded && elapsedTime > 4.0)
        {
            if (InterlockedCompareExchange(&gOwnerTransferMainToWorkerDone, 0, 0) != 0)
            {
                ValidateSharedOwnerApisWorkerToMain();
            }

            RLUnloadTexture(gPack.tex);
            if (workerUsesSharedShader) RLUnloadShader(gPack.shader);
            else if (workerLocalShader.id != 0) RLUnloadShader(workerLocalShader);
            RLUnloadRenderTexture(gPack.rt);

            // Drain any deferred deletes on this context.
            FlushPendingSharedDeletesForCurrentWindow("worker");
            RLDebugDumpSharedGpuState("worker: after unload+flush");
            PrintSharedGpuDiagStatsLine("worker_after_unload_flush");
            unloaded = true;
        }

        // Exit after ~6 seconds total.
        if (elapsedTime > 6.0) break;
    }

    // If user closes the worker window early (before the timed unload), make sure
    // we still drop this context's share-group references. Otherwise the primary
    // context will observe a leaked refcount when the share-group is destroyed.
    if (!unloaded)
    {
        if (InterlockedCompareExchange(&gOwnerTransferMainToWorkerDone, 0, 0) != 0)
        {
            ValidateSharedOwnerApisWorkerToMain();
        }

        RLUnloadTexture(gPack.tex);
        if (workerUsesSharedShader) RLUnloadShader(gPack.shader);
        else if (workerLocalShader.id != 0) RLUnloadShader(workerLocalShader);
        RLUnloadRenderTexture(gPack.rt);
        FlushPendingSharedDeletesForCurrentWindow("worker");
        RLDebugDumpSharedGpuState("worker: cleanup on early close");
        PrintSharedGpuDiagStatsLine("worker_cleanup_on_early_close");
        unloaded = true;
    }

    RLCloseWindow();
    RLDestroyContext(ctx);

    SetEvent(gEvtWorkerDone);
    return 0;
}
#endif

static RLShader LoadTintShaderFromMemory(void)
{
    const char *fs = RLTextFormat(
        "#version %i\n"
        "in vec2 fragTexCoord;\n"
        "in vec4 fragColor;\n"
        "uniform sampler2D texture0;\n"
        "uniform vec4 colDiffuse;\n"
        "out vec4 finalColor;\n"
        "void main() {\n"
        "    vec4 texel = texture(texture0, fragTexCoord);\n"
        "    finalColor = texel*fragColor*colDiffuse;\n"
        "}\n", GLSL_VERSION);

    // Use internal default vertex shader (vsCode = NULL)
    return RLLoadShaderFromMemory(NULL, fs);
}

int main(int argc, char **argv)
{
#if !defined(_WIN32)
    // For simplicity this example focuses on Win32 threading; adapt as needed for other platforms.
    RLTraceLog(LOG_WARNING, "This example is currently implemented for _WIN32 only.");
    return 0;
#else
    if (HasCommandLineFlag(argc, argv, "--trace-reentry-selftest"))
    {
        return RunTraceReentrySelfTest();
    }

    if (HasCommandLineFlag(argc, argv, "--shared-gpu-diag-scope-selftest"))
    {
        return RunSharedGpuDiagScopeSelfTest();
    }

    const int screenWidth = 900;
    const int screenHeight = 520;
    gShaderMode = ParseShaderModeFromArgv(argc, argv);
    RLTraceLog(RL_E_LOG_INFO, "shared-shader strategy: %s", GetShaderModeLabel(gShaderMode));
    RLEnableMemoryDiagStats();

    // ---- Phase A: primary window/context (main thread) ----
    RLContext *mainCtx = RLCreateContext();
    RLSetCurrentContext(mainCtx);
    gMainCtx = mainCtx;

    RLSetConfigFlags(RL_E_FLAG_WINDOW_RESIZABLE | RL_E_FLAG_WINDOW_EVENT_THREAD);
    RLInitWindow(screenWidth, screenHeight, "raylib [shared-gpu] primary (owner)");
    RLSetTargetFPS(60);

    if (!RLIsWindowReady())
    {
        RLTraceLog(RL_E_LOG_WARNING, "main: window init failed");
        RLDestroyContext(mainCtx);
        PrintMemDiagStatsLine("init_fail_post_destroy", RLGetMemoryDiagStats());
        printf("MEMDIAG_AUTOTEST: result=INIT_FAILED\n");
        return 1;
    }

    // Create shared resources in primary context.
    RLImage img = RLGenImageChecked(240, 240, 20, 20, (RLColor){ 60, 200, 140, 255 }, (RLColor){ 30, 60, 200, 255 });
    gPack.tex = RLLoadTextureFromImage(img);
    RLUnloadImage(img);

    gPack.shader = LoadTintShaderFromMemory();
    gPack.rt = RLLoadRenderTexture(240, 180);

    RLDebugDumpSharedGpuState("main: after create");
    PrintSharedGpuDiagStatsLine("main_after_create");

    // ---- Phase B: worker window/context (different thread) ----
    gEvtReady = CreateEventA(NULL, TRUE, FALSE, NULL);
    gEvtWorkerExit = CreateEventA(NULL, TRUE, FALSE, NULL);
    gEvtWorkerDone = CreateEventA(NULL, TRUE, FALSE, NULL);
    gEvtWorkerHeld = CreateEventA(NULL, TRUE, FALSE, NULL);

    uintptr_t workerThreadHandle = _beginthreadex(NULL, 0, WorkerThread, NULL, 0, NULL);
    if (workerThreadHandle == 0)
    {
        RLTraceLog(RL_E_LOG_WARNING, "main: failed to start worker thread");
        SetEvent(gEvtWorkerDone);
    }
    SetEvent(gEvtReady);

    double primaryPhaseStartTime = RLGetTime();
    bool mainUnloaded = false;
    bool ownerApiValidated = false;

    while (!RLWindowShouldClose())
    {
        double primaryElapsedTime = RLGetTime() - primaryPhaseStartTime;
        const bool workerHeld = (gEvtWorkerHeld && WaitForSingleObject(gEvtWorkerHeld, 0) == WAIT_OBJECT_0);
        const bool workerDone = (WaitForSingleObject(gEvtWorkerDone, 0) == WAIT_OBJECT_0);

        RLBeginDrawing();
        RLClearBackground((RLColor){ 25, 25, 28, 255 });

        RLDrawText(GetShaderModeLabel(gShaderMode), 20, 18, 18, RAYWHITE);
        RLDrawText(RLTextFormat("Primary t=%.2fs", primaryElapsedTime), 20, 46, 16, LIGHTGRAY);

        if (!mainUnloaded)
        {
            if (gShaderMode == SHARED_SHADER_MODE_LOCAL_PER_WINDOW)
            {
                RLBeginShaderMode(gPack.shader);
                RLDrawTexture(gPack.tex, 20, 80, RAYWHITE);
                RLEndShaderMode();
            }
            else
            {
                // In phased mode, do not block on turn handoff until worker is fully ready.
                // Otherwise main render thread can stall before pumping thread tasks, which in turn
                // can block worker shared-context creation barrier on Win32/WGL.
                const bool allowPhasedWait = (gShaderMode != SHARED_SHADER_MODE_PHASED_SHARED) || workerHeld || workerDone;
                if (!allowPhasedWait)
                {
                    RLBeginShaderMode(gPack.shader);
                    RLDrawTexture(gPack.tex, 20, 80, RAYWHITE);
                    RLEndShaderMode();
                }
                else
                {
                    bool began = BeginSharedShaderSection(0, gPack.shader);
                    if (began)
                    {
                        RLDrawTexture(gPack.tex, 20, 80, RAYWHITE);
                        EndSharedShaderSection(0);
                    }
                    else
                    {
                        RLDrawTexture(gPack.tex, 20, 80, (RLColor){ 190, 190, 190, 255 });
                    }
                }
            }
        }
        else
        {
            RLDrawText("Primary: resources already unloaded", 20, 80, 18, LIGHTGRAY);
        }

        RLDrawText("Use --mode=1/2/3 to test each strategy.", 20, 330, 14, LIGHTGRAY);
        RLDrawText("At ~8s primary unloads and flushes (target: refcount -> 0).", 20, 350, 14, LIGHTGRAY);
        RLDrawText("Then primary window closes and recreates a new context.", 20, 370, 14, LIGHTGRAY);

        if (!ownerApiValidated && workerHeld)
        {
            ValidateSharedOwnerApisMainToWorker();
            ownerApiValidated = true;
        }

        if (!mainUnloaded && ((primaryElapsedTime > 8.0 && (workerHeld || workerDone)) || (primaryElapsedTime > 15.0)))
        {
            // Unload in primary context: should drop refcounts to 0 and queue deletes.
            RLUnloadTexture(gPack.tex);
            RLUnloadShader(gPack.shader);
            RLUnloadRenderTexture(gPack.rt);
            FlushPendingSharedDeletesForCurrentWindow("main");
            RLDebugDumpSharedGpuState("main: after unload+flush");
            PrintSharedGpuDiagStatsLine("main_after_unload_flush");
            mainUnloaded = true;
        }

        bool wantBreak = false;

        // After worker finished and primary unloaded, exit.
        if (mainUnloaded && WaitForSingleObject(gEvtWorkerDone, 0) == WAIT_OBJECT_0)
        {
            RLDrawText("Worker done. Press ESC or close to continue...", 20, 410, 14, LIGHTGRAY);
            if (primaryElapsedTime > 10.0) wantBreak = true;
        }

        RLEndDrawing();

        if (wantBreak) break;
    }

    // Tell worker to exit (if still running)
    SetEvent(gEvtWorkerExit);
    if (workerThreadHandle != 0)
    {
        WaitForSingleObject((HANDLE)workerThreadHandle, INFINITE);
        CloseHandle((HANDLE)workerThreadHandle);
    }

    // If user closes the worker window early (before the timed unload), make sure
    // we still drop this context's share-group references. Otherwise the primary
    // context will observe a leaked refcount when the share-group is destroyed.
    if (!mainUnloaded)
    {
        // Unload in primary context: should drop refcounts to 0 and queue deletes.
        RLUnloadTexture(gPack.tex);
        RLUnloadShader(gPack.shader);
        RLUnloadRenderTexture(gPack.rt);
        FlushPendingSharedDeletesForCurrentWindow("main");
        RLDebugDumpSharedGpuState("main: cleanup on early close");
        PrintSharedGpuDiagStatsLine("main_cleanup_on_early_close");
        mainUnloaded = true;
    }

    RLDebugDumpSharedGpuState("main: before close");
    PrintSharedGpuDiagStatsLine("main_before_close");
    RLCloseWindow();
    RLDestroyContext(mainCtx);
    PrintMemDiagStatsLine("phase_b_post_destroy", RLGetMemoryDiagStats());
    gMainCtx = NULL;
    gWorkerCtx = NULL;

    CloseHandle(gEvtReady);
    CloseHandle(gEvtWorkerExit);
    CloseHandle(gEvtWorkerDone);
    CloseHandle(gEvtWorkerHeld);

    // ---- Phase C: recreate a brand new context/window on the same thread ----
    RLContext *ctx2 = RLCreateContext();
    RLSetCurrentContext(ctx2);
    RLSetConfigFlags(RL_E_FLAG_WINDOW_EVENT_THREAD);
    RLInitWindow(720, 420, "raylib [shared-gpu] recreated context");
    RLSetTargetFPS(60);

    if (!RLIsWindowReady())
    {
        RLTraceLog(RL_E_LOG_WARNING, "recreate: window init failed");
        RLDestroyContext(ctx2);
        PrintMemDiagStatsLine("recreate_init_fail_post_destroy", RLGetMemoryDiagStats());
        printf("MEMDIAG_AUTOTEST: result=INIT_FAILED\n");
        return 1;
    }

    RLImage img2 = RLGenImageGradientLinear(256, 256, 0, (RLColor){ 255, 90, 90, 255 }, (RLColor){ 90, 255, 180, 255 });
    RLTexture2D tex2 = RLLoadTextureFromImage(img2);
    RLUnloadImage(img2);

    RLShader sh2 = LoadTintShaderFromMemory();
    RLRenderTexture2D rt2 = RLLoadRenderTexture(256, 256);

    RLDebugDumpSharedGpuState("recreate: after create");
    PrintSharedGpuDiagStatsLine("recreate_after_create");
    ValidateSharedObjectIdApis();

    double recreatePhaseStartTime = RLGetTime();
    while (!RLWindowShouldClose())
    {
        double recreatePhaseElapsedTime = RLGetTime() - recreatePhaseStartTime;

        RLBeginTextureMode(rt2);
        RLClearBackground((RLColor){ 10, 10, 18, 255 });
        RLDrawText("Recreated context", 20, 20, 22, RAYWHITE);
        RLDrawCircle(128, 140, 50.0f + 15.0f*(float)sin(recreatePhaseElapsedTime*3.0), (RLColor){ 200, 220, 255, 255 });
        RLEndTextureMode();

        RLBeginDrawing();
        RLClearBackground((RLColor){ 20, 20, 20, 255 });
        RLDrawText("Phase C: create/unload/flush again", 20, 20, 18, RAYWHITE);

        RLBeginShaderMode(sh2);
        RLDrawTexture(tex2, 20, 60, RAYWHITE);
        RLEndShaderMode();

        RLDrawTextureRec(rt2.texture,
                         (RLRectangle){ 0, 0, (float)rt2.texture.width, -(float)rt2.texture.height },
                         (RLVector2){ 360, 60 }, RAYWHITE);

        RLDrawText("Auto-unload at ~3s", 20, 380, 14, LIGHTGRAY);
        RLEndDrawing();

        if (recreatePhaseElapsedTime > 3.0) break;
    }

    RLUnloadTexture(tex2);
    RLUnloadShader(sh2);
    RLUnloadRenderTexture(rt2);
    FlushPendingSharedDeletesForCurrentWindow("recreate");
    RLDebugDumpSharedGpuState("recreate: after unload+flush");
    PrintSharedGpuDiagStatsLine("recreate_after_unload_flush");

    RLCloseWindow();
    RLDestroyContext(ctx2);

    RLMemoryDiagStats finalStats = RLGetMemoryDiagStats();
    PrintMemDiagStatsLine("post_destroy", finalStats);
    if (finalStats.currentOutstandingBytes > 0ull)
    {
        printf("MEMDIAG_AUTOTEST: result=LEAK_DETECTED\n");
        RLDumpMemoryLeaks();
        return 3;
    }
    printf("MEMDIAG_AUTOTEST: result=NO_LEAK_DETECTED\n");

    return 0;
#endif
}
