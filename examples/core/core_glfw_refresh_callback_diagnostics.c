/*******************************************************************************************
*
*   raylib [core] example - GLFW refresh-callback (Win32 modal loop) diagnostics
*
*   This example targets the custom RL-prefixed raylib fork.
*
*   It verifies:
*     - FLAG_WINDOW_REFRESH_CALLBACK: OS-driven refresh ticks during Win32 modal loops
*     - RLSetWindowRefreshCallback(): user callback invoked with a valid GL context
*     - A set of window/input APIs that depend on GLFW message processing
*
*   Notes:
*     - The refresh callback is invoked from inside Win32 modal loops (move/size/menu tracking).
*       It must be fast and must NOT run its own event loop.
*     - In this fork, the refresh callback is wrapped by raylib with BeginDrawing/EndDrawing.
*       Therefore the callback should only issue draw calls (no RLBeginDrawing/RLEndDrawing).
*
********************************************************************************************/

#include "raylib.h"

#include <math.h>
#include <stdio.h>

#if defined(_WIN32)
    #include <windows.h>
#endif

static int gRefreshCount = 0;
static double gLastRefreshTime = 0.0;
static int gScripted = 0;
static int gScriptStep = 0;
static double gNextScriptTime = 0.0;
static int gShowHelp = 1;

static void DrawHud(const char* modeLabel)
{
    const int sw = RLGetScreenWidth();
    const int sh = RLGetScreenHeight();

    RLDrawRectangle(10, 10, sw - 20, 150, RLFade(BLACK, 0.55f));
    RLDrawRectangleLines(10, 10, sw - 20, 150, RLFade(WHITE, 0.6f));

    RLDrawText(modeLabel, 20, 18, 20, RAYWHITE);

    RLDrawText(RLTextFormat("refreshCount=%d  lastRefresh=%.3fs", gRefreshCount, gLastRefreshTime), 20, 45, 18, RAYWHITE);
    RLDrawText(RLTextFormat("window: %dx%d  render: %dx%d", RLGetScreenWidth(), RLGetScreenHeight(), RLGetRenderWidth(), RLGetRenderHeight()), 20, 68, 18, RAYWHITE);

    RLDrawText(RLTextFormat("focused=%d minimized=%d maximized=%d fullscreen=%d borderless=%d", 
        RLIsWindowFocused(), RLIsWindowMinimized(), RLIsWindowMaximized(), RLIsWindowFullscreen(), RLIsWindowState(FLAG_WINDOW_UNDECORATED)),
        20, 91, 18, RAYWHITE);

#if defined(_WIN32)
    RLDrawText(RLTextFormat("TID=%lu", (unsigned long)GetCurrentThreadId()), 20, 114, 18, RAYWHITE);
#endif

    if (gShowHelp)
    {
        RLDrawRectangle(10, sh - 190, sw - 20, 180, RLFade(BLACK, 0.55f));
        RLDrawRectangleLines(10, sh - 190, sw - 20, 180, RLFade(WHITE, 0.6f));

        int y = sh - 182;
        RLDrawText("Keys:", 20, y, 18, RAYWHITE); y += 22;
        RLDrawText("  H   toggle help", 20, y, 18, RAYWHITE); y += 20;
        RLDrawText("  F5  start/stop scripted window API exercise", 20, y, 18, RAYWHITE); y += 20;
        RLDrawText("  1   SetWindowTitle", 20, y, 18, RAYWHITE); y += 20;
        RLDrawText("  2   SetWindowSize (cycle)", 20, y, 18, RAYWHITE); y += 20;
        RLDrawText("  3   SetWindowPosition (cycle)", 20, y, 18, RAYWHITE); y += 20;
        RLDrawText("  4   Minimize  5 Restore  6 Maximize", 20, y, 18, RAYWHITE); y += 20;
        RLDrawText("  7   ToggleFullscreen  8 ToggleBorderlessWindowed", 20, y, 18, RAYWHITE); y += 20;
        RLDrawText("  9   SetWindowOpacity (cycle)", 20, y, 18, RAYWHITE); y += 20;
        RLDrawText("  C   Clipboard write/read test", 20, y, 18, RAYWHITE); y += 20;
#if defined(_WIN32)
        RLDrawText("Mouse:", 20, y, 18, RAYWHITE); y += 22;
        RLDrawText("  Right-click: open Win32 popup menu (forces menu modal loop)", 20, y, 18, RAYWHITE); y += 20;
#endif
    }
}

#if defined(_WIN32)
static void Win32_ShowPopupMenu(void)
{
    HWND hwnd = (HWND)RLGetWindowHandle();
    if (!hwnd) return;

    HMENU menu = CreatePopupMenu();
    AppendMenuA(menu, MF_STRING, 1, "Menu Item 1");
    AppendMenuA(menu, MF_STRING, 2, "Menu Item 2");
    AppendMenuA(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(menu, MF_STRING, 3, "Close Menu");

    POINT p;
    GetCursorPos(&p);

    // Enters Win32 menu tracking modal loop.
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_LEFTALIGN, p.x, p.y, 0, hwnd, NULL);

    DestroyMenu(menu);
}
#endif

static void RunScriptStep(int step)
{
    // A small scripted sequence to exercise key GLFW-backed window APIs.
    // Keep it conservative to avoid disrupting debugging.
    switch (step)
    {
        case 0: RLSetWindowTitle("[diag] title step 0"); break;
        case 1: RLSetWindowSize(900, 520); break;
        case 2: RLSetWindowPosition(80, 80); break;
        case 3: RLSetWindowOpacity(0.85f); break;
        case 4: RLMaximizeWindow(); break;
        case 5: RLRestoreWindow(); break;
        case 6: RLMinimizeWindow(); break;
        case 7: RLRestoreWindow(); break;
        case 8: RLToggleBorderlessWindowed(); break;
        case 9: RLToggleBorderlessWindowed(); break;
        default: break;
    }
}

static void OnRefreshDraw(void)
{
    // This callback is wrapped by raylib with RLBeginDrawing/RLEndDrawing.
    gRefreshCount++;
    gLastRefreshTime = RLGetTime();

    const double t = RLGetTime();
    const int x = 20 + (int)(10.0 * sin(t * 6.28318530718));

    RLClearBackground(RAYWHITE);
    RLDrawText("[RefreshCallback] Modal-loop repaint tick", x, 170, 22, RED);
    RLDrawText(RLTextFormat("refreshCount=%d", gRefreshCount), x, 200, 20, DARKGRAY);

    // Draw a small animated bar to prove continuous refresh.
    int barW = (int)(200 + 150 * (0.5 + 0.5 * sin(t * 3.0)));
    RLDrawRectangle(20, 240, barW, 14, (RLColor){ 200, 40, 40, 255 });
    RLDrawRectangleLines(20, 240, 360, 14, RLFade(BLACK, 0.5f));

    DrawHud("Mode: FLAG_WINDOW_REFRESH_CALLBACK (non-event-thread)");
}

int main(void)
{
    RLSetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_REFRESH_CALLBACK | FLAG_WINDOW_RESIZABLE);
    RLInitWindow(800, 450, "raylib [core] refresh callback diagnostics");

    // Register refresh callback (invoked during Win32 modal loops).
    RLSetWindowRefreshCallback(OnRefreshDraw);

    RLSetTargetFPS(60);

    int sizeIdx = 0;
    int posIdx = 0;
    int opacityIdx = 0;
    const float opacities[] = { 1.0f, 0.9f, 0.75f, 0.6f };

    while (!RLWindowShouldClose())
    {
        // Controls
        if (RLIsKeyPressed(KEY_H)) gShowHelp = !gShowHelp;
        if (RLIsKeyPressed(KEY_F5))
        {
            gScripted = !gScripted;
            gScriptStep = 0;
            gNextScriptTime = RLGetTime() + 0.5;
            printf("[diag] scripted=%d\n", gScripted);
        }

        if (gScripted && (RLGetTime() >= gNextScriptTime))
        {
            RunScriptStep(gScriptStep);
            printf("[diag] scripted step %d\n", gScriptStep);
            gScriptStep++;
            if (gScriptStep > 9) { gScripted = 0; }
            gNextScriptTime = RLGetTime() + 0.8;
        }

        if (RLIsKeyPressed(KEY_ONE)) RLSetWindowTitle("[diag] title via key 1");
        if (RLIsKeyPressed(KEY_TWO))
        {
            static const int sizes[][2] = { {800,450},{1024,576},{640,360},{900,520} };
            sizeIdx = (sizeIdx + 1) % 4;
            RLSetWindowSize(sizes[sizeIdx][0], sizes[sizeIdx][1]);
        }
        if (RLIsKeyPressed(KEY_THREE))
        {
            static const int pos[][2] = { {40,40},{200,120},{520,120},{120,240} };
            posIdx = (posIdx + 1) % 4;
            RLSetWindowPosition(pos[posIdx][0], pos[posIdx][1]);
        }
        if (RLIsKeyPressed(KEY_FOUR)) RLMinimizeWindow();
        if (RLIsKeyPressed(KEY_FIVE)) RLRestoreWindow();
        if (RLIsKeyPressed(KEY_SIX)) RLMaximizeWindow();
        if (RLIsKeyPressed(KEY_SEVEN)) RLToggleFullscreen();
        if (RLIsKeyPressed(KEY_EIGHT)) RLToggleBorderlessWindowed();
        if (RLIsKeyPressed(KEY_NINE))
        {
            opacityIdx = (opacityIdx + 1) % (int)(sizeof(opacities)/sizeof(opacities[0]));
            RLSetWindowOpacity(opacities[opacityIdx]);
        }

        if (RLIsKeyPressed(KEY_C))
        {
            RLSetClipboardText("[diag] clipboard set by core_glfw_refresh_callback_diagnostics");
            printf("[diag] clipboard now: %s\n", RLGetClipboardText());
        }

#if defined(_WIN32)
        if (RLIsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
        {
            Win32_ShowPopupMenu();
        }
#endif

        // Normal frame (not in modal loop)
        RLBeginDrawing();
        RLClearBackground(RAYWHITE);

        DrawHud("Mode: FLAG_WINDOW_REFRESH_CALLBACK (non-event-thread)");

        RLDrawText("Try resizing/moving the window or open the popup menu.", 20, 190, 20, BLACK);
        RLDrawText("During Win32 modal loops, the refresh callback should keep repainting.", 20, 215, 20, DARKGRAY);

        // Draw a moving dot so you can see normal loop animation.
        double t = RLGetTime();
        int x = 20 + (int)(300.0 * (0.5 + 0.5 * sin(t * 2.0)));
        RLDrawCircle(x, 260, 10, BLUE);

        RLEndDrawing();
    }

    RLCloseWindow();
    return 0;
}
