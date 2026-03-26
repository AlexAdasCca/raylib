/*******************************************************************************************
*
*   raylib [core] example - Win32 event thread diagnostics (interactive stress test)
*
*   This example is intended for the Win32 event-thread enabled raylib/GLFW build.
*   It provides interactive input + optional programmatic "stress modes" to catch
*   regressions in event draining, task posting, and heap allocation/free behavior.
*
*   Controls:
*     - Left click: place marker
*     - Hold LMB: paint markers
*     - Right click: remove last marker
*     - Middle click or C: clear markers
*     - Mouse wheel: change marker size
*     - R: reset diagnostics counters
*     - T: toggle diagnostics runtime counting
*     - G: toggle telemetry graphs
*     - H: toggle help overlay
*     - J: toggle window position jitter (stresses window-pos callbacks)
*     - U: toggle window resize jitter (stresses fbsize/scale callbacks)
*     - W: toggle mouse warp (stresses mouse-move callbacks)
*
********************************************************************************************/

#include "raylib.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#include <process.h>     // _beginthreadex
#include "../../src/rglfwglobal.h"
#ifndef CP_UTF8
#define CP_UTF8 65001
#endif

typedef struct SharedScenarioState
{
    volatile LONG sameThreadRunning;
    volatile LONG crossThreadRunning;
    volatile LONG sameThreadStartCount;
    volatile LONG crossThreadStartCount;
    volatile LONG sameThreadExpectedRejectCount;
    volatile LONG sameThreadUnexpectedSuccessCount;
    volatile LONG sameThreadFailureCount;
    volatile LONG crossThreadSuccessCount;
    volatile LONG crossThreadFailureCount;
    HANDLE sameThreadHandle;
    HANDLE crossThreadHandle;
    RLContext *mainContext;
} SharedScenarioState;

static SharedScenarioState gSharedScenario = { 0 };
static volatile LONG gTrackedObjectDiagConfigured = 0;

static void SharedDiagConfigureTrackedObjectDiagnostics(void)
{
    if (InterlockedCompareExchange(&gTrackedObjectDiagConfigured, 1, 0) != 0) return;

    RLSetTrackedObjectDiagFlags(
        RL_TRACKED_OBJECT_DIAG_LOG_RELEASE_CALLS |
        RL_TRACKED_OBJECT_DIAG_DUMP_STATE_ON_RELEASE_MISS |
        RL_TRACKED_OBJECT_DIAG_INCLUDE_TOMBSTONES |
        RL_TRACKED_OBJECT_DIAG_LOG_PROMOTIONS |
        RL_TRACKED_OBJECT_DIAG_AUDIT_PROMOTIONS);

    RLTraceLog(RL_E_LOG_INFO,
               "shared-diag: tracked-object diagnostics enabled (flags=0x%x)",
               RLGetTrackedObjectDiagFlags());
}

static void SharedDiagDumpTrackedObjects(const char *label)
{
    SharedDiagConfigureTrackedObjectDiagnostics();
    RLTraceLog(RL_E_LOG_INFO,
               "shared-diag: tracked-object dump requested: %s",
               (label != NULL) ? label : "(null)");
    RLDebugDumpTrackedObjectState(label);
}

static void SharedDiagCloseThreadHandle(HANDLE *handleSlot)
{
    if ((handleSlot == NULL) || (*handleSlot == NULL)) return;
    CloseHandle(*handleSlot);
    *handleSlot = NULL;
}

static void SharedDiagJoinThreadForShutdown(HANDLE *handleSlot, volatile LONG *runningFlag, const char *tag)
{
    if ((handleSlot == NULL) || (*handleSlot == NULL)) return;

    DWORD waitResult = WaitForSingleObject(*handleSlot, 3000);
    if (waitResult == WAIT_TIMEOUT)
    {
        RLTraceLog(RL_E_LOG_WARNING,
                   "shared-diag: %s thread still running after 3000 ms, waiting until exit to keep cleanup ordered",
                   (tag != NULL) ? tag : "(unknown)");
        waitResult = WaitForSingleObject(*handleSlot, INFINITE);
    }

    if (waitResult != WAIT_OBJECT_0)
    {
        RLTraceLog(RL_E_LOG_WARNING,
                   "shared-diag: %s thread wait failed (result=%lu), closing handle anyway",
                   (tag != NULL) ? tag : "(unknown)",
                   (unsigned long)waitResult);
    }

    SharedDiagCloseThreadHandle(handleSlot);
    if (runningFlag != NULL) InterlockedExchange(runningFlag, 0);
}

static unsigned __stdcall SharedDiagSameThreadMultiWindowThread(void *arg)
{
    (void)arg;
    RLContext *ctxPrimary = NULL;
    RLContext *ctxSecondary = NULL;
    bool primaryReady = false;
    bool secondaryReady = false;

    ctxPrimary = RLCreateContext();
    if (ctxPrimary == NULL)
    {
        InterlockedIncrement(&gSharedScenario.sameThreadFailureCount);
        InterlockedExchange(&gSharedScenario.sameThreadRunning, 0);
        return 0;
    }

    RLSetCurrentContext(ctxPrimary);
    SharedDiagConfigureTrackedObjectDiagnostics();
    RLSetConfigFlags(RL_E_FLAG_WINDOW_EVENT_THREAD | RL_E_FLAG_WINDOW_RESIZABLE);
    RLInitWindow(360, 220, "diag same-thread primary");
    primaryReady = RLIsWindowReady();
    SharedDiagDumpTrackedObjects("same-thread-primary-after-init");

    if (!primaryReady)
    {
        InterlockedIncrement(&gSharedScenario.sameThreadFailureCount);
        RLDestroyContext(ctxPrimary);
        InterlockedExchange(&gSharedScenario.sameThreadRunning, 0);
        return 0;
    }

    ctxSecondary = RLCreateContext();
    if (ctxSecondary == NULL)
    {
        InterlockedIncrement(&gSharedScenario.sameThreadFailureCount);
        RLSetCurrentContext(ctxPrimary);
        RLCloseWindow();
        RLDestroyContext(ctxPrimary);
        InterlockedExchange(&gSharedScenario.sameThreadRunning, 0);
        return 0;
    }

    RLSetCurrentContext(ctxSecondary);
    SharedDiagConfigureTrackedObjectDiagnostics();
    RLSetConfigFlags(RL_E_FLAG_WINDOW_EVENT_THREAD | RL_E_FLAG_WINDOW_RESIZABLE);
    RLInitWindow(320, 200, "diag same-thread secondary");
    secondaryReady = RLIsWindowReady();
    SharedDiagDumpTrackedObjects("same-thread-secondary-after-init-attempt");

    if (secondaryReady)
    {
        InterlockedIncrement(&gSharedScenario.sameThreadUnexpectedSuccessCount);
        SharedDiagDumpTrackedObjects("same-thread-secondary-before-close");
        RLCloseWindow();
        SharedDiagDumpTrackedObjects("same-thread-secondary-after-close");
    }
    else InterlockedIncrement(&gSharedScenario.sameThreadExpectedRejectCount);

    RLDestroyContext(ctxSecondary);

    RLSetCurrentContext(ctxPrimary);
    SharedDiagDumpTrackedObjects("same-thread-primary-before-close");
    RLCloseWindow();
    SharedDiagDumpTrackedObjects("same-thread-primary-after-close");
    RLDestroyContext(ctxPrimary);

    InterlockedExchange(&gSharedScenario.sameThreadRunning, 0);
    return 0;
}

static unsigned __stdcall SharedDiagCrossThreadSharedWindowThread(void *arg)
{
    (void)arg;
    RLContext *ctx = RLCreateContext();
    bool shareSet = false;

    if (ctx == NULL)
    {
        InterlockedIncrement(&gSharedScenario.crossThreadFailureCount);
        InterlockedExchange(&gSharedScenario.crossThreadRunning, 0);
        return 0;
    }

    RLSetCurrentContext(ctx);
    SharedDiagConfigureTrackedObjectDiagnostics();
    shareSet = RLContextSetResourceShareMode(ctx, RL_CONTEXT_SHARE_WITH_PRIMARY, NULL);
    if (!shareSet || !RLContextValidateResourceShareConfig(ctx))
    {
        InterlockedIncrement(&gSharedScenario.crossThreadFailureCount);
        RLDestroyContext(ctx);
        InterlockedExchange(&gSharedScenario.crossThreadRunning, 0);
        return 0;
    }

    RLSetConfigFlags(RL_E_FLAG_WINDOW_EVENT_THREAD | RL_E_FLAG_WINDOW_RESIZABLE);
    RLInitWindow(320, 200, "diag cross-thread shared");
    SharedDiagDumpTrackedObjects("cross-thread-after-init");
    if (RLIsWindowReady())
    {
        InterlockedIncrement(&gSharedScenario.crossThreadSuccessCount);
        Sleep(3000);
        SharedDiagDumpTrackedObjects("cross-thread-before-close");
        RLCloseWindow();
        SharedDiagDumpTrackedObjects("cross-thread-after-close");
    }
    else InterlockedIncrement(&gSharedScenario.crossThreadFailureCount);

    RLDestroyContext(ctx);
    InterlockedExchange(&gSharedScenario.crossThreadRunning, 0);
    return 0;
}

static void SharedDiagTryStartSameThreadMultiWindowTest(void)
{
    if (InterlockedCompareExchange(&gSharedScenario.sameThreadRunning, 1, 0) != 0) return;
    InterlockedIncrement(&gSharedScenario.sameThreadStartCount);
    SharedDiagDumpTrackedObjects("main-before-start-same-thread-test");
    uintptr_t handleValue = _beginthreadex(NULL, 0, SharedDiagSameThreadMultiWindowThread, NULL, 0, NULL);
    if (handleValue == 0)
    {
        InterlockedIncrement(&gSharedScenario.sameThreadFailureCount);
        InterlockedExchange(&gSharedScenario.sameThreadRunning, 0);
        return;
    }
    gSharedScenario.sameThreadHandle = (HANDLE)handleValue;
}

static void SharedDiagTryStartCrossThreadSharedWindowTest(void)
{
    if (InterlockedCompareExchange(&gSharedScenario.crossThreadRunning, 1, 0) != 0) return;
    InterlockedIncrement(&gSharedScenario.crossThreadStartCount);
    SharedDiagDumpTrackedObjects("main-before-start-cross-thread-test");
    uintptr_t handleValue = _beginthreadex(NULL, 0, SharedDiagCrossThreadSharedWindowThread, NULL, 0, NULL);
    if (handleValue == 0)
    {
        InterlockedIncrement(&gSharedScenario.crossThreadFailureCount);
        InterlockedExchange(&gSharedScenario.crossThreadRunning, 0);
        return;
    }
    gSharedScenario.crossThreadHandle = (HANDLE)handleValue;
}

static void SharedDiagPumpThreadState(void)
{
    if (gSharedScenario.sameThreadHandle != NULL)
    {
        if (WaitForSingleObject(gSharedScenario.sameThreadHandle, 0) == WAIT_OBJECT_0)
        {
            SharedDiagDumpTrackedObjects("main-after-same-thread-test-finished");
            SharedDiagCloseThreadHandle(&gSharedScenario.sameThreadHandle);
        }
    }

    if (gSharedScenario.crossThreadHandle != NULL)
    {
        if (WaitForSingleObject(gSharedScenario.crossThreadHandle, 0) == WAIT_OBJECT_0)
        {
            SharedDiagDumpTrackedObjects("main-after-cross-thread-test-finished");
            SharedDiagCloseThreadHandle(&gSharedScenario.crossThreadHandle);
        }
    }
}
#endif

//------------------------------------------------------------------------------
// Simple marker painter (interactive input stress)
//------------------------------------------------------------------------------
#define MAX_MARKERS 8192
#define TELEMETRY_SAMPLES 240
#define TELEMETRY_CHART_COUNT 8

typedef struct Marker
{
    RLVector2 pos;
    float r;
    RLColor col;
} Marker;

static Marker gMarkers[MAX_MARKERS];
static int gMarkerCount = 0;

// We render markers into a canvas RenderTexture to keep FPS stable even with many markers.
// This makes it easier to spot *event-thread* performance regressions without conflating
// them with draw-call scaling.
static RLRenderTexture2D gCanvas = {0};
static int gCanvasW = 0;
static int gCanvasH = 0;
static bool gCanvasDirty = true;
static bool gDrawMarkersDirect = false; // optional: stress rendering path

typedef struct TelemetrySeries
{
    float frameMs[TELEMETRY_SAMPLES];
    float frameCpuMs[TELEMETRY_SAMPLES];
    float swapCostMs[TELEMETRY_SAMPLES];
    float waitCostMs[TELEMETRY_SAMPLES];
    float residualMs[TELEMETRY_SAMPLES];
    float residualThresholdMs[TELEMETRY_SAMPLES];
    float residualSpikeScore[TELEMETRY_SAMPLES];
    unsigned char residualAnomaly[TELEMETRY_SAMPLES];
    unsigned char focusedState[TELEMETRY_SAMPLES];
    float pumpTasksLast[TELEMETRY_SAMPLES];
    float pumpTimeLastMs[TELEMETRY_SAMPLES];
    float nativeQueueLast[TELEMETRY_SAMPLES];
    int head;
    int count;
} TelemetrySeries;

static TelemetrySeries gTelemetry = { 0 };
static float gWaitCostEmaMs = 0.0f;
static float gResidualEmaMs = 0.0f;
static bool gDerivedEmaReady = false;
static float gResidualMeanMs = 0.0f;
static float gResidualStdMs = 0.0f;
static unsigned long long gFocusedSamples = 0;
static unsigned long long gFocusedAnomalies = 0;
static unsigned long long gUnfocusedSamples = 0;
static unsigned long long gUnfocusedAnomalies = 0;
static float gChartDisplayMax[TELEMETRY_CHART_COUNT] = { 0 };
static bool gChartDisplayMaxReady = false;
static double gChartScaleLastUpdateSec = 0.0;

typedef struct ApiSmokeStatus
{
    int done;
    int textFormatOk;
    int saveTextOk;
    int loadTextOk;
    int saveDataOk;
    int loadDataOk;
    int appDirLen;
    int workDirLen;
    int textPathLen;
    int dataPathLen;
    int fileExistsOk;
    int dirExistsOk;
    int changeDirOk;
    char textPath[512];
    char dataPath[512];
} ApiSmokeStatus;

static ApiSmokeStatus gApiSmoke = { 0 };

typedef struct TelemetrySampleMode
{
    const char *label;
    double intervalSec;
} TelemetrySampleMode;

static const TelemetrySampleMode gTelemetrySampleModes[] =
{
    { "Realtime (per frame)", 0.0 },
    { "100 Hz (10 ms)", 0.01 },
    { "50 Hz (20 ms)", 0.02 },
    { "20 Hz (50 ms)", 0.05 },
    { "10 Hz (100 ms)", 0.10 },
    { "5 Hz (200 ms)", 0.20 },
    { "2 Hz (500 ms)", 0.50 },
    { "1 Hz (1000 ms)", 1.00 }
};

#define TELEMETRY_SAMPLE_MODE_COUNT ((int)(sizeof(gTelemetrySampleModes)/sizeof(gTelemetrySampleModes[0])))

static const char *ShareModeToText(RLContextResourceShareMode mode)
{
    if (mode == RL_CONTEXT_SHARE_WITH_PRIMARY) return "WITH_PRIMARY";
    if (mode == RL_CONTEXT_SHARE_WITH_CONTEXT) return "WITH_CONTEXT";
    return "NONE";
}

static int RunFileIoSmokeCli(const char *rootOverride, const char *uncRootOverride)
{
    int ok = 1;
    int uncEnabled = 0;
    int uncOk = 1;
    int required = 0;
    const char *failStage = "ok";
#if defined(_WIN32)
    const char *sep = "\\";
#else
    const char *sep = "/";
#endif

    char root[4096] = { 0 };
    if ((rootOverride != NULL) && (rootOverride[0] != '\0'))
    {
        RLTextFormatTo(root, (int)sizeof(root), "%s", rootOverride);
    }
    else
    {
        RLTextFormatTo(root, (int)sizeof(root), "%s%stemp%slongpath_smoke", RLGetWorkingDirectory(), sep, sep);
    }

    if (RLMakeDirectory(root) != 0) { ok = 0; failStage = "make-root"; }

    char deepDir[4096] = { 0 };
    RLTextFormatTo(deepDir, (int)sizeof(deepDir), "%s", root);
    for (int i = 0; i < 16; i++)
    {
        char next[4096] = { 0 };
        required = RLTextFormatTo(next, (int)sizeof(next), "%s%ssegment_%02d_abcdefghijklmnopqrstuvwxyz0123456789", deepDir, sep, i);
        if (required >= (int)sizeof(next)) { ok = 0; failStage = "format-deep"; break; }
        RLTextFormatTo(deepDir, (int)sizeof(deepDir), "%s", next);
        if (RLMakeDirectory(deepDir) != 0)
        {
            static char deepFailStage[64] = { 0 };
            RLTextFormatTo(deepFailStage, (int)sizeof(deepFailStage), "make-deep-%d", i);
            failStage = deepFailStage;
            ok = 0;
            break;
        }
    }

    char textPath[4096] = { 0 };
    char dataPath[4096] = { 0 };
    required = RLTextFormatTo(textPath, (int)sizeof(textPath), "%s%slong_path_smoke_text.txt", deepDir, sep);
    if (required >= (int)sizeof(textPath)) { ok = 0; failStage = "format-text-path"; }
    required = RLTextFormatTo(dataPath, (int)sizeof(dataPath), "%s%slong_path_smoke_data.bin", deepDir, sep);
    if (required >= (int)sizeof(dataPath)) { ok = 0; failStage = "format-data-path"; }

    if (ok)
    {
        const char *payloadText = "raylib-long-path-smoke";
        unsigned char payloadData[8] = { 0x42, 0x13, 0x24, 0x35, 0xaa, 0xbb, 0xcc, 0xdd };
        if (!RLSaveFileText(textPath, (char *)payloadText)) { ok = 0; failStage = "save-text"; }
        if (!RLSaveFileData(dataPath, payloadData, (int)sizeof(payloadData))) { ok = 0; failStage = "save-data"; }

        char *loadedText = RLLoadFileText(textPath);
        if ((loadedText == NULL) || (strcmp(loadedText, payloadText) != 0)) { ok = 0; failStage = "load-text"; }
        if (loadedText != NULL) RLUnloadFileText(loadedText);

        int readSize = 0;
        unsigned char *loadedData = RLLoadFileData(dataPath, &readSize);
        if ((loadedData == NULL) || (readSize != (int)sizeof(payloadData)) || (memcmp(loadedData, payloadData, sizeof(payloadData)) != 0)) { ok = 0; failStage = "load-data"; }
        if (loadedData != NULL) RLUnloadFileData(loadedData);
    }

    if ((uncRootOverride != NULL) && (uncRootOverride[0] != '\0'))
    {
        uncEnabled = 1;
        if (!RLDirectoryExists(uncRootOverride)) uncOk = 0;
        else
        {
            char uncDir[4096] = { 0 };
            char uncFile[4096] = { 0 };
            RLTextFormatTo(uncDir, (int)sizeof(uncDir), "%s%sraylib_unc_smoke", uncRootOverride, sep);
            if (RLMakeDirectory(uncDir) != 0) uncOk = 0;
            RLTextFormatTo(uncFile, (int)sizeof(uncFile), "%s%sunc_text.txt", uncDir, sep);
            if (!RLSaveFileText(uncFile, "unc-ok")) uncOk = 0;
            char *uncText = RLLoadFileText(uncFile);
            if ((uncText == NULL) || (strcmp(uncText, "unc-ok") != 0)) uncOk = 0;
            if (uncText != NULL) RLUnloadFileText(uncText);
        }
    }

    printf("{\"root\":\"%s\",\"rootLen\":%d,\"longPathOk\":%s,\"failStage\":\"%s\",\"uncEnabled\":%s,\"uncOk\":%s}\n",
           root, (int)RLTextLength(textPath),
           ok ? "true" : "false", failStage,
           uncEnabled ? "true" : "false",
           uncOk ? "true" : "false");

    return (ok && uncOk) ? 0 : 2;
}

static void RunApiSmokeOnce(void)
{
    if (gApiSmoke.done) return;
    gApiSmoke.done = 1;

    char fmtBuf[128] = { 0 };
    int required = RLTextFormatTo(fmtBuf, (int)sizeof(fmtBuf), "diag-smoke-%d", RLGetRandomValue(1000, 9999));
    gApiSmoke.textFormatOk = (required > 0);

    const char *appDir = RLGetApplicationDirectory();
    const char *workDir = RLGetWorkingDirectory();
    gApiSmoke.appDirLen = (int)RLTextLength(appDir);
    gApiSmoke.workDirLen = (int)RLTextLength(workDir);

    char textPath[4096] = { 0 };
    char dataPath[4096] = { 0 };
    RLTextFormatTo(textPath, (int)sizeof(textPath), "%s%s", appDir, "event_diag_text_smoke.txt");
    RLTextFormatTo(dataPath, (int)sizeof(dataPath), "%s%s", appDir, "event_diag_data_smoke.bin");

    gApiSmoke.textPathLen = (int)RLTextLength(textPath);
    gApiSmoke.dataPathLen = (int)RLTextLength(dataPath);
    RLTextFormatTo(gApiSmoke.textPath, (int)sizeof(gApiSmoke.textPath), "%s", textPath);
    RLTextFormatTo(gApiSmoke.dataPath, (int)sizeof(gApiSmoke.dataPath), "%s", dataPath);

    gApiSmoke.saveTextOk = RLSaveFileText(textPath, fmtBuf)? 1 : 0;
    char *loadedText = RLLoadFileText(textPath);
    if (loadedText != NULL)
    {
        gApiSmoke.loadTextOk = (strcmp(loadedText, fmtBuf) == 0)? 1 : 0;
        RLUnloadFileText(loadedText);
    }

    unsigned char payload[8] = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 };
    gApiSmoke.saveDataOk = RLSaveFileData(dataPath, payload, (int)sizeof(payload))? 1 : 0;
    int readSize = 0;
    unsigned char *readData = RLLoadFileData(dataPath, &readSize);
    if ((readData != NULL) && (readSize == (int)sizeof(payload)) && (memcmp(readData, payload, sizeof(payload)) == 0))
    {
        gApiSmoke.loadDataOk = 1;
    }
    if (readData != NULL) RLUnloadFileData(readData);

    gApiSmoke.fileExistsOk = RLFileExists(textPath) ? 1 : 0;
    gApiSmoke.dirExistsOk = RLDirectoryExists(appDir) ? 1 : 0;
    gApiSmoke.changeDirOk = RLChangeDirectory(workDir) ? 1 : 0;
}

#if defined(_WIN32)
#define QUEUE_SELFTEST_FRAME_CALLBACK_NORMAL_CAPACITY 1024u
#define QUEUE_SELFTEST_FRAME_CALLBACK_CRITICAL_CAPACITY 256u
#define SEMAPHORE_SELFTEST_WAITER_COUNT 4u

typedef struct QueueSaturationNativeState
{
    void *windowHandle;
    HANDLE blockerEnteredEvent;
    HANDLE blockerReleaseEvent;
    HANDLE workerDoneEvent;
    volatile LONG acceptedCount;
    volatile LONG failedCount;
    volatile LONG executedCount;
    volatile LONG blockerStarted;
    volatile LONG blockerReleased;
} QueueSaturationNativeState;

typedef struct QueueSaturationFrameState
{
    void *windowHandle;
    HANDLE workerDoneEvent;
    volatile LONG acceptedNormalCount;
    volatile LONG failedNormalCount;
    volatile LONG executedNormalCount;
    volatile LONG acceptedCriticalCount;
    volatile LONG failedCriticalCount;
    volatile LONG executedCriticalCount;
} QueueSaturationFrameState;

typedef struct FrameCallbackCloseWaiterState
{
    void *windowHandle;
    volatile LONG startedCount;
    volatile LONG acceptedCount;
    volatile LONG failedCount;
} FrameCallbackCloseWaiterState;

typedef struct SemaphoreSelfTestState
{
    volatile LONG waitingCount;
    volatile LONG successCount;
    volatile LONG failureCount;
} SemaphoreSelfTestState;

static intptr_t QueueSaturationNativeBlockingCallback(void *windowHandle, void *user)
{
    (void)windowHandle;
    QueueSaturationNativeState *state = (QueueSaturationNativeState *)user;
    if (state == NULL) return 0;

    InterlockedExchange(&state->blockerStarted, 1);
    if (state->blockerEnteredEvent != NULL) SetEvent(state->blockerEnteredEvent);
    if (state->blockerReleaseEvent != NULL) WaitForSingleObject(state->blockerReleaseEvent, 5000);
    InterlockedExchange(&state->blockerReleased, 1);
    return 1;
}

static intptr_t QueueSaturationNativeCountCallback(void *windowHandle, void *user)
{
    (void)windowHandle;
    QueueSaturationNativeState *state = (QueueSaturationNativeState *)user;
    if (state != NULL) InterlockedIncrement(&state->executedCount);
    return 1;
}

static unsigned __stdcall QueueSaturationNativeWorkerThread(void *arg)
{
    QueueSaturationNativeState *state = (QueueSaturationNativeState *)arg;
    if (state == NULL) return 0;

    if (RLInvokeOnWindowRenderThreadByHandle(state->windowHandle, QueueSaturationNativeBlockingCallback, state, 0) == 0)
    {
        InterlockedIncrement(&state->failedCount);
        if (state->workerDoneEvent != NULL) SetEvent(state->workerDoneEvent);
        return 0;
    }

    if (state->blockerEnteredEvent != NULL) WaitForSingleObject(state->blockerEnteredEvent, 5000);

    for (int i = 0; i < 5000; i++)
    {
        if (RLInvokeOnWindowRenderThreadByHandle(state->windowHandle, QueueSaturationNativeCountCallback, state, 0) != 0)
        {
            InterlockedIncrement(&state->acceptedCount);
        }
        else
        {
            InterlockedIncrement(&state->failedCount);
        }
    }

    if (state->blockerReleaseEvent != NULL) SetEvent(state->blockerReleaseEvent);
    if (state->workerDoneEvent != NULL) SetEvent(state->workerDoneEvent);
    return 0;
}

static intptr_t QueueSaturationFrameNormalCallback(void *windowHandle, void *user)
{
    (void)windowHandle;
    QueueSaturationFrameState *state = (QueueSaturationFrameState *)user;
    if (state != NULL) InterlockedIncrement(&state->executedNormalCount);
    return 1;
}

static intptr_t QueueSaturationFrameCriticalCallback(void *windowHandle, void *user)
{
    (void)windowHandle;
    QueueSaturationFrameState *state = (QueueSaturationFrameState *)user;
    if (state != NULL) InterlockedIncrement(&state->executedCriticalCount);
    return 1;
}

static unsigned __stdcall QueueSaturationFrameWorkerThread(void *arg)
{
    QueueSaturationFrameState *state = (QueueSaturationFrameState *)arg;
    if (state == NULL) return 0;

    for (int i = 0; i < 1100; i++)
    {
        if (RLPostWindowFrameCallbackByHandleEx(state->windowHandle,
                                                QueueSaturationFrameNormalCallback,
                                                state,
                                                RL_FRAME_CALLBACK_KIND_NORMAL) != 0)
        {
            InterlockedIncrement(&state->acceptedNormalCount);
        }
        else
        {
            InterlockedIncrement(&state->failedNormalCount);
        }
    }

    for (int i = 0; i < 280; i++)
    {
        if (RLPostWindowFrameCallbackByHandleEx(state->windowHandle,
                                                QueueSaturationFrameCriticalCallback,
                                                state,
                                                RL_FRAME_CALLBACK_KIND_CRITICAL) != 0)
        {
            InterlockedIncrement(&state->acceptedCriticalCount);
        }
        else
        {
            InterlockedIncrement(&state->failedCriticalCount);
        }
    }

    if (state->workerDoneEvent != NULL) SetEvent(state->workerDoneEvent);
    return 0;
}

static intptr_t FrameCallbackCloseNoopCallback(void *windowHandle, void *user)
{
    (void)windowHandle;
    (void)user;
    return 1;
}

typedef struct FrameCallbackCloseWaiterThreadArg
{
    FrameCallbackCloseWaiterState *state;
    RLFrameCallbackKind callbackKind;
} FrameCallbackCloseWaiterThreadArg;

typedef struct SemaphoreSelfTestThreadArg
{
    SemaphoreSelfTestState *state;
    RLSemaphore *semaphoreHandle;
} SemaphoreSelfTestThreadArg;

static unsigned __stdcall FrameCallbackCloseWaiterThread(void *arg)
{
    FrameCallbackCloseWaiterThreadArg *threadArg = (FrameCallbackCloseWaiterThreadArg *)arg;
    if ((threadArg == NULL) || (threadArg->state == NULL))
    {
        if (threadArg != NULL) RL_FREE(threadArg);
        return 0;
    }

    FrameCallbackCloseWaiterState *state = threadArg->state;
    RLFrameCallbackKind callbackKind = threadArg->callbackKind;
    RL_FREE(threadArg);

    InterlockedIncrement(&state->startedCount);

    if (RLPostWindowFrameCallbackByHandleEx(state->windowHandle,
                                            FrameCallbackCloseNoopCallback,
                                            NULL,
                                            callbackKind) != 0)
    {
        InterlockedIncrement(&state->acceptedCount);
    }
    else
    {
        InterlockedIncrement(&state->failedCount);
    }

    return 0;
}

static unsigned __stdcall SemaphoreSelfTestThread(void *arg)
{
    SemaphoreSelfTestThreadArg *threadArg = (SemaphoreSelfTestThreadArg *)arg;
    if ((threadArg == NULL) || (threadArg->state == NULL) || (threadArg->semaphoreHandle == NULL))
    {
        if (threadArg != NULL)
        {
            if (threadArg->semaphoreHandle != NULL) RLSemaphoreRelease(threadArg->semaphoreHandle);
            RL_FREE(threadArg);
        }
        return 0;
    }

    SemaphoreSelfTestState *state = threadArg->state;
    RLSemaphore *semaphoreHandle = threadArg->semaphoreHandle;
    RL_FREE(threadArg);

    InterlockedIncrement(&state->waitingCount);
    if (RLSemaphoreWaitTimeout(semaphoreHandle, 5000u))
    {
        InterlockedIncrement(&state->successCount);
    }
    else
    {
        InterlockedIncrement(&state->failureCount);
    }

    RLSemaphoreRelease(semaphoreHandle);
    return 0;
}

static int WaitForLongValue(volatile LONG *value, LONG expectedValue, DWORD timeoutMs)
{
    const DWORD startTick = GetTickCount();
    while ((GetTickCount() - startTick) < timeoutMs)
    {
        if (InterlockedCompareExchange((volatile LONG *)value, 0, 0) >= expectedValue) return 1;
        Sleep(1);
    }

    return (InterlockedCompareExchange((volatile LONG *)value, 0, 0) >= expectedValue) ? 1 : 0;
}

static int RunSemaphoreSelfTest(void)
{
    SemaphoreSelfTestState state = { 0 };
    HANDLE waiterHandles[SEMAPHORE_SELFTEST_WAITER_COUNT];
    RLSemaphore *semaphoreHandle = NULL;
    int result = 1;

    memset(waiterHandles, 0, sizeof(waiterHandles));

    semaphoreHandle = RLSemaphoreCreate(0u, SEMAPHORE_SELFTEST_WAITER_COUNT);
    if (semaphoreHandle == NULL)
    {
        RLTraceLog(RL_E_LOG_WARNING, "semaphore-selftest: failed to create semaphore");
        return 1;
    }

    for (unsigned int waiterIndex = 0u; waiterIndex < SEMAPHORE_SELFTEST_WAITER_COUNT; waiterIndex++)
    {
        SemaphoreSelfTestThreadArg *threadArg =
            (SemaphoreSelfTestThreadArg *)RL_MALLOC(sizeof(SemaphoreSelfTestThreadArg));
        if (threadArg == NULL)
        {
            RLTraceLog(RL_E_LOG_WARNING, "semaphore-selftest: failed to allocate thread arg");
            goto cleanup;
        }

        threadArg->state = &state;
        threadArg->semaphoreHandle = RLSemaphoreRetain(semaphoreHandle);
        if (threadArg->semaphoreHandle == NULL)
        {
            RL_FREE(threadArg);
            RLTraceLog(RL_E_LOG_WARNING, "semaphore-selftest: failed to retain semaphore for waiter");
            goto cleanup;
        }

        {
            uintptr_t threadHandleValue = _beginthreadex(NULL, 0, SemaphoreSelfTestThread, threadArg, 0, NULL);
            if (threadHandleValue == 0u)
            {
                RLSemaphoreRelease(threadArg->semaphoreHandle);
                RL_FREE(threadArg);
                RLTraceLog(RL_E_LOG_WARNING, "semaphore-selftest: failed to start waiter thread");
                goto cleanup;
            }
            waiterHandles[waiterIndex] = (HANDLE)threadHandleValue;
        }
    }

    if (!WaitForLongValue(&state.waitingCount, (LONG)SEMAPHORE_SELFTEST_WAITER_COUNT, 1000u))
    {
        RLTraceLog(RL_E_LOG_WARNING, "semaphore-selftest: waiters did not enter wait state in time");
        goto cleanup;
    }

    Sleep(50);
    RLSemaphoreReleaseOne(semaphoreHandle);
    if (!WaitForLongValue(&state.successCount, 1, 1000u))
    {
        RLTraceLog(RL_E_LOG_WARNING, "semaphore-selftest: first ReleaseOne did not wake exactly one waiter in time");
        goto cleanup;
    }
    if (InterlockedCompareExchange(&state.failureCount, 0, 0) != 0)
    {
        RLTraceLog(RL_E_LOG_WARNING, "semaphore-selftest: unexpected waiter failure before close");
        goto cleanup;
    }

    RLSemaphoreReleaseOne(semaphoreHandle);
    if (!WaitForLongValue(&state.successCount, 2, 1000u))
    {
        RLTraceLog(RL_E_LOG_WARNING, "semaphore-selftest: second ReleaseOne did not wake a second waiter in time");
        goto cleanup;
    }
    if (InterlockedCompareExchange(&state.failureCount, 0, 0) != 0)
    {
        RLTraceLog(RL_E_LOG_WARNING, "semaphore-selftest: unexpected waiter failure before close");
        goto cleanup;
    }

    RLSemaphoreClose(semaphoreHandle);

    for (unsigned int waiterIndex = 0u; waiterIndex < SEMAPHORE_SELFTEST_WAITER_COUNT; waiterIndex++)
    {
        if (waiterHandles[waiterIndex] == NULL) continue;
        WaitForSingleObject(waiterHandles[waiterIndex], 8000u);
        CloseHandle(waiterHandles[waiterIndex]);
        waiterHandles[waiterIndex] = NULL;
    }

    RLTraceLog(RL_E_LOG_INFO,
               "semaphore-selftest: waiting=%ld success=%ld failure=%ld",
               InterlockedCompareExchange(&state.waitingCount, 0, 0),
               InterlockedCompareExchange(&state.successCount, 0, 0),
               InterlockedCompareExchange(&state.failureCount, 0, 0));

    if ((InterlockedCompareExchange(&state.successCount, 0, 0) != 2) ||
        (InterlockedCompareExchange(&state.failureCount, 0, 0) != 2))
    {
        RLTraceLog(RL_E_LOG_WARNING, "semaphore-selftest: failed invariants");
        goto cleanup;
    }

    RLTraceLog(RL_E_LOG_INFO, "semaphore-selftest: PASSED");
    result = 0;

cleanup:
    if (semaphoreHandle != NULL)
    {
        RLSemaphoreClose(semaphoreHandle);
        for (unsigned int waiterIndex = 0u; waiterIndex < SEMAPHORE_SELFTEST_WAITER_COUNT; waiterIndex++)
        {
            if (waiterHandles[waiterIndex] == NULL) continue;
            WaitForSingleObject(waiterHandles[waiterIndex], 8000u);
            CloseHandle(waiterHandles[waiterIndex]);
            waiterHandles[waiterIndex] = NULL;
        }
        RLSemaphoreRelease(semaphoreHandle);
    }

    return result;
}

static int PumpFramesUntilCondition(double timeoutSeconds, bool (*conditionFn)(void *user), void *user)
{
    const double startTime = RLGetTime();

    while ((RLGetTime() - startTime) < timeoutSeconds)
    {
        if ((conditionFn != NULL) && conditionFn(user)) return 1;
        RLBeginDrawing();
        RLClearBackground(BLACK);
        RLEndDrawing();
    }

    return ((conditionFn != NULL) && conditionFn(user)) ? 1 : 0;
}

static bool QueueSaturationNativeDone(void *user)
{
    QueueSaturationNativeState *state = (QueueSaturationNativeState *)user;
    if (state == NULL) return true;

    return (WaitForSingleObject(state->workerDoneEvent, 0) == WAIT_OBJECT_0) &&
           (InterlockedCompareExchange(&state->executedCount, 0, 0) >= InterlockedCompareExchange(&state->acceptedCount, 0, 0)) &&
           (InterlockedCompareExchange(&state->blockerReleased, 0, 0) != 0);
}

static bool QueueSaturationFrameDone(void *user)
{
    QueueSaturationFrameState *state = (QueueSaturationFrameState *)user;
    if (state == NULL) return true;

    return (WaitForSingleObject(state->workerDoneEvent, 0) == WAIT_OBJECT_0) &&
           (InterlockedCompareExchange(&state->executedNormalCount, 0, 0) >= InterlockedCompareExchange(&state->acceptedNormalCount, 0, 0)) &&
           (InterlockedCompareExchange(&state->executedCriticalCount, 0, 0) >= InterlockedCompareExchange(&state->acceptedCriticalCount, 0, 0));
}

static RLEventThreadDiagStats SubtractEventThreadDiagStats(RLEventThreadDiagStats afterStats, RLEventThreadDiagStats beforeStats)
{
    RLEventThreadDiagStats deltaStats = afterStats;

    deltaStats.nativeTaskQueueDroppedCount -= beforeStats.nativeTaskQueueDroppedCount;
    deltaStats.nativeTaskQueueDroppedInputCount -= beforeStats.nativeTaskQueueDroppedInputCount;
    deltaStats.nativeTaskQueueDroppedStateCount -= beforeStats.nativeTaskQueueDroppedStateCount;
    deltaStats.nativeTaskQueueDroppedCriticalCount -= beforeStats.nativeTaskQueueDroppedCriticalCount;
    deltaStats.nativeTaskQueueDroppedMaintenanceCount -= beforeStats.nativeTaskQueueDroppedMaintenanceCount;
    deltaStats.frameCallbackDroppedCount -= beforeStats.frameCallbackDroppedCount;
    deltaStats.frameCallbackDroppedNormalCount -= beforeStats.frameCallbackDroppedNormalCount;
    deltaStats.frameCallbackDroppedCriticalCount -= beforeStats.frameCallbackDroppedCriticalCount;
    deltaStats.frameCallbackEvictedNormalForCriticalCount -= beforeStats.frameCallbackEvictedNormalForCriticalCount;

    return deltaStats;
}

static int RunQueueSaturationSelfTest(void)
{
    QueueSaturationNativeState nativeState = { 0 };
    QueueSaturationFrameState frameState = { 0 };
    FrameCallbackCloseWaiterState closeWaiterState = { 0 };
    int result = 1;
    HANDLE nativeWorkerHandle = NULL;
    HANDLE frameWorkerHandle = NULL;
    HANDLE closeWaiterHandles[8] = { 0 };
    RLEventThreadDiagStats nativeStatsBefore = { 0 };
    RLEventThreadDiagStats frameStatsBefore = { 0 };

    RLSetConfigFlags(RL_E_FLAG_WINDOW_EVENT_THREAD | RL_E_FLAG_WINDOW_RESIZABLE);
    RLInitWindow(640, 360, "raylib [queue-saturation] selftest");
    if (!RLIsWindowReady())
    {
        RLTraceLog(RL_E_LOG_WARNING, "queue-selftest: failed to initialize window");
        return 1;
    }

    RLSetTargetFPS(120);
    RLEnableEventDiagStats();
    RLResetEventThreadDiagStatsForCurrentContext();
    nativeStatsBefore = RLGetEventThreadDiagStats();

    nativeState.windowHandle = RLGetWindowHandle();
    nativeState.blockerEnteredEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    nativeState.blockerReleaseEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    nativeState.workerDoneEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

    if ((nativeState.blockerEnteredEvent == NULL) || (nativeState.blockerReleaseEvent == NULL) || (nativeState.workerDoneEvent == NULL))
    {
        RLTraceLog(RL_E_LOG_WARNING, "queue-selftest: failed to create native-task events");
        goto cleanup;
    }

    {
        uintptr_t nativeWorkerHandleValue = _beginthreadex(NULL, 0, QueueSaturationNativeWorkerThread, &nativeState, 0, NULL);
        if (nativeWorkerHandleValue == 0)
        {
            RLTraceLog(RL_E_LOG_WARNING, "queue-selftest: failed to start native-task worker");
            goto cleanup;
        }
        nativeWorkerHandle = (HANDLE)nativeWorkerHandleValue;
    }

    if (!PumpFramesUntilCondition(8.0, QueueSaturationNativeDone, &nativeState))
    {
        RLTraceLog(RL_E_LOG_WARNING, "queue-selftest: native-task phase timed out");
        goto cleanup;
    }

    WaitForSingleObject(nativeWorkerHandle, 8000);
    CloseHandle(nativeWorkerHandle);
    nativeWorkerHandle = NULL;

    {
        RLEventThreadDiagStats nativeStats = SubtractEventThreadDiagStats(RLGetEventThreadDiagStats(), nativeStatsBefore);
        const long accepted = InterlockedCompareExchange(&nativeState.acceptedCount, 0, 0);
        const long failed = InterlockedCompareExchange(&nativeState.failedCount, 0, 0);
        const long executed = InterlockedCompareExchange(&nativeState.executedCount, 0, 0);
        const unsigned long long dropped = nativeStats.nativeTaskQueueDroppedCount;
        const unsigned long long droppedNoise = (dropped > (unsigned long long)failed) ? (dropped - (unsigned long long)failed) : 0u;

        RLTraceLog(RL_E_LOG_INFO,
                   "queue-selftest: native-task accepted=%ld failed=%ld executed=%ld dropped=%llu droppedNoise=%llu tasksPostFailed=%llu",
                   accepted, failed, executed, dropped, droppedNoise, nativeStats.tasksPostFailed);

        if ((accepted <= 0) || (failed <= 0) || (executed != accepted) || (dropped < (unsigned long long)failed) || (droppedNoise > 4u))
        {
            RLTraceLog(RL_E_LOG_WARNING, "queue-selftest: native-task phase failed invariants");
            goto cleanup;
        }
    }

    RLResetEventThreadDiagStatsForCurrentContext();
    frameStatsBefore = RLGetEventThreadDiagStats();

    frameState.windowHandle = RLGetWindowHandle();
    frameState.workerDoneEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (frameState.workerDoneEvent == NULL)
    {
        RLTraceLog(RL_E_LOG_WARNING, "queue-selftest: failed to create frame-callback worker event");
        goto cleanup;
    }

    {
        uintptr_t frameWorkerHandleValue = _beginthreadex(NULL, 0, QueueSaturationFrameWorkerThread, &frameState, 0, NULL);
        if (frameWorkerHandleValue == 0)
        {
            RLTraceLog(RL_E_LOG_WARNING, "queue-selftest: failed to start frame-callback worker");
            goto cleanup;
        }
        frameWorkerHandle = (HANDLE)frameWorkerHandleValue;
    }

    Sleep(200);

    if (!PumpFramesUntilCondition(8.0, QueueSaturationFrameDone, &frameState))
    {
        RLEventThreadDiagStats frameStatsOnTimeout = SubtractEventThreadDiagStats(RLGetEventThreadDiagStats(), frameStatsBefore);
        RLTraceLog(RL_E_LOG_WARNING,
                   "queue-selftest: frame-callback timeout stats normal accepted=%ld failed=%ld executed=%ld | critical accepted=%ld failed=%ld executed=%ld | dropped=%llu droppedNormal=%llu droppedCritical=%llu evicted=%llu",
                   InterlockedCompareExchange(&frameState.acceptedNormalCount, 0, 0),
                   InterlockedCompareExchange(&frameState.failedNormalCount, 0, 0),
                   InterlockedCompareExchange(&frameState.executedNormalCount, 0, 0),
                   InterlockedCompareExchange(&frameState.acceptedCriticalCount, 0, 0),
                   InterlockedCompareExchange(&frameState.failedCriticalCount, 0, 0),
                   InterlockedCompareExchange(&frameState.executedCriticalCount, 0, 0),
                   frameStatsOnTimeout.frameCallbackDroppedCount,
                   frameStatsOnTimeout.frameCallbackDroppedNormalCount,
                   frameStatsOnTimeout.frameCallbackDroppedCriticalCount,
                   frameStatsOnTimeout.frameCallbackEvictedNormalForCriticalCount);
        RLTraceLog(RL_E_LOG_WARNING, "queue-selftest: frame-callback phase timed out");
        goto cleanup;
    }

    WaitForSingleObject(frameWorkerHandle, 8000);
    CloseHandle(frameWorkerHandle);
    frameWorkerHandle = NULL;

    {
        RLEventThreadDiagStats frameStats = SubtractEventThreadDiagStats(RLGetEventThreadDiagStats(), frameStatsBefore);
        const long acceptedNormal = InterlockedCompareExchange(&frameState.acceptedNormalCount, 0, 0);
        const long failedNormal = InterlockedCompareExchange(&frameState.failedNormalCount, 0, 0);
        const long executedNormal = InterlockedCompareExchange(&frameState.executedNormalCount, 0, 0);
        const long acceptedCritical = InterlockedCompareExchange(&frameState.acceptedCriticalCount, 0, 0);
        const long failedCritical = InterlockedCompareExchange(&frameState.failedCriticalCount, 0, 0);
        const long executedCritical = InterlockedCompareExchange(&frameState.executedCriticalCount, 0, 0);

        RLTraceLog(RL_E_LOG_INFO,
                   "queue-selftest: frame-callback normal accepted=%ld failed=%ld executed=%ld | critical accepted=%ld failed=%ld executed=%ld | dropped=%llu droppedNormal=%llu droppedCritical=%llu evicted=%llu",
                   acceptedNormal,
                   failedNormal,
                   executedNormal,
                   acceptedCritical,
                   failedCritical,
                   executedCritical,
                   frameStats.frameCallbackDroppedCount,
                   frameStats.frameCallbackDroppedNormalCount,
                   frameStats.frameCallbackDroppedCriticalCount,
                   frameStats.frameCallbackEvictedNormalForCriticalCount);

        if ((acceptedNormal <= 0) ||
            (acceptedCritical <= 0) ||
            ((failedNormal + failedCritical) <= 0) ||
            (executedNormal != acceptedNormal) ||
            (executedCritical != acceptedCritical) ||
            (frameStats.frameCallbackDroppedNormalCount != (unsigned long long)failedNormal) ||
            (frameStats.frameCallbackDroppedCriticalCount != (unsigned long long)failedCritical) ||
            (frameStats.frameCallbackDroppedCount != (unsigned long long)(failedNormal + failedCritical)) ||
            (frameStats.frameCallbackEvictedNormalForCriticalCount != 0))
        {
            RLTraceLog(RL_E_LOG_WARNING, "queue-selftest: frame-callback phase failed invariants");
            goto cleanup;
        }
    }

    {
        unsigned int queuedCount = 0u;
        unsigned int queuedCriticalCount = 0u;
        const unsigned int closeWaiterCount = (unsigned int)(sizeof(closeWaiterHandles)/sizeof(closeWaiterHandles[0]));

        for (unsigned int callbackIndex = 0u; callbackIndex < QUEUE_SELFTEST_FRAME_CALLBACK_NORMAL_CAPACITY; callbackIndex++)
        {
            if (RLPostWindowFrameCallbackByHandleEx(RLGetWindowHandle(),
                                                    FrameCallbackCloseNoopCallback,
                                                    NULL,
                                                    RL_FRAME_CALLBACK_KIND_NORMAL) == 0)
            {
                RLTraceLog(RL_E_LOG_WARNING, "queue-selftest: failed to prefill normal frame-callback queue");
                goto cleanup;
            }
        }

        for (unsigned int callbackIndex = 0u; callbackIndex < QUEUE_SELFTEST_FRAME_CALLBACK_CRITICAL_CAPACITY; callbackIndex++)
        {
            if (RLPostWindowFrameCallbackByHandleEx(RLGetWindowHandle(),
                                                    FrameCallbackCloseNoopCallback,
                                                    NULL,
                                                    RL_FRAME_CALLBACK_KIND_CRITICAL) == 0)
            {
                RLTraceLog(RL_E_LOG_WARNING, "queue-selftest: failed to prefill critical frame-callback queue");
                goto cleanup;
            }
        }

        if (!RLGetCurrentContextFrameCallbackQueueStats(&queuedCount, &queuedCriticalCount, NULL, NULL, NULL, NULL, NULL))
        {
            RLTraceLog(RL_E_LOG_WARNING, "queue-selftest: failed to query frame-callback queue stats before close-waiter phase");
            goto cleanup;
        }

        if ((queuedCount != (QUEUE_SELFTEST_FRAME_CALLBACK_NORMAL_CAPACITY + QUEUE_SELFTEST_FRAME_CALLBACK_CRITICAL_CAPACITY)) ||
            (queuedCriticalCount != QUEUE_SELFTEST_FRAME_CALLBACK_CRITICAL_CAPACITY))
        {
            RLTraceLog(RL_E_LOG_WARNING,
                       "queue-selftest: unexpected prefill queue counts total=%u critical=%u",
                       queuedCount,
                       queuedCriticalCount);
            goto cleanup;
        }

        closeWaiterState.windowHandle = RLGetWindowHandle();

        for (unsigned int waiterIndex = 0u; waiterIndex < closeWaiterCount; waiterIndex++)
        {
            FrameCallbackCloseWaiterThreadArg *threadArg =
                (FrameCallbackCloseWaiterThreadArg *)RL_MALLOC(sizeof(FrameCallbackCloseWaiterThreadArg));
            if (threadArg == NULL)
            {
                RLTraceLog(RL_E_LOG_WARNING, "queue-selftest: failed to allocate close-waiter thread arg");
                goto cleanup;
            }

            threadArg->state = &closeWaiterState;
            threadArg->callbackKind = (waiterIndex < (closeWaiterCount/2u))
                                      ? RL_FRAME_CALLBACK_KIND_NORMAL
                                      : RL_FRAME_CALLBACK_KIND_CRITICAL;

            {
                uintptr_t waiterHandleValue = _beginthreadex(NULL, 0, FrameCallbackCloseWaiterThread, threadArg, 0, NULL);
                if (waiterHandleValue == 0u)
                {
                    RL_FREE(threadArg);
                    RLTraceLog(RL_E_LOG_WARNING, "queue-selftest: failed to start close-waiter thread");
                    goto cleanup;
                }
                closeWaiterHandles[waiterIndex] = (HANDLE)waiterHandleValue;
            }
        }

        {
            const double waiterStartTime = RLGetTime();
            while ((RLGetTime() - waiterStartTime) < 1.0)
            {
                if (InterlockedCompareExchange(&closeWaiterState.startedCount, 0, 0) >= (LONG)closeWaiterCount) break;
                Sleep(1);
            }
        }

        Sleep(50);
        RLCloseWindow();

        for (unsigned int waiterIndex = 0u; waiterIndex < closeWaiterCount; waiterIndex++)
        {
            if (closeWaiterHandles[waiterIndex] == NULL) continue;
            WaitForSingleObject(closeWaiterHandles[waiterIndex], 8000);
            CloseHandle(closeWaiterHandles[waiterIndex]);
            closeWaiterHandles[waiterIndex] = NULL;
        }

        {
            const long started = InterlockedCompareExchange(&closeWaiterState.startedCount, 0, 0);
            const long accepted = InterlockedCompareExchange(&closeWaiterState.acceptedCount, 0, 0);
            const long failed = InterlockedCompareExchange(&closeWaiterState.failedCount, 0, 0);

            RLTraceLog(RL_E_LOG_INFO,
                       "queue-selftest: frame-callback close-waiters started=%ld accepted=%ld failed=%ld",
                       started,
                       accepted,
                       failed);

            if ((started != (long)closeWaiterCount) ||
                (accepted != 0) ||
                (failed != (long)closeWaiterCount))
            {
                RLTraceLog(RL_E_LOG_WARNING, "queue-selftest: frame-callback close-waiter phase failed invariants");
                goto cleanup;
            }
        }
    }

    RLTraceLog(RL_E_LOG_INFO, "queue-selftest: PASSED");
    result = 0;

cleanup:
    if (nativeState.blockerReleaseEvent != NULL) SetEvent(nativeState.blockerReleaseEvent);
    if (nativeWorkerHandle != NULL)
    {
        WaitForSingleObject(nativeWorkerHandle, 8000);
        CloseHandle(nativeWorkerHandle);
    }
    if (frameWorkerHandle != NULL)
    {
        WaitForSingleObject(frameWorkerHandle, 8000);
        CloseHandle(frameWorkerHandle);
    }
    for (unsigned int waiterIndex = 0u; waiterIndex < (unsigned int)(sizeof(closeWaiterHandles)/sizeof(closeWaiterHandles[0])); waiterIndex++)
    {
        if (closeWaiterHandles[waiterIndex] == NULL) continue;
        WaitForSingleObject(closeWaiterHandles[waiterIndex], 8000);
        CloseHandle(closeWaiterHandles[waiterIndex]);
    }
    if (nativeState.blockerEnteredEvent != NULL) CloseHandle(nativeState.blockerEnteredEvent);
    if (nativeState.blockerReleaseEvent != NULL) CloseHandle(nativeState.blockerReleaseEvent);
    if (nativeState.workerDoneEvent != NULL) CloseHandle(nativeState.workerDoneEvent);
    if (frameState.workerDoneEvent != NULL) CloseHandle(frameState.workerDoneEvent);
    if (RLIsWindowReady()) RLCloseWindow();
    return result;
}
#endif

static void AddMarker(RLVector2 p, float r)
{
    if (gMarkerCount >= MAX_MARKERS) return;

    gMarkers[gMarkerCount].pos = p;
    gMarkers[gMarkerCount].r = r;
    gMarkers[gMarkerCount].col = (RLColor){
        (unsigned char)RLGetRandomValue(80, 250),
        (unsigned char)RLGetRandomValue(80, 250),
        (unsigned char)RLGetRandomValue(80, 250),
        220
    };

    gMarkerCount++;
}

static void DrawLastMarkerToCanvas(int canvasX, int canvasY)
{
    if (gCanvas.id == 0) return;
    if (gMarkerCount <= 0) return;

    const Marker *m = &gMarkers[gMarkerCount - 1];
    RLBeginTextureMode(gCanvas);
    // No clear: incremental draw.
    RLVector2 lp = (RLVector2){ m->pos.x - (float)canvasX, m->pos.y - (float)canvasY };
    RLDrawCircleV(lp, m->r, m->col);
    RLEndTextureMode();
}

static void PopMarker(void)
{
    if (gMarkerCount > 0) gMarkerCount--;
    gCanvasDirty = true;
}

static void ClearMarkers(void)
{
    gMarkerCount = 0;
    gCanvasDirty = true;
}

static void DrawOverlayHelp(int x, int y)
{
    int oy = y;

    RLDrawText("Win32 event-thread diagnostics (interactive)", x, oy, 18, WHITE); oy += 24;
    RLDrawText("LMB: add / paint | RMB: undo | MMB or C: clear | Wheel: size", x, oy, 16, RAYWHITE); oy += 20;
    RLDrawText("R: safe reset diag | T: toggle diag | G: telemetry | H: help", x, oy, 16, RAYWHITE); oy += 20;
    RLDrawText("V: toggle marker render mode (canvas/direct) | K: dump tracked objects", x, oy, 16, RAYWHITE); oy += 20;
    RLDrawText("1/2/3: switch diagnostics page", x, oy, 16, RAYWHITE); oy += 20;
    RLDrawText("Y: same-thread multi-window test  X: cross-thread shared test", x, oy, 16, RAYWHITE); oy += 20;
    RLDrawText("Telemetry panel: click sample-rate dropdown", x, oy, 16, RAYWHITE); oy += 20;

#if defined(RL_EVENTTHREAD_COALESCE_STATE)
    RLDrawText(RLTextFormat("RL_EVENTTHREAD_COALESCE_STATE=%d", (int)RL_EVENTTHREAD_COALESCE_STATE), x, oy, 16, RAYWHITE); oy += 20;
#endif

    (void)oy;
}

static int GetOverlayHelpHeight(void)
{
    int height = 24;   // title line
    height += 20;      // line 1
    height += 20;      // line 2
    height += 20;      // line 3
    height += 20;      // line 4
    height += 20;      // line 5
    height += 20;      // line 6
#if defined(RL_EVENTTHREAD_COALESCE_STATE)
    height += 20;      // coalesce-state line
#endif
    return height;
}

static void EnsureCanvas(int w, int h)
{
    if ((w <= 0) || (h <= 0)) return;
    if ((gCanvas.id != 0) && (gCanvasW == w) && (gCanvasH == h)) return;

    if (gCanvas.id != 0) RLUnloadRenderTexture(gCanvas);
    gCanvas = RLLoadRenderTexture(w, h);
    gCanvasW = w;
    gCanvasH = h;
    gCanvasDirty = true;
}

static void RebuildCanvas(int canvasX, int canvasY)
{
    if (gCanvas.id == 0) return;

    RLBeginTextureMode(gCanvas);
    RLClearBackground((RLColor){ 26, 26, 34, 255 });
    for (int i = 0; i < gMarkerCount; i++)
    {
        RLVector2 lp = (RLVector2){ gMarkers[i].pos.x - (float)canvasX, gMarkers[i].pos.y - (float)canvasY };
        RLDrawCircleV(lp, gMarkers[i].r, gMarkers[i].col);
    }
    RLEndTextureMode();
    gCanvasDirty = false;
}

static inline int TelemetryGetIndexByAge(int age)
{
    // age=0 returns oldest sample currently kept.
    return (gTelemetry.head - gTelemetry.count + age + TELEMETRY_SAMPLES) % TELEMETRY_SAMPLES;
}

static inline bool IsPointInsideRect(RLVector2 p, RLRectangle rect)
{
    return (p.x >= rect.x) && (p.x <= (rect.x + rect.width)) &&
           (p.y >= rect.y) && (p.y <= (rect.y + rect.height));
}

static inline int TelemetryGetRenderSegments(int sampleCount, int maxSegments)
{
    (void)maxSegments;
    if (sampleCount <= 1) return 0;
    int segments = sampleCount - 1;
    if (segments < 1) segments = 1;
    return segments;
}

static inline int TelemetryMapSegmentToAge(int segmentIndex, int segmentCount, int sampleCount)
{
    if (sampleCount <= 1) return 0;
    if (segmentCount <= 0) return 0;
    int age = (segmentIndex*(sampleCount - 1))/segmentCount;
    if (age < 0) age = 0;
    if (age > (sampleCount - 1)) age = sampleCount - 1;
    return age;
}

static void TelemetryResetDisplayScale(void)
{
    for (int i = 0; i < TELEMETRY_CHART_COUNT; i++) gChartDisplayMax[i] = 0.0f;
    gChartDisplayMaxReady = false;
    gChartScaleLastUpdateSec = 0.0;
}

static float TelemetryGetDisplayScale(int chartIndex, float targetMax, float floorMax, bool updateNow)
{
    if (chartIndex < 0) chartIndex = 0;
    if (chartIndex >= TELEMETRY_CHART_COUNT) chartIndex = TELEMETRY_CHART_COUNT - 1;
    if (targetMax < floorMax) targetMax = floorMax;

    if (!gChartDisplayMaxReady || (gChartDisplayMax[chartIndex] <= 0.0f))
    {
        gChartDisplayMax[chartIndex] = targetMax;
        return targetMax;
    }

    if (targetMax > gChartDisplayMax[chartIndex])
    {
        gChartDisplayMax[chartIndex] = targetMax;
        return gChartDisplayMax[chartIndex];
    }

    if (!updateNow) return gChartDisplayMax[chartIndex];

    float current = gChartDisplayMax[chartIndex];
    if (targetMax > current)
    {
        current = targetMax; // fast attack
    }
    else
    {
        const float release = 0.03f; // slow release to avoid high-frequency flicker
        current = (1.0f - release)*current + release*targetMax;
    }

    if (current < floorMax) current = floorMax;
    gChartDisplayMax[chartIndex] = current;
    return current;
}

static inline int ClampInt(int value, int minv, int maxv)
{
    if (value < minv) return minv;
    if (value > maxv) return maxv;
    return value;
}

static float TelemetrySampleMaxInAgeRange(const float *series, int ageStart, int ageEnd)
{
    if ((series == NULL) || (gTelemetry.count <= 0)) return 0.0f;
    if (ageStart < 0) ageStart = 0;
    if (ageEnd < ageStart) ageEnd = ageStart;
    if (ageEnd >= gTelemetry.count) ageEnd = gTelemetry.count - 1;

    float vmax = series[TelemetryGetIndexByAge(ageStart)];
    for (int age = ageStart + 1; age <= ageEnd; age++)
    {
        const float v = series[TelemetryGetIndexByAge(age)];
        if (v > vmax) vmax = v;
    }
    return vmax;
}

static float TelemetrySampleMinInAgeRange(const float *series, int ageStart, int ageEnd)
{
    if ((series == NULL) || (gTelemetry.count <= 0)) return 0.0f;
    if (ageStart < 0) ageStart = 0;
    if (ageEnd < ageStart) ageEnd = ageStart;
    if (ageEnd >= gTelemetry.count) ageEnd = gTelemetry.count - 1;

    float vmin = series[TelemetryGetIndexByAge(ageStart)];
    for (int age = ageStart + 1; age <= ageEnd; age++)
    {
        const float v = series[TelemetryGetIndexByAge(age)];
        if (v < vmin) vmin = v;
    }
    return vmin;
}

static float TelemetrySampleMeanInAgeRange(const float *series, int ageStart, int ageEnd)
{
    if ((series == NULL) || (gTelemetry.count <= 0)) return 0.0f;
    if (ageStart < 0) ageStart = 0;
    if (ageEnd < ageStart) ageEnd = ageStart;
    if (ageEnd >= gTelemetry.count) ageEnd = gTelemetry.count - 1;

    double sum = 0.0;
    int count = 0;
    for (int age = ageStart; age <= ageEnd; age++)
    {
        sum += series[TelemetryGetIndexByAge(age)];
        count++;
    }
    if (count <= 0) return 0.0f;
    return (float)(sum/(double)count);
}

static float TelemetrySampleForSegment(const float *series, int segmentIndex, int segmentCount, int sampleCount)
{
    if ((series == NULL) || (sampleCount <= 0)) return 0.0f;
    if (sampleCount == 1) return series[TelemetryGetIndexByAge(0)];

    const int downsampled = (segmentCount < (sampleCount - 1));
    const int age0 = TelemetryMapSegmentToAge(segmentIndex, segmentCount, sampleCount);
    const int age1 = TelemetryMapSegmentToAge(segmentIndex + 1, segmentCount, sampleCount);

    if (!downsampled || (age1 <= age0))
    {
        return series[TelemetryGetIndexByAge(age0)];
    }

    // Mean in bin for line trend; peaks are preserved by min-max band overlay.
    return TelemetrySampleMeanInAgeRange(series, age0, age1);
}

// Draw a compact dropdown in current panel style and process click interactions.
static bool DrawSampleRateCombo(int x, int y, int w, int h, int *active, bool *open)
{
    if ((active == NULL) || (open == NULL)) return false;
    if (*active < 0) *active = 0;
    if (*active >= TELEMETRY_SAMPLE_MODE_COUNT) *active = TELEMETRY_SAMPLE_MODE_COUNT - 1;

    bool changed = false;
    RLVector2 mouse = RLGetMousePosition();
    bool click = RLIsMouseButtonPressed(RL_E_MOUSE_BUTTON_LEFT);
    RLRectangle box = { (float)x, (float)y, (float)w, (float)h };

    if (click)
    {
        if (IsPointInsideRect(mouse, box))
        {
            *open = !(*open);
        }
        else if (*open)
        {
            bool itemHit = false;
            for (int i = 0; i < TELEMETRY_SAMPLE_MODE_COUNT; i++)
            {
                RLRectangle item = { (float)x, (float)(y + h + i*h), (float)w, (float)h };
                if (IsPointInsideRect(mouse, item))
                {
                    *active = i;
                    *open = false;
                    changed = true;
                    itemHit = true;
                    break;
                }
            }
            if (!itemHit) *open = false;
        }
    }

    RLColor boxBg = (RLColor){ 29, 35, 47, 240 };
    RLColor boxBd = (RLColor){ 86, 104, 132, 255 };
    RLColor txt = (RLColor){ 228, 236, 250, 255 };
    RLColor acc = (RLColor){ 113, 181, 255, 255 };
    RLColor hov = (RLColor){ 45, 56, 74, 250 };

    RLDrawRectangleRec(box, boxBg);
    RLDrawRectangleLinesEx(box, 1.0f, *open ? acc : boxBd);
    RLDrawText(gTelemetrySampleModes[*active].label, x + 10, y + 7, 16, txt);

    RLVector2 a = { (float)(x + w - 16), (float)(y + h/2 - 2) };
    RLVector2 b = { (float)(x + w - 8),  (float)(y + h/2 - 2) };
    RLVector2 c = { (float)(x + w - 12), (float)(y + h/2 + 4) };
    if (*open) { a.y += 4; b.y += 4; c.y -= 4; }
    RLDrawTriangle(a, b, c, acc);

    static float openAnim = 0.0f;
    if (*open) openAnim += 0.22f;
    else openAnim -= 0.22f;
    if (openAnim < 0.0f) openAnim = 0.0f;
    if (openAnim > 1.0f) openAnim = 1.0f;

    if (openAnim > 0.001f)
    {
        const int fullListH = h*TELEMETRY_SAMPLE_MODE_COUNT;
        const int shownListH = (int)((float)fullListH*openAnim);
        RLRectangle listRect = { (float)x, (float)(y + h), (float)w, (float)shownListH };
        RLDrawRectangleRec(listRect, (RLColor){ 21, 26, 36, (unsigned char)(210 + 36*openAnim) });
        RLDrawRectangleLinesEx(listRect, 1.0f, boxBd);
        for (int i = 0; i < TELEMETRY_SAMPLE_MODE_COUNT; i++)
        {
            const int itemY = y + h + i*h;
            if (itemY >= (y + h + shownListH)) break;
            RLRectangle item = { (float)x, (float)itemY, (float)w, (float)h };
            bool hovered = IsPointInsideRect(mouse, item);
            if ((i == *active) || hovered)
            {
                RLDrawRectangleRec(item, (i == *active)? hov : (RLColor){ 35, 43, 58, 230 });
            }
            RLDrawText(gTelemetrySampleModes[i].label, x + 10, y + h + i*h + 7, 16, txt);
        }
    }

    return changed;
}

static void TelemetryComputeRollingMeanStd(const float *series, int windowSize, float *outMean, float *outStd)
{
    if (outMean) *outMean = 0.0f;
    if (outStd) *outStd = 0.0f;
    if (gTelemetry.count <= 0) return;

    int sampleCount = gTelemetry.count;
    if (windowSize > 0 && sampleCount > windowSize) sampleCount = windowSize;
    if (sampleCount <= 0) return;

    const int startAge = gTelemetry.count - sampleCount;
    double sum = 0.0;
    for (int i = 0; i < sampleCount; i++)
    {
        const int idx = TelemetryGetIndexByAge(startAge + i);
        sum += (double)series[idx];
    }

    const double mean = sum/(double)sampleCount;
    double var = 0.0;
    for (int i = 0; i < sampleCount; i++)
    {
        const int idx = TelemetryGetIndexByAge(startAge + i);
        const double d = (double)series[idx] - mean;
        var += d*d;
    }
    var /= (double)sampleCount;

    if (outMean) *outMean = (float)mean;
    if (outStd) *outStd = (float)sqrt(var);
}

static void TelemetryPush(const RLEventThreadDiagStats *stats, float frameMs, bool focused)
{
    if (stats == NULL) return;
    const float frameCpuMs = (float)stats->frameCpuLastMs;
    const float swapCostMs = (float)stats->swapCostLastMs;
    float waitCostMs = (float)stats->waitCostLastMs;
    if (waitCostMs < 0.0f) waitCostMs = 0.0f;
    float residualMs = frameMs - frameCpuMs - swapCostMs - waitCostMs;
    if (residualMs < 0.0f) residualMs = 0.0f;

    // Noise gate for derived values: tiny residuals are mostly timer/scheduling jitter.
    if (waitCostMs < 0.20f) waitCostMs = 0.0f;
    if (residualMs < 0.20f) residualMs = 0.0f;

    // EMA smoothing for derived curves only.
    // Keep raw values in textual diagnostics while charts show trend-friendly curves.
    {
        const float alpha = 0.20f;
        if (!gDerivedEmaReady)
        {
            gWaitCostEmaMs = waitCostMs;
            gResidualEmaMs = residualMs;
            gDerivedEmaReady = true;
        }
        else
        {
            gWaitCostEmaMs = alpha*waitCostMs + (1.0f - alpha)*gWaitCostEmaMs;
            gResidualEmaMs = alpha*residualMs + (1.0f - alpha)*gResidualEmaMs;
        }
    }

    // Rolling baseline on smoothed residual: robust anomaly detection for system stalls.
    {
        const int rollingWindow = 120;
        const float epsStd = 0.05f;
        const float absGateMs = focused ? 0.60f : 1.20f;
        const float sigmaFactor = focused ? 3.0f : 4.0f;
        float meanMs = 0.0f;
        float stdMs = 0.0f;
        TelemetryComputeRollingMeanStd(gTelemetry.residualMs, rollingWindow, &meanMs, &stdMs);
        gResidualMeanMs = meanMs;
        gResidualStdMs = stdMs;

        {
            const float thresholdMs = meanMs + sigmaFactor*stdMs;
            float spikeScore = 0.0f;
            unsigned char anomaly = 0;
            if (gResidualEmaMs > thresholdMs)
            {
                spikeScore = (gResidualEmaMs - meanMs)/(stdMs + epsStd);
                if (spikeScore < 0.0f) spikeScore = 0.0f;
                if (spikeScore > 10.0f) spikeScore = 10.0f;
                if (gResidualEmaMs >= absGateMs) anomaly = 1;
            }

            gTelemetry.residualThresholdMs[gTelemetry.head] = thresholdMs;
            gTelemetry.residualSpikeScore[gTelemetry.head] = spikeScore;
            gTelemetry.residualAnomaly[gTelemetry.head] = anomaly;
            if (focused)
            {
                gFocusedSamples++;
                if (anomaly) gFocusedAnomalies++;
            }
            else
            {
                gUnfocusedSamples++;
                if (anomaly) gUnfocusedAnomalies++;
            }
        }
    }

    gTelemetry.frameMs[gTelemetry.head] = frameMs;
    gTelemetry.frameCpuMs[gTelemetry.head] = frameCpuMs;
    gTelemetry.swapCostMs[gTelemetry.head] = swapCostMs;
    gTelemetry.waitCostMs[gTelemetry.head] = gWaitCostEmaMs;
    gTelemetry.residualMs[gTelemetry.head] = gResidualEmaMs;
    gTelemetry.focusedState[gTelemetry.head] = focused ? 1 : 0;
    gTelemetry.pumpTasksLast[gTelemetry.head] = (float)stats->pumpTasksExecutedLast;
    gTelemetry.pumpTimeLastMs[gTelemetry.head] = (float)stats->pumpTimeLastMs;
    gTelemetry.nativeQueueLast[gTelemetry.head] = (float)stats->nativeTaskQueueLastObservedCount;

    gTelemetry.head = (gTelemetry.head + 1) % TELEMETRY_SAMPLES;
    if (gTelemetry.count < TELEMETRY_SAMPLES) gTelemetry.count++;
}

static float TelemetrySeriesMax(const float *series, float floorValue)
{
    float maxv = floorValue;
    for (int i = 0; i < gTelemetry.count; i++)
    {
        const int idx = TelemetryGetIndexByAge(i);
        if (series[idx] > maxv) maxv = series[idx];
    }
    return maxv;
}

static void DrawTelemetryGraph(int x, int y, int w, int h, const char *label, const float *series, float floorMax, RLColor stroke, bool fixedScale, float fixedScaleMax)
{
    const RLColor bg = (RLColor){ 21, 25, 33, 220 };
    const RLColor grid = (RLColor){ 56, 66, 84, 180 };
    const RLColor text = (RLColor){ 208, 216, 228, 255 };
    RLDrawRectangle(x, y, w, h, bg);
    RLDrawRectangleLines(x, y, w, h, (RLColor){ 70, 84, 104, 255 });

    // Grid
    for (int i = 1; i < 4; i++)
    {
        const int gy = y + (h*i)/4;
        RLDrawLine(x + 1, gy, x + w - 2, gy, grid);
    }

    if (gTelemetry.count > 1)
    {
        const int left = x + 6;
        const int right = x + w - 6;
        const int top = y + 20;
        const int bottom = y + h - 6;
        const int drawW = right - left;
        const int drawH = bottom - top;

        float vmax = fixedScale? fixedScaleMax : TelemetrySeriesMax(series, floorMax);
        if (vmax < 0.001f) vmax = 0.001f;
        if (!fixedScale) vmax *= 1.10f; // headroom only for auto-scale

        const int segmentCount = TelemetryGetRenderSegments(gTelemetry.count, 96);
        RLColor band = stroke;
        band.a = (unsigned char)((int)stroke.a/3);

        // 1) Draw min-max band per segment (stable, anti-flicker envelope).
        for (int segment = 0; segment <= segmentCount; segment++)
        {
            const int age0 = TelemetryMapSegmentToAge(segment, segmentCount, gTelemetry.count);
            const int age1 = TelemetryMapSegmentToAge(segment + 1, segmentCount, gTelemetry.count);
            const int a0 = (age0 < age1)? age0 : age1;
            const int a1 = (age0 < age1)? age1 : age0;
            const float vMin = TelemetrySampleMinInAgeRange(series, a0, a1);
            const float vMax = TelemetrySampleMaxInAgeRange(series, a0, a1);
            const float t = (segmentCount > 0)? (float)segment/(float)segmentCount : 0.0f;
            const int sx = left + (int)(t*(float)drawW);
            int yMin = bottom - (int)((vMin/vmax)*(float)drawH);
            int yMax = bottom - (int)((vMax/vmax)*(float)drawH);
            yMin = ClampInt(yMin, top, bottom);
            yMax = ClampInt(yMax, top, bottom);
            RLDrawLine(sx, yMax, sx, yMin, band);
        }

        // 2) Overlay trend line.
        for (int segment = 0; segment < segmentCount; segment++)
        {
            const int age0 = TelemetryMapSegmentToAge(segment, segmentCount, gTelemetry.count);
            const int age1 = TelemetryMapSegmentToAge(segment + 1, segmentCount, gTelemetry.count);
            if (age1 <= age0) continue;

            const float v0 = TelemetrySampleForSegment(series, segment, segmentCount, gTelemetry.count);
            const float v1 = TelemetrySampleForSegment(series, segment + 1, segmentCount, gTelemetry.count);

            const float t0 = (float)segment/(float)segmentCount;
            const float t1 = (float)(segment + 1)/(float)segmentCount;

            const int x0 = left + (int)(t0*(float)drawW);
            const int x1 = left + (int)(t1*(float)drawW);
            int y0 = bottom - (int)((v0/vmax)*(float)drawH);
            int y1 = bottom - (int)((v1/vmax)*(float)drawH);
            y0 = ClampInt(y0, top, bottom);
            y1 = ClampInt(y1, top, bottom);

            RLDrawLine(x0, y0, x1, y1, stroke);
        }

        if (fixedScale) RLDrawText(RLTextFormat("%s  max=%.2f", label, vmax), x + 8, y + 2, 14, text);
        else RLDrawText(RLTextFormat("%s  max~%.2f", label, vmax/1.10f), x + 8, y + 2, 14, text);
    }
    else
    {
        RLDrawText(RLTextFormat("%s  (warming...)", label), x + 8, y + 2, 14, text);
    }
}

static void DrawTelemetryGraphWithThreshold(int x, int y, int w, int h, const char *label, const float *series, const float *thresholdSeries, const unsigned char *anomalySeries, float floorMax, RLColor stroke, bool fixedScale, float fixedScaleMax)
{
    const RLColor bg = (RLColor){ 21, 25, 33, 220 };
    const RLColor grid = (RLColor){ 56, 66, 84, 180 };
    const RLColor text = (RLColor){ 208, 216, 228, 255 };
    const RLColor thresholdCol = (RLColor){ 255, 225, 125, 220 };
    const RLColor anomalyCol = (RLColor){ 255, 96, 96, 255 };
    RLDrawRectangle(x, y, w, h, bg);
    RLDrawRectangleLines(x, y, w, h, (RLColor){ 70, 84, 104, 255 });

    for (int i = 1; i < 4; i++)
    {
        const int gy = y + (h*i)/4;
        RLDrawLine(x + 1, gy, x + w - 2, gy, grid);
    }

    if (gTelemetry.count > 1)
    {
        const int left = x + 6;
        const int right = x + w - 6;
        const int top = y + 20;
        const int bottom = y + h - 6;
        const int drawW = right - left;
        const int drawH = bottom - top;

        float vmax = fixedScale? fixedScaleMax : TelemetrySeriesMax(series, floorMax);
        if (thresholdSeries != NULL)
        {
            float tmax = TelemetrySeriesMax(thresholdSeries, floorMax);
            if (tmax > vmax) vmax = tmax;
        }
        if (vmax < 0.001f) vmax = 0.001f;
        if (!fixedScale) vmax *= 1.10f;

        const int segmentCount = TelemetryGetRenderSegments(gTelemetry.count, 96);
        RLColor band = stroke;
        band.a = (unsigned char)((int)stroke.a/3);

        // 1) Draw min-max band for the primary signal.
        for (int segment = 0; segment <= segmentCount; segment++)
        {
            const int age0 = TelemetryMapSegmentToAge(segment, segmentCount, gTelemetry.count);
            const int age1 = TelemetryMapSegmentToAge(segment + 1, segmentCount, gTelemetry.count);
            const int a0 = (age0 < age1)? age0 : age1;
            const int a1 = (age0 < age1)? age1 : age0;
            const float vMin = TelemetrySampleMinInAgeRange(series, a0, a1);
            const float vMax = TelemetrySampleMaxInAgeRange(series, a0, a1);
            const float t = (segmentCount > 0)? (float)segment/(float)segmentCount : 0.0f;
            const int sx = left + (int)(t*(float)drawW);
            int yMin = bottom - (int)((vMin/vmax)*(float)drawH);
            int yMax = bottom - (int)((vMax/vmax)*(float)drawH);
            yMin = ClampInt(yMin, top, bottom);
            yMax = ClampInt(yMax, top, bottom);
            RLDrawLine(sx, yMax, sx, yMin, band);
        }

        // 2) Overlay trend line.
        for (int segment = 0; segment < segmentCount; segment++)
        {
            const int age0 = TelemetryMapSegmentToAge(segment, segmentCount, gTelemetry.count);
            const int age1 = TelemetryMapSegmentToAge(segment + 1, segmentCount, gTelemetry.count);
            if (age1 <= age0) continue;

            const float v0 = TelemetrySampleForSegment(series, segment, segmentCount, gTelemetry.count);
            const float v1 = TelemetrySampleForSegment(series, segment + 1, segmentCount, gTelemetry.count);

            const float t0 = (float)segment/(float)segmentCount;
            const float t1 = (float)(segment + 1)/(float)segmentCount;
            const int x0 = left + (int)(t0*(float)drawW);
            const int x1 = left + (int)(t1*(float)drawW);
            int y0 = bottom - (int)((v0/vmax)*(float)drawH);
            int y1 = bottom - (int)((v1/vmax)*(float)drawH);
            y0 = ClampInt(y0, top, bottom);
            y1 = ClampInt(y1, top, bottom);
            RLDrawLine(x0, y0, x1, y1, stroke);
        }

        if (thresholdSeries != NULL)
        {
            for (int segment = 0; segment < segmentCount; segment++)
            {
                const int age0 = TelemetryMapSegmentToAge(segment, segmentCount, gTelemetry.count);
                const int age1 = TelemetryMapSegmentToAge(segment + 1, segmentCount, gTelemetry.count);
                if (age1 <= age0) continue;

                const float v0 = TelemetrySampleForSegment(thresholdSeries, segment, segmentCount, gTelemetry.count);
                const float v1 = TelemetrySampleForSegment(thresholdSeries, segment + 1, segmentCount, gTelemetry.count);
                const float t0 = (float)segment/(float)segmentCount;
                const float t1 = (float)(segment + 1)/(float)segmentCount;
                const int x0 = left + (int)(t0*(float)drawW);
                const int x1 = left + (int)(t1*(float)drawW);
                int y0 = bottom - (int)((v0/vmax)*(float)drawH);
                int y1 = bottom - (int)((v1/vmax)*(float)drawH);
                y0 = ClampInt(y0, top, bottom);
                y1 = ClampInt(y1, top, bottom);
                RLDrawLine(x0, y0, x1, y1, thresholdCol);
            }
        }

        if (anomalySeries != NULL)
        {
            for (int segment = 0; segment <= segmentCount; segment++)
            {
                const int age = TelemetryMapSegmentToAge(segment, segmentCount, gTelemetry.count);
                const int idx = TelemetryGetIndexByAge(age);
                if (anomalySeries[idx] == 0) continue;
                const float t = (segmentCount > 0)? (float)segment/(float)segmentCount : 0.0f;
                const int sx = left + (int)(t*(float)drawW);
                int sy = bottom - (int)((series[idx]/vmax)*(float)drawH);
                sy = ClampInt(sy, top, bottom);
                RLDrawCircle(sx, sy, 2.0f, anomalyCol);
            }
        }

        if (fixedScale) RLDrawText(RLTextFormat("%s  max=%.2f", label, vmax), x + 8, y + 2, 14, text);
        else RLDrawText(RLTextFormat("%s  max~%.2f", label, vmax/1.10f), x + 8, y + 2, 14, text);
    }
    else
    {
        RLDrawText(RLTextFormat("%s  (warming...)", label), x + 8, y + 2, 14, text);
    }
}

static void DrawTelemetryOverlay(int x, int y, int w, int h)
{
    gChartDisplayMaxReady = true;
    const double now = RLGetTime();
    const bool scaleTick = (gChartScaleLastUpdateSec <= 0.0) || ((now - gChartScaleLastUpdateSec) >= 0.25);
    if (scaleTick) gChartScaleLastUpdateSec = now;

    const RLColor frame = (RLColor){ 90, 106, 130, 220 };
    RLDrawRectangle(x, y, w, h, (RLColor){ 14, 16, 22, 190 });
    RLDrawRectangleLines(x, y, w, h, frame);
    RLDrawText("Live Telemetry", x + 10, y + 8, 18, (RLColor){ 232, 238, 250, 255 });

    const int pad = 10;
    const int header = 34;
    const int innerX = x + pad;
    const int innerY = y + header;
    const int innerW = w - pad*2;
    const int innerH = h - header - pad;
    const int gapX = 6;
    const int gapY = 6;
    const int cols = 2;
    const int rows = 4;
    const int gW = (innerW - gapX*(cols - 1))/cols;
    const int gH = (innerH - gapY*(rows - 1))/rows;

    const float frameScale = TelemetryGetDisplayScale(0, TelemetrySeriesMax(gTelemetry.frameMs, 16.67f)*1.10f, 16.67f, scaleTick);
    const float frameCpuScale = TelemetryGetDisplayScale(1, TelemetrySeriesMax(gTelemetry.frameCpuMs, 12.0f)*1.10f, 12.0f, scaleTick);
    const float swapScale = TelemetryGetDisplayScale(2, TelemetrySeriesMax(gTelemetry.swapCostMs, 1.0f)*1.10f, 1.0f, scaleTick);
    const float waitScale = TelemetryGetDisplayScale(3, TelemetrySeriesMax(gTelemetry.waitCostMs, 1.0f)*1.10f, 1.0f, scaleTick);
    float residualTarget = TelemetrySeriesMax(gTelemetry.residualMs, 1.0f);
    {
        const float thresholdTarget = TelemetrySeriesMax(gTelemetry.residualThresholdMs, 1.0f);
        if (thresholdTarget > residualTarget) residualTarget = thresholdTarget;
    }
    const float residualScale = TelemetryGetDisplayScale(4, residualTarget*1.10f, 2.0f, scaleTick);
    const float spikeScale = TelemetryGetDisplayScale(5, TelemetrySeriesMax(gTelemetry.residualSpikeScore, 1.0f)*1.10f, 10.0f, scaleTick);
    const float pumpTasksScale = TelemetryGetDisplayScale(6, TelemetrySeriesMax(gTelemetry.pumpTasksLast, 8.0f)*1.10f, 8.0f, scaleTick);
    const float pumpTimeScale = TelemetryGetDisplayScale(7, TelemetrySeriesMax(gTelemetry.pumpTimeLastMs, 1.0f)*1.10f, 1.0f, scaleTick);

    DrawTelemetryGraph(innerX + (gW + gapX)*0, innerY + (gH + gapY)*0, gW, gH, "Frame Time (ms)", gTelemetry.frameMs, 16.67f, (RLColor){ 255, 180, 90, 255 }, true, frameScale);
    DrawTelemetryGraph(innerX + (gW + gapX)*1, innerY + (gH + gapY)*0, gW, gH, "Frame CPU (ms)", gTelemetry.frameCpuMs, 12.0f, (RLColor){ 255, 135, 110, 255 }, true, frameCpuScale);
    DrawTelemetryGraph(innerX + (gW + gapX)*0, innerY + (gH + gapY)*1, gW, gH, "Swap Cost (ms)", gTelemetry.swapCostMs, 1.0f, (RLColor){ 255, 95, 170, 255 }, true, swapScale);
    DrawTelemetryGraph(innerX + (gW + gapX)*1, innerY + (gH + gapY)*1, gW, gH, "Wait Cost (ms)", gTelemetry.waitCostMs, 1.0f, (RLColor){ 255, 225, 125, 255 }, true, waitScale);
    DrawTelemetryGraphWithThreshold(innerX + (gW + gapX)*0, innerY + (gH + gapY)*2, gW, gH, "Timing Residual (ms)", gTelemetry.residualMs, gTelemetry.residualThresholdMs, gTelemetry.residualAnomaly, 1.0f, (RLColor){ 195, 160, 255, 255 }, true, residualScale);
    DrawTelemetryGraph(innerX + (gW + gapX)*1, innerY + (gH + gapY)*2, gW, gH, "SpikeScore (Residual)", gTelemetry.residualSpikeScore, 1.0f, (RLColor){ 120, 205, 255, 255 }, true, spikeScale);
    DrawTelemetryGraph(innerX + (gW + gapX)*0, innerY + (gH + gapY)*3, gW, gH, "Pump Tasks Last", gTelemetry.pumpTasksLast, 8.0f, (RLColor){ 120, 205, 255, 255 }, true, pumpTasksScale);
    DrawTelemetryGraph(innerX + (gW + gapX)*1, innerY + (gH + gapY)*3, gW, gH, "Pump Time Last (ms)", gTelemetry.pumpTimeLastMs, 1.0f, (RLColor){ 156, 188, 255, 255 }, true, pumpTimeScale);
    // Native queue last is still shown in text panel; keep 8 charts total for readability.
}

int main(int argc, char **argv)
{
#if defined(_WIN32)
    // Example-local console setup: print UTF-8 paths correctly on Windows terminals.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    if ((argc >= 2) && (strcmp(argv[1], "--fileio-smoke") == 0))
    {
        const char *rootOverride = (argc >= 3)? argv[2] : NULL;
        const char *uncRootOverride = (argc >= 4)? argv[3] : NULL;
        return RunFileIoSmokeCli(rootOverride, uncRootOverride);
    }

#if defined(_WIN32)
    if ((argc >= 2) && (strcmp(argv[1], "--queue-saturation-selftest") == 0))
    {
        return RunQueueSaturationSelfTest();
    }

    if ((argc >= 2) && (strcmp(argv[1], "--semaphore-selftest") == 0))
    {
        return RunSemaphoreSelfTest();
    }
#endif

    // RLSetTraceLogLevel(RL_E_LOG_NONE); // Disable trace log message

    // Enable Win32 event-thread mode via config flags.
    // (The modified platform layer should read this flag and start the event thread.)
    RLSetConfigFlags(RL_E_FLAG_WINDOW_RESIZABLE | RL_E_FLAG_MSAA_4X_HINT | RL_E_FLAG_WINDOW_EVENT_THREAD);

    RLInitWindow(1280, 720, "raylib Win32 event thread diagnostics (interactive)");
    SharedDiagConfigureTrackedObjectDiagnostics();
    SharedDiagDumpTrackedObjects("main-after-init");
    RLSetTargetFPS(120);
    RunApiSmokeOnce();
#if defined(_WIN32)
    gSharedScenario.mainContext = RLGetCurrentContext();
#endif

    bool showHelp = true;
    bool jitterWindow = false;
    bool jitterResize = false;
    bool warpMouse = false;
    bool showTelemetry = true;
    bool diagPullEnabled = true;
    int diagPage = 0; // 0: event-thread, 1: timing, 2: shared/resource
    int sampleRateMode = 0;
    bool sampleRateComboOpen = false;
    bool telemetrySampleClockReady = false;
    double telemetryLastSampleTime = 0.0;
    RLEventThreadDiagStats lastStats = { 0 };

    float markerRadius = 6.0f;

    // Base window metrics for resize jitter
    const int baseW = 1280;
    const int baseH = 720;
    const double stressStartTime = RLGetTime();

    while (!RLWindowShouldClose())
    {
        // --- toggles ---
        if (RLIsKeyPressed(RL_E_KEY_H)) showHelp = !showHelp;
        if (RLIsKeyPressed(RL_E_KEY_R))
        {
            RLResetEventThreadDiagStatsForCurrentContext();
            RLResetThreadMismatchDiagStats();
            TelemetryResetDisplayScale();
            gDerivedEmaReady = false;
            gWaitCostEmaMs = 0.0f;
            gResidualEmaMs = 0.0f;
            gResidualMeanMs = 0.0f;
            gResidualStdMs = 0.0f;
            gFocusedSamples = 0;
            gFocusedAnomalies = 0;
            gUnfocusedSamples = 0;
            gUnfocusedAnomalies = 0;
        }
        if (RLIsKeyPressed(RL_E_KEY_T))
        {
            diagPullEnabled = !diagPullEnabled;
            telemetrySampleClockReady = false;
            if (diagPullEnabled) RLEnableEventDiagStats();
            else { RLDisableEventDiagStats(); sampleRateComboOpen = false; }
        }
        if (RLIsKeyPressed(RL_E_KEY_C)) ClearMarkers();
        if (RLIsKeyPressed(RL_E_KEY_J)) jitterWindow = !jitterWindow;
        if (RLIsKeyPressed(RL_E_KEY_U)) jitterResize = !jitterResize;
        if (RLIsKeyPressed(RL_E_KEY_W)) warpMouse = !warpMouse;
        if (RLIsKeyPressed(RL_E_KEY_V)) { gDrawMarkersDirect = !gDrawMarkersDirect; gCanvasDirty = true; }
        if (RLIsKeyPressed(RL_E_KEY_K)) SharedDiagDumpTrackedObjects("main-manual-key-dump");
        if (RLIsKeyPressed(RL_E_KEY_G)) showTelemetry = !showTelemetry;
        if (RLIsKeyPressed(RL_E_KEY_ONE)) diagPage = 0;
        if (RLIsKeyPressed(RL_E_KEY_TWO)) diagPage = 1;
        if (RLIsKeyPressed(RL_E_KEY_THREE)) diagPage = 2;
#if defined(_WIN32)
        if (RLIsKeyPressed(RL_E_KEY_Y)) SharedDiagTryStartSameThreadMultiWindowTest();
        if (RLIsKeyPressed(RL_E_KEY_X)) SharedDiagTryStartCrossThreadSharedWindowTest();
        SharedDiagPumpThreadState();
#endif

        // Layout (used by both input + draw)
        const int pad = 12;
        const int panelW = 520;

        const int canvasX = pad;
        const int canvasY = pad;
        const int canvasW = RLGetScreenWidth() - panelW - pad*3;
        const int canvasH = RLGetScreenHeight() - pad*2;

        // Ensure canvas render target exists (unless you want direct-mode only)
        if (!gDrawMarkersDirect)
        {
            EnsureCanvas(canvasW, canvasH);
            if (gCanvasDirty) RebuildCanvas(canvasX, canvasY);
        }

        // --- interactive drawing stress ---
        const RLVector2 mp = RLGetMousePosition();
        const RLRectangle canvasRect = { (float)canvasX, (float)canvasY, (float)canvasW, (float)canvasH };
        const bool mouseInCanvas = IsPointInsideRect(mp, canvasRect);

        if (mouseInCanvas && RLIsMouseButtonPressed(RL_E_MOUSE_BUTTON_LEFT))
        {
            AddMarker(mp, markerRadius);
            if (!gDrawMarkersDirect)
            {
                if ((gCanvas.id != 0) && !gCanvasDirty) DrawLastMarkerToCanvas(canvasX, canvasY);
                else gCanvasDirty = true;
            }
        }

        if (mouseInCanvas && RLIsMouseButtonDown(RL_E_MOUSE_BUTTON_LEFT))
        {
            // Paint at a controlled rate (still generates lots of mouse move + button polling).
            static double lastPaint = 0.0;
            const double now = RLGetTime();
            if (now - lastPaint > 0.008)
            {
                AddMarker(mp, markerRadius);
                if (!gDrawMarkersDirect)
                {
                    if ((gCanvas.id != 0) && !gCanvasDirty) DrawLastMarkerToCanvas(canvasX, canvasY);
                    else gCanvasDirty = true;
                }
                lastPaint = now;
            }
        }

        if (mouseInCanvas && RLIsMouseButtonPressed(RL_E_MOUSE_BUTTON_RIGHT))
        {
            PopMarker();
            if (!gDrawMarkersDirect && gCanvasDirty) RebuildCanvas(canvasX, canvasY);
        }
        if (mouseInCanvas && RLIsMouseButtonPressed(RL_E_MOUSE_BUTTON_MIDDLE))
        {
            ClearMarkers();
            if (!gDrawMarkersDirect && gCanvasDirty) RebuildCanvas(canvasX, canvasY);
        }

        // Wheel changes marker size (also stresses wheel event path)
        const RLVector2 wheel = RLGetMouseWheelMoveV();
        if ((wheel.x != 0.0f) || (wheel.y != 0.0f))
        {
            markerRadius += wheel.y*2.0f;
            if (markerRadius < 1.0f) markerRadius = 1.0f;
            if (markerRadius > 60.0f) markerRadius = 60.0f;
        }

        // --- programmatic stress modes (optional) ---
        const double stressElapsedTime = RLGetTime() - stressStartTime;

        if (jitterWindow)
        {
            // Small sinusoidal movement (stresses window-pos callbacks)
            const int dx = (int)(8.0*sin(stressElapsedTime*2.0));
            const int dy = (int)(6.0*cos(stressElapsedTime*1.7));
            RLSetWindowPosition(80 + dx, 80 + dy);
        }

        if (jitterResize)
        {
            // Resizing (stresses framebuffer-size + projection update paths)
            const int dw = (int)(80.0*sin(stressElapsedTime*1.5));
            const int dh = (int)(60.0*cos(stressElapsedTime*1.2));
            const int w = baseW + dw;
            const int h = baseH + dh;
            RLSetWindowSize(w, h);
        }

        if (warpMouse)
        {
            // Mouse warping (stresses mouse-move callbacks)
            const int sw = RLGetScreenWidth();
            const int sh = RLGetScreenHeight();
            const int cx = sw/2;
            const int cy = sh/2;
            const int rx = (int)(0.35*sw);
            const int ry = (int)(0.25*sh);
            const int x = cx + (int)(rx*cos(stressElapsedTime*3.3));
            const int y = cy + (int)(ry*sin(stressElapsedTime*2.9));
            RLSetMousePosition(x, y);
        }

        RLEventThreadDiagStats stats = lastStats;
        const float frameTimeMs = RLGetFrameTime()*1000.0f;
        if (diagPullEnabled)
        {
            const double now = RLGetTime();
            const double intervalSec = gTelemetrySampleModes[sampleRateMode].intervalSec;
            if (!telemetrySampleClockReady || (intervalSec <= 0.0) || ((now - telemetryLastSampleTime) >= intervalSec))
            {
                stats = RLGetEventThreadDiagStats();
                lastStats = stats;
                telemetryLastSampleTime = now;
                telemetrySampleClockReady = true;
                TelemetryPush(&stats, frameTimeMs, RLIsWindowFocused());
            }
        }

        // --- draw ---
        RLBeginDrawing();
        RLClearBackground((RLColor){ 20, 20, 24, 255 });

        RLDrawRectangle(canvasX, canvasY, canvasW, canvasH, (RLColor){ 26, 26, 34, 255 });
        RLDrawRectangleLines(canvasX, canvasY, canvasW, canvasH, (RLColor){ 70, 70, 90, 255 });

        // Markers
        if (!gDrawMarkersDirect && (gCanvas.id != 0))
        {
            // RenderTexture in raylib is Y-flipped when drawn; use negative height.
            RLRectangle src = { 0.0f, 0.0f, (float)gCanvas.texture.width, -(float)gCanvas.texture.height };
            RLVector2 dst = { (float)canvasX, (float)canvasY };
            RLDrawTextureRec(gCanvas.texture, src, dst, WHITE);
        }
        else
        {
            for (int i = 0; i < gMarkerCount; i++) RLDrawCircleV(gMarkers[i].pos, gMarkers[i].r, gMarkers[i].col);
        }

        // Crosshair
        RLDrawLineV((RLVector2){ mp.x - 10, mp.y }, (RLVector2){ mp.x + 10, mp.y }, (RLColor){ 240, 240, 240, 160 });
        RLDrawLineV((RLVector2){ mp.x, mp.y - 10 }, (RLVector2){ mp.x, mp.y + 10 }, (RLColor){ 240, 240, 240, 160 });

        // Diagnostics panel
        const int px = RLGetScreenWidth() - panelW - pad;
        const int py = pad;
        const int ph = RLGetScreenHeight() - pad*2;

        RLDrawRectangle(px, py, panelW, ph, (RLColor){ 18, 18, 22, 255 });
        RLDrawRectangleLines(px, py, panelW, ph, (RLColor){ 70, 70, 90, 255 });

        int tx = px + 14;
        int ty = py + 12;

        RLDrawText("Diagnostics", tx, ty, 20, WHITE);
        ty += 30;

        RLDrawText(RLTextFormat("diag pull/runtime: %s / %s",
            diagPullEnabled? "enabled" : "disabled",
            RLIsEventDiagStatsEnabled()? "enabled" : "disabled"), tx, ty, 16, RAYWHITE); ty += 20;
        {
            RLContext *diagContext = RLGetCurrentContext();
            RLContextResourceShareMode shareMode = RLContextGetResourceShareMode(diagContext);
            RLContext *shareTargetContext = RLContextGetResourceShareContext(diagContext);
            bool shareConfigValid = RLContextValidateResourceShareConfig(diagContext);
            int shareValidationError = RLContextGetResourceShareValidationError(diagContext);

            RLDrawText(RLTextFormat("share mode/valid/error: %s / %s / %d",
                ShareModeToText(shareMode), shareConfigValid ? "yes" : "no", shareValidationError), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("share target ctx: %s", (shareTargetContext != NULL) ? "set" : "null"), tx, ty, 16, RAYWHITE); ty += 20;
        }
        RLDrawText("sample:", tx, ty, 16, (RLColor){ 160, 180, 220, 255 });
        const int comboX = tx + 170;
        const int comboY = ty - 4;
        const int comboW = 250;
        const int comboH = 30;
        ty += 36;
        RLDrawText(RLTextFormat("page: [%d]  1:event  2:timing  3:shared", diagPage + 1), tx, ty, 16, (RLColor){ 160, 180, 220, 255 }); ty += 24;
        ty += 4;

        if (diagPage == 0)
        {
            RLDrawText("logical queue = posted - executed (raylib)", tx, ty, 16, (RLColor){ 160, 180, 220, 255 }); ty += 20;
            RLDrawText("native queue = glfw ring queue realtime", tx, ty, 16, (RLColor){ 160, 180, 220, 255 }); ty += 20;
            ty += 4;

            RLDrawText(RLTextFormat("tasks posted/executed/failed: %llu / %llu / %llu",
                stats.tasksPosted, stats.tasksExecuted, stats.tasksPostFailed), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("logical queue depth current/max: %llu / %llu",
                stats.taskQueueDepthCurrent, stats.taskQueueDepthMax), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("native queue queued/peak/dropped: %u / %u / %llu",
                stats.nativeTaskQueueCount, stats.nativeTaskQueuePeakCount, stats.nativeTaskQueueDroppedCount), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("native queue last observed: %u",
                stats.nativeTaskQueueLastObservedCount), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("native dropped C/S/I/M: %llu / %llu / %llu / %llu",
                stats.nativeTaskQueueDroppedCriticalCount, stats.nativeTaskQueueDroppedStateCount,
                stats.nativeTaskQueueDroppedInputCount, stats.nativeTaskQueueDroppedMaintenanceCount), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("native wake sent/dedup: %llu / %llu",
                stats.nativeTaskWakeSentCount, stats.nativeTaskWakeDedupCount), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("frame-cb queued critical/total/peak: %u / %u / %u",
                stats.frameCallbackQueueCriticalCount, stats.frameCallbackQueueCount, stats.frameCallbackQueuePeakCount), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("frame-cb dropped total/N/C: %llu / %llu / %llu",
                stats.frameCallbackDroppedCount, stats.frameCallbackDroppedNormalCount, stats.frameCallbackDroppedCriticalCount), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("frame-cb evict N->C: %llu",
                stats.frameCallbackEvictedNormalForCriticalCount), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("thread mismatch detected: %llu", stats.threadMismatchDetectedCount), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("mismatch handoff attempt/success/fail: %llu / %llu / %llu",
                stats.threadMismatchHandoffAttemptedCount, stats.threadMismatchHandoffSuccessCount, stats.threadMismatchHandoffFailedCount), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("mismatch deferred queued/executed/failed: %llu / %llu / %llu",
                stats.threadMismatchDeferredQueuedCount, stats.threadMismatchDeferredExecutedCount, stats.threadMismatchDeferredFailedCount), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("mismatch rejected: %llu", stats.threadMismatchRejectedCount), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("thread mismatch last: api=%s tid=%llu", stats.threadMismatchLastApi, stats.threadMismatchLastCallerThreadId), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("renderCall alloc/free: %llu / %llu", stats.renderCallAlloc, stats.renderCallFree), tx, ty, 16, RAYWHITE); ty += 20;
            ty += 10;
            RLDrawText(RLTextFormat("payload alloc/free: %llu / %llu", stats.payloadAlloc, stats.payloadFree), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("payload bytes alloc/free: %llu / %llu", stats.payloadAllocBytes, stats.payloadFreeBytes), tx, ty, 16, RAYWHITE); ty += 20;
            {
                long long payloadOutstanding = (long long)stats.payloadAlloc - (long long)stats.payloadFree;
                RLDrawText(RLTextFormat("payload outstanding/max: %lld / %llu", payloadOutstanding, stats.payloadOutstandingMax), tx, ty, 16, RAYWHITE); ty += 20;
            }
            RLDrawText(RLTextFormat("mouseMove alloc/free: %llu / %llu  mouseWheel alloc/free: %llu / %llu",
                stats.mouseMoveAlloc, stats.mouseMoveFree, stats.mouseWheelAlloc, stats.mouseWheelFree), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("winPos alloc/free: %llu / %llu  scale alloc/free: %llu / %llu",
                stats.winPosAlloc, stats.winPosFree, stats.scaleAlloc, stats.scaleFree), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("fbSize alloc/free: %llu / %llu",
                stats.fbSizeAlloc, stats.fbSizeFree), tx, ty, 16, RAYWHITE); ty += 20;
        }
        else if (diagPage == 1)
        {
            RLDrawText(RLTextFormat("pump calls: %llu  time total/max: %.3f/%.3f ms",
                stats.pumpCalls, stats.pumpTimeTotalMs, stats.pumpTimeMaxMs), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("pump time last: %.3f ms", stats.pumpTimeLastMs), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("swap last/max: %.3f / %.3f ms", stats.swapCostLastMs, stats.swapCostMaxMs), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("wait last/max: %.3f / %.3f ms", stats.waitCostLastMs, stats.waitCostMaxMs), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("frameCpu last/max: %.3f / %.3f ms", stats.frameCpuLastMs, stats.frameCpuMaxMs), tx, ty, 16, RAYWHITE); ty += 20;
            {
                float residualMs = frameTimeMs - (float)stats.frameCpuLastMs - (float)stats.swapCostLastMs - (float)stats.waitCostLastMs;
                if (residualMs < 0.0f) residualMs = 0.0f;
                RLDrawText(RLTextFormat("timing residual: %.3f ms", residualMs), tx, ty, 16, RAYWHITE); ty += 20;
                RLDrawText(RLTextFormat("residual baseline mu/sigma: %.3f / %.3f ms", gResidualMeanMs, gResidualStdMs), tx, ty, 16, RAYWHITE); ty += 20;
                {
                    const double focusedRate = (gFocusedSamples > 0)? (100.0*(double)gFocusedAnomalies/(double)gFocusedSamples) : 0.0;
                    const double unfocusedRate = (gUnfocusedSamples > 0)? (100.0*(double)gUnfocusedAnomalies/(double)gUnfocusedSamples) : 0.0;
                    RLDrawText(RLTextFormat("anomaly rate F/U: %.2f%% / %.2f%%", focusedRate, unfocusedRate), tx, ty, 16, RAYWHITE);
                }
                ty += 20;
            }
            RLDrawText(RLTextFormat("pump tasks total/max: %llu / %u",
                stats.pumpTasksExecutedTotal, stats.pumpTasksExecutedMax), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("pump tasks last: %u", stats.pumpTasksExecutedLast), tx, ty, 16, RAYWHITE); ty += 20;
            ty += 10;
            RLDrawText(RLTextFormat("markers: %d  size: %.1f", gMarkerCount, markerRadius), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("wheel: (%.2f, %.2f)", wheel.x, wheel.y), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("modes: jitterWin=%d jitterResize=%d warpMouse=%d", (int)jitterWindow, (int)jitterResize, (int)warpMouse), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("frameTime: %.3f ms", frameTimeMs), tx, ty, 16, RAYWHITE); ty += 20;
        }
        else
        {
            const unsigned long long sharedRejectTotal = stats.sharedUnregisteredRetainRejectCount + stats.sharedUnregisteredReleaseRejectCount;
            RLDrawText("shared/resource page (needs shared workload)", tx, ty, 16, (RLColor){ 160, 180, 220, 255 }); ty += 20;
            RLDrawText("tip: run core_shared_gpu_context for active validation", tx, ty, 16, (RLColor){ 160, 180, 220, 255 }); ty += 20;
            ty += 4;
            RLDrawText(RLTextFormat("share mode/valid/error: %s / %s / %d",
                ShareModeToText(RLContextGetResourceShareMode(RLGetCurrentContext())),
                RLContextValidateResourceShareConfig(RLGetCurrentContext()) ? "yes" : "no",
                RLContextGetResourceShareValidationError(RLGetCurrentContext())), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("shared unregistered retain/release rejected: %llu / %llu",
                stats.sharedUnregisteredRetainRejectCount, stats.sharedUnregisteredReleaseRejectCount), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("shared reject total: %llu", sharedRejectTotal), tx, ty, 16, RAYWHITE); ty += 20;
            if (sharedRejectTotal == 0)
            {
                RLDrawText("current run has no visible shared pressure yet", tx, ty, 16, (RLColor){ 190, 200, 220, 255 }); ty += 20;
            }
            ty += 8;
            RLDrawText("api smoke:", tx, ty, 16, (RLColor){ 160, 180, 220, 255 }); ty += 20;
            RLDrawText(RLTextFormat("TextFormatTo: %s", gApiSmoke.textFormatOk? "ok" : "fail"), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("Save/Load text: %s/%s", gApiSmoke.saveTextOk? "ok" : "fail", gApiSmoke.loadTextOk? "ok" : "fail"), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("Save/Load data: %s/%s", gApiSmoke.saveDataOk? "ok" : "fail", gApiSmoke.loadDataOk? "ok" : "fail"), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("FileExists/DirExists/ChangeDir: %s/%s/%s", gApiSmoke.fileExistsOk? "ok" : "fail", gApiSmoke.dirExistsOk? "ok" : "fail", gApiSmoke.changeDirOk? "ok" : "fail"), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("AppDirLen=%d WorkDirLen=%d", gApiSmoke.appDirLen, gApiSmoke.workDirLen), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("textPathLen=%d dataPathLen=%d", gApiSmoke.textPathLen, gApiSmoke.dataPathLen), tx, ty, 16, RAYWHITE); ty += 20;

#if defined(_WIN32)
            ty += 8;
            RLDrawText("shared scenario tests (Y/X trigger):", tx, ty, 16, (RLColor){ 160, 180, 220, 255 }); ty += 20;
            RLDrawText(RLTextFormat("same-thread running/start: %ld / %ld",
                InterlockedCompareExchange(&gSharedScenario.sameThreadRunning, 0, 0),
                InterlockedCompareExchange(&gSharedScenario.sameThreadStartCount, 0, 0)), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("same-thread reject/unexpected/fail: %ld / %ld / %ld",
                InterlockedCompareExchange(&gSharedScenario.sameThreadExpectedRejectCount, 0, 0),
                InterlockedCompareExchange(&gSharedScenario.sameThreadUnexpectedSuccessCount, 0, 0),
                InterlockedCompareExchange(&gSharedScenario.sameThreadFailureCount, 0, 0)), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("cross-thread running/start: %ld / %ld",
                InterlockedCompareExchange(&gSharedScenario.crossThreadRunning, 0, 0),
                InterlockedCompareExchange(&gSharedScenario.crossThreadStartCount, 0, 0)), tx, ty, 16, RAYWHITE); ty += 20;
            RLDrawText(RLTextFormat("cross-thread success/fail: %ld / %ld",
                InterlockedCompareExchange(&gSharedScenario.crossThreadSuccessCount, 0, 0),
                InterlockedCompareExchange(&gSharedScenario.crossThreadFailureCount, 0, 0)), tx, ty, 16, RAYWHITE); ty += 20;
#endif
        }

        if (showHelp)
        {
            const int helpX = canvasX + 10;
            const int helpHeight = GetOverlayHelpHeight();
            const int helpMargin = 8;
            int helpY = canvasY + canvasH - 80;
            const int minHelpY = canvasY + helpMargin;
            int maxHelpY = canvasY + canvasH - helpHeight - helpMargin;
            if (maxHelpY < minHelpY) maxHelpY = minHelpY;
            if (helpY < minHelpY) helpY = minHelpY;
            if (helpY > maxHelpY) helpY = maxHelpY;
            DrawOverlayHelp(helpX, helpY);
        }
        if (showTelemetry)
        {
            const int gw = (canvasW < 680)? canvasW - 24 : 640;
            const int gh = (canvasH < 300)? canvasH - 24 : 290;
            DrawTelemetryOverlay(canvasX + 12, canvasY + 12, gw, gh);
        }

        // Draw sample-rate combo as top-most UI to avoid being covered by panel text.
        if (DrawSampleRateCombo(comboX, comboY, comboW, comboH, &sampleRateMode, &sampleRateComboOpen))
        {
            telemetrySampleClockReady = false;
            TelemetryResetDisplayScale();
        }

        RLDrawFPS(px + panelW - 90, py + 10);

        RLEndDrawing();
    }

#if defined(_WIN32)
    SharedDiagJoinThreadForShutdown(&gSharedScenario.sameThreadHandle, &gSharedScenario.sameThreadRunning, "same-thread");
    SharedDiagJoinThreadForShutdown(&gSharedScenario.crossThreadHandle, &gSharedScenario.crossThreadRunning, "cross-thread");
#endif

    if (gCanvas.id != 0)
    {
        SharedDiagDumpTrackedObjects("main-before-final-canvas-unload");
        RLUnloadRenderTexture(gCanvas);
        gCanvas = (RLRenderTexture2D){ 0 };
        SharedDiagDumpTrackedObjects("main-after-final-canvas-unload");
    }

    SharedDiagDumpTrackedObjects("main-before-close-window");
    RLCloseWindow();
    return 0;
}
