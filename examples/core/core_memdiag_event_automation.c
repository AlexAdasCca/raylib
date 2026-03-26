/*******************************************************************************************
*
*   raylib [core] example - memory diagnostics with automated input playback
*
*   This example runs an automated keyboard/mouse event script, drives callback-heavy paths,
*   and prints memory diagnostics summary at shutdown.
*
*   Expected usage:
*     1) Build raylib with RL_MEM_DIAG=1 (Debug diagnostics build).
*     2) Run this executable and inspect MEMDIAG_AUTOTEST output.
*
********************************************************************************************/

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMDIAG_AUTO_WIDTH  960
#define MEMDIAG_AUTO_HEIGHT 540
#define MEMDIAG_MAX_MARKERS 2048
#define MEMDIAG_SETTLE_FRAMES 120u

typedef struct Marker {
    RLVector2 position;
    float radius;
    RLColor color;
} Marker;

typedef struct AutoScriptState {
    RLAutomationEventList eventList;
    unsigned int playIndex;
    unsigned int lastFrame;
} AutoScriptState;

// Mirrors rcore.c AutomationEventType values.
typedef enum LocalAutomationEventType {
    LOCAL_EVENT_NONE = 0,
    LOCAL_INPUT_KEY_UP,
    LOCAL_INPUT_KEY_DOWN,
    LOCAL_INPUT_KEY_PRESSED,
    LOCAL_INPUT_KEY_RELEASED,
    LOCAL_INPUT_MOUSE_BUTTON_UP,
    LOCAL_INPUT_MOUSE_BUTTON_DOWN,
    LOCAL_INPUT_MOUSE_POSITION,
    LOCAL_INPUT_MOUSE_WHEEL_MOTION
} LocalAutomationEventType;

static Marker gMarkers[MEMDIAG_MAX_MARKERS];
static int gMarkerCount = 0;
static RLColor gPalette[] = {
    { 230, 41, 55, 255 },   // RED
    { 255, 161, 0, 255 },   // ORANGE
    { 255, 203, 0, 255 },   // GOLD
    { 0, 158, 47, 255 },    // LIME
    { 102, 191, 255, 255 }, // SKYBLUE
    { 0, 121, 241, 255 },   // BLUE
    { 135, 60, 190, 255 }   // VIOLET
};
static int gPaletteIndex = 0;

static void PushMarker(RLVector2 position, float radius)
{
    if (gMarkerCount >= MEMDIAG_MAX_MARKERS) return;

    gMarkers[gMarkerCount].position = position;
    gMarkers[gMarkerCount].radius = radius;
    gMarkers[gMarkerCount].color = gPalette[gPaletteIndex];
    gMarkerCount++;
}

static void PopMarker(void)
{
    if (gMarkerCount > 0) gMarkerCount--;
}

static void ClearMarkers(void)
{
    gMarkerCount = 0;
}

static void CyclePalette(void)
{
    gPaletteIndex++;
    if (gPaletteIndex >= (int)(sizeof(gPalette)/sizeof(gPalette[0]))) gPaletteIndex = 0;
}

static int AppendAutoEvent(AutoScriptState *script, unsigned int frame, unsigned int type, int param0, int param1, int param2, int param3)
{
    if ((script == NULL) || (script->eventList.events == NULL)) return 0;
    if (script->eventList.count >= script->eventList.capacity) return 0;

    RLAutomationEvent *eventEntry = &script->eventList.events[script->eventList.count];
    eventEntry->frame = frame;
    eventEntry->type = type;
    eventEntry->params[0] = param0;
    eventEntry->params[1] = param1;
    eventEntry->params[2] = param2;
    eventEntry->params[3] = param3;
    script->eventList.count++;

    if (frame > script->lastFrame) script->lastFrame = frame;
    return 1;
}

static int BuildAutoScript(AutoScriptState *script)
{
    if (script == NULL) return 0;

    script->eventList = RLLoadAutomationEventList(NULL);
    script->playIndex = 0;
    script->lastFrame = 0;

    if ((script->eventList.events == NULL) || (script->eventList.capacity == 0u)) return 0;

    // Generate deterministic mouse painting + keyboard toggles.
    unsigned int frame = 20u;
    for (int index = 0; index < 120; index++)
    {
        const int x = 120 + ((index * 13) % (MEMDIAG_AUTO_WIDTH - 240));
        const int y = 100 + ((index * 11) % (MEMDIAG_AUTO_HEIGHT - 180));

        if (!AppendAutoEvent(script, frame, LOCAL_INPUT_MOUSE_POSITION, x, y, 0, 0)) return 0;
        if (!AppendAutoEvent(script, frame, LOCAL_INPUT_MOUSE_BUTTON_DOWN, RL_E_MOUSE_BUTTON_LEFT, 0, 0, 0)) return 0;
        if (!AppendAutoEvent(script, frame + 1u, LOCAL_INPUT_MOUSE_BUTTON_UP, RL_E_MOUSE_BUTTON_LEFT, 0, 0, 0)) return 0;

        if ((index % 15) == 0)
        {
            if (!AppendAutoEvent(script, frame + 1u, LOCAL_INPUT_KEY_PRESSED, RL_E_KEY_SPACE, 0, 0, 0)) return 0;
        }
        if ((index % 20) == 10)
        {
            if (!AppendAutoEvent(script, frame + 1u, LOCAL_INPUT_KEY_PRESSED, RL_E_KEY_C, 0, 0, 0)) return 0;
        }
        if ((index % 18) == 7)
        {
            if (!AppendAutoEvent(script, frame + 1u, LOCAL_INPUT_MOUSE_WHEEL_MOTION, 0, 1, 0, 0)) return 0;
        }

        frame += 3u;
    }

    for (int index = 0; index < 40; index++)
    {
        if (!AppendAutoEvent(script, frame, LOCAL_INPUT_KEY_PRESSED, RL_E_KEY_BACKSPACE, 0, 0, 0)) return 0;
        frame += 2u;
    }

    return 1;
}

static void PlayFrameEvents(AutoScriptState *script, unsigned int currentFrame)
{
    if (script == NULL) return;

    while ((script->playIndex < script->eventList.count) && (script->eventList.events[script->playIndex].frame == currentFrame))
    {
        RLPlayAutomationEvent(script->eventList.events[script->playIndex]);
        script->playIndex++;
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    RLContext *mainContext = RLCreateContext();
    if (mainContext == NULL)
    {
        printf("MEMDIAG_AUTOTEST: result=CONTEXT_CREATE_FAILED\n");
        return 4;
    }
    RLSetCurrentContext(mainContext);

    RLEnableMemoryDiagStats();
    RLSetTraceLogLevel(RL_E_LOG_WARNING);
    RLSetConfigFlags(RL_E_FLAG_WINDOW_RESIZABLE | RL_E_FLAG_WINDOW_EVENT_THREAD);
    RLInitWindow(MEMDIAG_AUTO_WIDTH, MEMDIAG_AUTO_HEIGHT, "raylib [core] memdiag event automation");
    RLSetTargetFPS(60);

    AutoScriptState script = { 0 };
    if (!BuildAutoScript(&script))
    {
        printf("MEMDIAG_AUTOTEST: result=SCRIPT_BUILD_FAILED\n");
        if (RLIsWindowReady()) RLCloseWindow();
        RLDestroyContext(mainContext);
        return 2;
    }

    RLSetAutomationEventList(&script.eventList);

    unsigned int frameCounter = 0u;
    while (!RLWindowShouldClose())
    {
        PlayFrameEvents(&script, frameCounter);

        if (RLIsMouseButtonPressed(RL_E_MOUSE_BUTTON_LEFT))
        {
            PushMarker(RLGetMousePosition(), 8.0f + (float)(gPaletteIndex % 5));
        }

        if (RLIsMouseButtonDown(RL_E_MOUSE_BUTTON_LEFT) && ((frameCounter % 2u) == 0u))
        {
            PushMarker(RLGetMousePosition(), 6.0f + (float)(gPaletteIndex % 3));
        }

        if (RLIsMouseButtonPressed(RL_E_MOUSE_BUTTON_RIGHT))
        {
            PopMarker();
        }

        if (RLIsKeyPressed(RL_E_KEY_C))
        {
            ClearMarkers();
        }

        if (RLIsKeyPressed(RL_E_KEY_SPACE))
        {
            CyclePalette();
        }

        if (RLIsKeyPressed(RL_E_KEY_BACKSPACE))
        {
            PopMarker();
        }

        float wheelMove = RLGetMouseWheelMove();
        if (wheelMove > 0.0f)
        {
            for (int step = 0; step < (int)wheelMove; step++) CyclePalette();
        }

        RLBeginDrawing();
            RLClearBackground((RLColor){ 18, 22, 30, 255 });
            for (int markerIndex = 0; markerIndex < gMarkerCount; markerIndex++)
            {
                RLDrawCircleV(gMarkers[markerIndex].position, gMarkers[markerIndex].radius, gMarkers[markerIndex].color);
            }

            RLDrawText("Automated input playback: mouse + keyboard", 20, 20, 20, RAYWHITE);
            RLDrawText(RLTextFormat("frame: %u / last script frame: %u", frameCounter, script.lastFrame), 20, 50, 18, LIGHTGRAY);
            RLDrawText(RLTextFormat("markers: %d", gMarkerCount), 20, 74, 18, LIGHTGRAY);
            RLDrawText("Program exits automatically and prints MEMDIAG_AUTOTEST result", 20, 98, 18, LIGHTGRAY);
        RLEndDrawing();

        if ((script.playIndex >= script.eventList.count) && (frameCounter > (script.lastFrame + MEMDIAG_SETTLE_FRAMES))) break;
        frameCounter++;
    }

    RLSetAutomationEventList(NULL);
    RLUnloadAutomationEventList(script.eventList);

    RLMemoryDiagStats statsBeforeClose = RLGetMemoryDiagStats();
    RLCloseWindow();
    RLMemoryDiagStats statsAfterClose = RLGetMemoryDiagStats();
    RLDestroyContext(mainContext);
    RLMemoryDiagStats statsAfterDestroy = RLGetMemoryDiagStats();

    printf("MEMDIAG_AUTOTEST: pre_close outstanding_bytes=%llu alloc=%llu free=%llu\n",
        statsBeforeClose.currentOutstandingBytes,
        statsBeforeClose.allocCount + statsBeforeClose.callocCount + statsBeforeClose.reallocCount,
        statsBeforeClose.freeCount);
    printf("MEMDIAG_AUTOTEST: post_close outstanding_bytes=%llu peak=%llu alloc_fail=%llu realloc_fail=%llu\n",
        statsAfterClose.currentOutstandingBytes,
        statsAfterClose.peakOutstandingBytes,
        statsAfterClose.allocFailCount,
        statsAfterClose.reallocFailCount);
    printf("MEMDIAG_AUTOTEST: post_destroy outstanding_bytes=%llu alloc=%llu free=%llu\n",
        statsAfterDestroy.currentOutstandingBytes,
        statsAfterDestroy.allocCount + statsAfterDestroy.callocCount + statsAfterDestroy.reallocCount,
        statsAfterDestroy.freeCount);

    if (statsAfterDestroy.currentOutstandingBytes > 0ull)
    {
        printf("MEMDIAG_AUTOTEST: result=LEAK_DETECTED\n");
        RLDumpMemoryLeaks();
        return 3;
    }

    printf("MEMDIAG_AUTOTEST: result=NO_LEAK_DETECTED\n");
    return 0;
}
