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
*     - H: toggle help overlay
*     - J: toggle window position jitter (stresses window-pos callbacks)
*     - U: toggle window resize jitter (stresses fbsize/scale callbacks)
*     - W: toggle mouse warp (stresses mouse-move callbacks)
*
********************************************************************************************/

#include "raylib.h"

#include <math.h>

//------------------------------------------------------------------------------
// Simple marker painter (interactive input stress)
//------------------------------------------------------------------------------
#define MAX_MARKERS 8192

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
    RLDrawText("R: reset diag | H: toggle help | J: window jitter | U: resize jitter | W: mouse warp", x, oy, 16, RAYWHITE); oy += 20;
    RLDrawText("V: toggle marker render mode (canvas/direct)", x, oy, 16, RAYWHITE); oy += 20;

#if defined(RL_EVENTTHREAD_COALESCE_STATE)
    RLDrawText(RLTextFormat("RL_EVENTTHREAD_COALESCE_STATE=%d", (int)RL_EVENTTHREAD_COALESCE_STATE), x, oy, 16, RAYWHITE); oy += 20;
#endif

    (void)oy;
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

int main(void)
{
    // Enable Win32 event-thread mode via config flags.
    // (Your modified platform layer should read this flag and start the event thread.)
    RLSetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_WINDOW_EVENT_THREAD);

    RLInitWindow(1280, 720, "raylib Win32 event thread diagnostics (interactive)");
    RLSetTargetFPS(120);

    bool showHelp = true;
    bool jitterWindow = false;
    bool jitterResize = false;
    bool warpMouse = false;

    float markerRadius = 6.0f;

    // Base window metrics for resize jitter
    const int baseW = 1280;
    const int baseH = 720;
    const double t0 = RLGetTime();

    while (!RLWindowShouldClose())
    {
        // --- toggles ---
        if (RLIsKeyPressed(KEY_H)) showHelp = !showHelp;
        if (RLIsKeyPressed(KEY_R)) RLResetEventThreadDiagStats();
        if (RLIsKeyPressed(KEY_C)) ClearMarkers();
        if (RLIsKeyPressed(KEY_J)) jitterWindow = !jitterWindow;
        if (RLIsKeyPressed(KEY_U)) jitterResize = !jitterResize;
        if (RLIsKeyPressed(KEY_W)) warpMouse = !warpMouse;
        if (RLIsKeyPressed(KEY_V)) { gDrawMarkersDirect = !gDrawMarkersDirect; gCanvasDirty = true; }

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

        if (RLIsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            AddMarker(mp, markerRadius);
            if (!gDrawMarkersDirect)
            {
                if ((gCanvas.id != 0) && !gCanvasDirty) DrawLastMarkerToCanvas(canvasX, canvasY);
                else gCanvasDirty = true;
            }
        }

        if (RLIsMouseButtonDown(MOUSE_BUTTON_LEFT))
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

        if (RLIsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
        {
            PopMarker();
            if (!gDrawMarkersDirect && gCanvasDirty) RebuildCanvas(canvasX, canvasY);
        }
        if (RLIsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
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
        const double t = RLGetTime() - t0;

        if (jitterWindow)
        {
            // Small sinusoidal movement (stresses window-pos callbacks)
            const int dx = (int)(8.0*sin(t*2.0));
            const int dy = (int)(6.0*cos(t*1.7));
            RLSetWindowPosition(80 + dx, 80 + dy);
        }

        if (jitterResize)
        {
            // Resizing (stresses framebuffer-size + projection update paths)
            const int dw = (int)(80.0*sin(t*1.5));
            const int dh = (int)(60.0*cos(t*1.2));
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
            const int x = cx + (int)(rx*cos(t*3.3));
            const int y = cy + (int)(ry*sin(t*2.9));
            RLSetMousePosition(x, y);
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

        const RLEventThreadDiagStats stats = RLGetEventThreadDiagStats();

        RLDrawText(RLTextFormat("tasks posted/executed: %llu / %llu", stats.tasksPosted, stats.tasksExecuted), tx, ty, 16, RAYWHITE); ty += 20;
        RLDrawText(RLTextFormat("renderCall alloc/free: %llu / %llu", stats.renderCallAlloc, stats.renderCallFree), tx, ty, 16, RAYWHITE); ty += 20;

        ty += 10;
        RLDrawText(RLTextFormat("payload alloc/free: %llu / %llu", stats.payloadAlloc, stats.payloadFree), tx, ty, 16, RAYWHITE); ty += 20;
        RLDrawText(RLTextFormat("payload bytes alloc/free: %llu / %llu", stats.payloadAllocBytes, stats.payloadFreeBytes), tx, ty, 16, RAYWHITE); ty += 20;
        {
            // NOTE: The public stats structure exposes max outstanding; current outstanding can be derived.
            long long outstanding = (long long)stats.payloadAlloc - (long long)stats.payloadFree;
            RLDrawText(RLTextFormat("payload outstanding: %lld  max: %llu", outstanding, stats.payloadOutstandingMax), tx, ty, 16, RAYWHITE);
            ty += 20;
        }

        ty += 10;
        RLDrawText(RLTextFormat("mouseMove alloc/free: %llu / %llu", stats.mouseMoveAlloc, stats.mouseMoveFree), tx, ty, 16, RAYWHITE); ty += 20;
        RLDrawText(RLTextFormat("wheel alloc/free: %llu / %llu", stats.mouseWheelAlloc, stats.mouseWheelFree), tx, ty, 16, RAYWHITE); ty += 20;
        RLDrawText(RLTextFormat("winPos alloc/free: %llu / %llu", stats.winPosAlloc, stats.winPosFree), tx, ty, 16, RAYWHITE); ty += 20;
        RLDrawText(RLTextFormat("scale alloc/free: %llu / %llu", stats.scaleAlloc, stats.scaleFree), tx, ty, 16, RAYWHITE); ty += 20;
        RLDrawText(RLTextFormat("fbSize alloc/free: %llu / %llu", stats.fbSizeAlloc, stats.fbSizeFree), tx, ty, 16, RAYWHITE); ty += 20;

        ty += 10;
        RLDrawText(RLTextFormat("pump calls: %llu  time total/max: %.3f/%.3f ms",
            stats.pumpCalls, stats.pumpTimeTotalMs, stats.pumpTimeMaxMs), tx, ty, 16, RAYWHITE);
        ty += 20;
        RLDrawText(RLTextFormat("pump tasks total/max: %llu / %u",
            stats.pumpTasksExecutedTotal, stats.pumpTasksExecutedMax), tx, ty, 16, RAYWHITE);
        ty += 20;

        ty += 10;
        RLDrawText(RLTextFormat("markers: %d  size: %.1f", gMarkerCount, markerRadius), tx, ty, 16, RAYWHITE); ty += 20;
        RLDrawText(RLTextFormat("wheel: (%.2f, %.2f)", wheel.x, wheel.y), tx, ty, 16, RAYWHITE); ty += 20;
        RLDrawText(RLTextFormat("modes: jitterWin=%d jitterResize=%d warpMouse=%d", (int)jitterWindow, (int)jitterResize, (int)warpMouse), tx, ty, 16, RAYWHITE); ty += 20;
        RLDrawText(RLTextFormat("frameTime: %.3f ms", RLGetFrameTime()*1000.0f), tx, ty, 16, RAYWHITE); ty += 20;

        if (showHelp) DrawOverlayHelp(canvasX + 10, canvasY + canvasH - 80);

        RLDrawFPS(px + panelW - 90, py + 10);

        RLEndDrawing();
    }

    if (gCanvas.id != 0) RLUnloadRenderTexture(gCanvas);

    RLCloseWindow();
    return 0;
}
