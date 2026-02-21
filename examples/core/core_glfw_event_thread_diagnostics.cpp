/*******************************************************************************************
*
*   raylib [core] example - GLFW event-thread diagnostics (Route2 multi-window)
*
*   This is a C++ example (requires examples CMake to include *.cpp and CXX).
*
*   It verifies:
*     - FLAG_WINDOW_EVENT_THREAD: Win32 event loop runs on a dedicated event thread
*     - Per-window render loops can block on event-waiting/minimized pause
*     - Primary window close triggers global quit + wake so a minimized secondary window can exit
*     - Drag/resize/menu modal loops do NOT stall rendering (event thread owns the modal loop)
*
*   Controls (focus main window A):
*     M: request secondary window B to Minimize
*     R: request secondary window B to Restore
*     W: toggle B event-waiting (Enable/DisableEventWaiting)
*     B: toggle B borderless windowed
*     Esc / close: exit (closing primary A should exit B without deadlock)
*
********************************************************************************************/

#include "raylib.h"
#include "rl_context.h"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <thread>

#if defined(_WIN32)
    #include <windows.h>
    #include <process.h>
#endif

// Simple command channel from A -> B (so B executes window API on its own thread/context).
enum class BCmd : int
{
    None = 0,
    Minimize,
    Restore,
    ToggleWait,
    ToggleBorderless,
    Quit
};

static std::atomic<int> gQuitRequested{0};
static std::atomic<BCmd> gBCmd{BCmd::None};

static std::atomic<unsigned long> gAThreadId{0};
static std::atomic<unsigned long> gBThreadId{0};

static void RequestQuit() { gQuitRequested.store(1, std::memory_order_release); }
static bool QuitRequested() { return gQuitRequested.load(std::memory_order_acquire) != 0; }

static unsigned __stdcall SecondaryWindowThread(void* /*arg*/)
{
#if defined(_WIN32)
    gBThreadId.store((unsigned long)GetCurrentThreadId(), std::memory_order_relaxed);
#endif

    RLContext* ctx = RLCreateContext();
    RLSetCurrentContext(ctx);

    // Secondary window uses event-thread mode.
    RLSetConfigFlags(RL_E_FLAG_WINDOW_RESIZABLE | RL_E_FLAG_WINDOW_EVENT_THREAD);
    RLInitWindow(680, 370, "raylib [event-thread] secondary window B");
    RLSetTargetFPS(60);

    bool waitEnabled = false;

    while (!QuitRequested() && !RLWindowShouldClose())
    {
        // Consume command from A.
        BCmd cmd = gBCmd.exchange(BCmd::None, std::memory_order_acq_rel);
        switch (cmd)
        {
            case BCmd::Minimize: RLMinimizeWindow(); break;
            case BCmd::Restore: RLRestoreWindow(); break;
            case BCmd::ToggleWait:
                waitEnabled = !waitEnabled;
                if (waitEnabled) RLEnableEventWaiting();
                else RLDisableEventWaiting();
                break;
            case BCmd::ToggleBorderless:
                RLToggleBorderlessWindowed();
                break;
            case BCmd::Quit:
                RequestQuit();
                break;
            default: break;
        }

        RLBeginDrawing();
        RLClearBackground(RLColor{ 30, 30, 30, 255 });

        RLDrawText("Secondary window B (thread)", 20, 20, 20, RAYWHITE);
        RLDrawText(RLTextFormat("waitEnabled=%d (event-waiting)", waitEnabled), 20, 48, 16, LIGHTGRAY);
        RLDrawText(RLTextFormat("minimized=%d focused=%d", RLIsWindowMinimized(), RLIsWindowFocused()), 20, 70, 16, LIGHTGRAY);

#if defined(_WIN32)
        RLDrawText(RLTextFormat("B render TID=%lu", (unsigned long)GetCurrentThreadId()), 20, 92, 16, LIGHTGRAY);
#endif

        RLDrawText("Test: Minimize B + enable waiting; then close primary A -> exit without deadlock.", 20, 140, 16, LIGHTGRAY);
        RLDrawText("Also test: drag/resize A or open system menu; animation should keep running.", 20, 162, 16, LIGHTGRAY);
        RLDrawCircle(360, 240, 60, RLColor{ 80, 160, 255, 255 });
        RLEndDrawing();
    }

    RLCloseWindow();
    RLDestroyContext(ctx);
    return 0;
}

int main(void)
{
#if defined(_WIN32)
    gAThreadId.store((unsigned long)GetCurrentThreadId(), std::memory_order_relaxed);
#endif

    // Primary window A uses event-thread mode too.
    RLSetConfigFlags(RL_E_FLAG_MSAA_4X_HINT | RL_E_FLAG_WINDOW_RESIZABLE | RL_E_FLAG_WINDOW_EVENT_THREAD);
    RLInitWindow(800, 450, "raylib [event-thread] primary window A");
    RLSetTargetFPS(60);

    // Start B.
#if defined(_WIN32)
    HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, SecondaryWindowThread, NULL, 0, NULL);
#else
    std::thread th([](){ SecondaryWindowThread(nullptr); });
#endif

    bool showHelp = true;

    while (!QuitRequested() && !RLWindowShouldClose())
    {
        if (RLIsKeyPressed(RL_E_KEY_H)) showHelp = !showHelp;

        if (RLIsKeyPressed(RL_E_KEY_M)) gBCmd.store(BCmd::Minimize, std::memory_order_release);
        if (RLIsKeyPressed(RL_E_KEY_R)) gBCmd.store(BCmd::Restore, std::memory_order_release);
        if (RLIsKeyPressed(RL_E_KEY_W)) gBCmd.store(BCmd::ToggleWait, std::memory_order_release);
        if (RLIsKeyPressed(RL_E_KEY_B)) gBCmd.store(BCmd::ToggleBorderless, std::memory_order_release);

        RLBeginDrawing();
        RLClearBackground(RAYWHITE);

        RLDrawText("Primary window A (main thread)", 20, 20, 22, BLACK);
        RLDrawText("Press M(minimize B), R(restore B), W(toggle B waiting), B(toggle B borderless), H(help)", 20, 52, 16, DARKGRAY);

#if defined(_WIN32)
        RLDrawText(RLTextFormat("A render TID=%lu", (unsigned long)GetCurrentThreadId()), 20, 80, 16, DARKGRAY);
        RLDrawText(RLTextFormat("B render TID=%lu", gBThreadId.load()), 20, 100, 16, DARKGRAY);
#endif

        if (showHelp)
        {
            RLDrawRectangle(20, 180, 760, 220, RLFade(BLACK, 0.05f));
            RLDrawRectangleLines(20, 180, 760, 220, RLFade(BLACK, 0.15f));
            RLDrawText("Expected:", 30, 190, 18, BLACK);
            RLDrawText("  - Drag/resize A or open its system menu: animation should keep running.", 30, 214, 16, DARKGRAY);
            RLDrawText("  - Minimize B + enable waiting; then close A -> process exits without deadlock.", 30, 234, 16, DARKGRAY);
            RLDrawText("  - If animation stalls during modal loops, event-thread routing is broken.", 30, 254, 16, DARKGRAY);
        }

        // A small animation so you can see A is alive.
        double t = RLGetTime();
        int x = 20 + (int)(720.0 * (0.5 + 0.5 * std::sin(t * 1.2)));
        RLDrawCircle(x, 430, 8, BLUE);

        RLEndDrawing();
    }

    // Signal B to exit.
    RequestQuit();
    gBCmd.store(BCmd::Quit, std::memory_order_release);

#if defined(_WIN32)
    if (hThread)
    {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        hThread = NULL;
    }
#else
    if (th.joinable()) th.join();
#endif

    RLCloseWindow();
    return 0;
}
