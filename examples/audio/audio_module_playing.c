/*******************************************************************************************
*
*   raylib [audio] example - module playing
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 1.5, last time updated with raylib 3.5
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2016-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#if defined(_WIN32)
#include <windows.h>
#include <process.h>     // _beginthreadex
#endif
#include <GLFW/glfw3.h>
#include <rl_context.h>

#define MAX_CIRCLES  64

typedef struct {
    RLVector2 position;
    float radius;
    float alpha;
    float speed;
    RLColor color;
} CircleWave;

#if defined(_WIN32)
// NOTE: Route2 Stage-A multi-window test: one window per thread.
// Each thread must set its own RLContext as current before calling any raylib API.
static volatile LONG gQuitRequested = 0;

static int QuitRequested(void) { return (InterlockedCompareExchange(&gQuitRequested, 0, 0) != 0); }
static void RequestQuit(void) { InterlockedExchange(&gQuitRequested, 1); }

// Win32-specific per-thread event-wakeup and task-dispatch context
//
typedef struct _GLFWwin32ThreadTask
{
    void (*fn)(void* user);
    void* user;
    struct _GLFWwin32ThreadTask* next;
} _GLFWwin32ThreadTask;

struct GLFWthread
{
    DWORD                 tid;
    HANDLE                wakeEvent;
    HWND                  dispatchWindow;
    CRITICAL_SECTION       tasksLock;
    _GLFWwin32ThreadTask* tasksHead;
    _GLFWwin32ThreadTask* tasksTail;
    struct GLFWthread* next;
};

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct {
    GLFWwindow* handle;                 // GLFW window handle (graphic device)
    struct GLFWthread* mainThread;             // [Win32] Owner thread (FLAG_WINDOW_EVENT_THREAD scaffolding)
} Win32PlatformData;


static int MyHook1(void* hwnd, unsigned int msg, uintptr_t wp, intptr_t lp, intptr_t* result, void* user)
{
    (void)hwnd; (void)wp; (void)lp; (void)user;

    if (msg == WM_NCHITTEST) { *result = HTCAPTION; return 1; }
    return 0;
}

static int MyHook2(void* hwnd, unsigned int msg, uintptr_t wp, intptr_t lp, intptr_t* result, void* user) {
    (void)hwnd; (void)msg; (void)wp; (void)lp; (void)result; (void)user;
    RLTraceLog(RL_E_LOG_INFO, "MyHook2: %p, %d, %p", hwnd, msg, wp, lp, result, user);
    return 0;
}

static unsigned __stdcall OtherThread(void* arg)
{
    int nRelNumber = RLWin32GetAllWindowHandles(NULL, -1);

    RLTraceLog(RL_E_LOG_INFO, "Win32 Window Handle number: %d.", nRelNumber);

    void* hwnds[32];
    int n = RLWin32GetAllWindowHandles(hwnds, 32);

    for (int i = 0; i < n; i++) {
        void* hwnd = hwnds[i];
        RLWin32SetWindowPropByHandle(hwnd, "MyTag", (void*)0x1234);
        RLTraceLog(RL_E_LOG_INFO, "Win32 Window Handle %d: %p.", i, hwnd);
    }

    void* hwnd = RLWin32GetPrimaryWindowHandle();
    void* token = RLWin32AddMessageHookByHandle(hwnd, MyHook2, NULL);

    while (!QuitRequested())
    {
#if defined(_WIN32)
        Sleep(100);
#endif // WIN32

    }
    RLWin32RemoveMessageHookByHandle(hwnd, token);
}

static unsigned __stdcall SecondaryWindowThread(void* arg)
{
    (void)arg;

    RLContext* ctx = RLCreateContext();
    RLSetCurrentContext(ctx);

    RLSetConfigFlags(RL_E_FLAG_MSAA_4X_HINT | RL_E_FLAG_WINDOW_HIGHDPI | RL_E_FLAG_WINDOW_RESIZABLE | RL_E_FLAG_WINDOW_EVENT_THREAD);
    RLInitWindow(680, 370, "raylib [thread] secondary window");
    //RLSetWindowMaxSize(1000, 800);
    RLSetTargetFPS(60);

    RLWin32SetWindowProp("my.key", (void*)0x1234);
    void* v = RLWin32GetWindowProp("my.key");

    void* token = RLWin32AddMessageHook(MyHook1, NULL);

    Win32PlatformData* platformData = (Win32PlatformData*)ctx->platformData;
    //_GLFWwindow* window = (_GLFWwindow*)platformData->handle;

    while (!QuitRequested() && !RLWindowShouldClose())
    {
        RLBeginDrawing();
        RLClearBackground((RLColor) { 30, 30, 30, 255 });
        RLDrawText("Secondary window (thread)", 20, 20, 20, RAYWHITE);
        RLDrawText("Close this window or press ESC.", 20, 52, 10, LIGHTGRAY);
        RLDrawText(RLTextFormat("Window properties: 0x%p", v), 20, 72, 10, LIGHTGRAY);
        RLDrawCircle(360, 150, 60, (RLColor) { 80, 160, 255, 255 });
        RLEndDrawing();
    }

    // Signal main thread to exit too.
    //RequestQuit();

    RLWin32RemoveMessageHook(token);
    RLWin32RemoveWindowProp("my.key");
    RLCloseWindow();
    RLDestroyContext(ctx);
    return 0;
}
#endif  // _WIN32

static RLMusic gMusic;
static void OnRefreshDraw(void)
{
    double t = RLGetTime();
    int x = 20 + (int)(10.0 * sin(t * 6.283));
    RLUpdateMusicStream(gMusic);

    RLClearBackground(RAYWHITE);
    RLDrawText("Refreshing during modal loop...", x, 20, 20, RED);
}

static intptr_t DoRender(void* hwnd, void* user) {
    double t = RLGetTime();
    int x = 20 + (int)(10.0 * sin(t * 6.283));
    RLDrawText("Paint Command Invoked From Another Thread.", x, 320, 20, RED);
    return 1;
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLSetConfigFlags(RL_E_FLAG_MSAA_4X_HINT | RL_E_FLAG_WINDOW_REFRESH_CALLBACK);  // NOTE: Try to enable MSAA 4X

    RLSetWindowState(RL_E_FLAG_WINDOW_SNAP_LAYOUT);

    RLInitWindowEx(screenWidth, screenHeight, 
        "raylib [audio] example - module playing", "RLCustomWindowClass");

    //RLSetWindowState(RL_E_FLAG_WINDOW_SNAP_LAYOUT);

    RLInitAudioDevice();                  // Initialize audio device

    RLSetWindowRefreshCallback(OnRefreshDraw);

#if defined(_WIN32)
    HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, SecondaryWindowThread, NULL, 0, NULL);
    HANDLE hOtherThread = (HANDLE)_beginthreadex(NULL, 0, OtherThread, NULL, 0, NULL);
#endif
    RLColor colors[14] = { ORANGE, RED, GOLD, LIME, BLUE, VIOLET, BROWN, LIGHTGRAY, PINK,
                         YELLOW, GREEN, SKYBLUE, PURPLE, BEIGE };

    // Creates some circles for visual effect
    CircleWave circles[MAX_CIRCLES] = { 0 };

    for (int i = MAX_CIRCLES - 1; i >= 0; i--)
    {
        circles[i].alpha = 0.0f;
        circles[i].radius = (float)RLGetRandomValue(10, 40);
        circles[i].position.x = (float)RLGetRandomValue((int)circles[i].radius, (int)(screenWidth - circles[i].radius));
        circles[i].position.y = (float)RLGetRandomValue((int)circles[i].radius, (int)(screenHeight - circles[i].radius));
        circles[i].speed = (float)RLGetRandomValue(1, 100) / 2000.0f;
        circles[i].color = colors[RLGetRandomValue(0, 13)];
    }

    gMusic = RLLoadMusicStream("resources/mini1111.xm");
    gMusic.looping = false;
    float pitch = 1.0f;

    RLPlayMusicStream(gMusic);

    float timePlayed = 0.0f;
    bool pause = false;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    void* hwnds[32];

    // Main game loop
#if defined(_WIN32)
    while (!QuitRequested() && !RLWindowShouldClose())
#else
    while (!RLWindowShouldClose())
#endif    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateMusicStream(gMusic);      // Update music buffer with new stream data

        // Restart music playing (stop and play)
        if (RLIsKeyPressed(RL_E_KEY_SPACE))
        {
            RLStopMusicStream(gMusic);
            RLPlayMusicStream(gMusic);
            pause = false;
        }

        // Pause/Resume music playing
        if (RLIsKeyPressed(RL_E_KEY_P))
        {
            pause = !pause;

            if (pause) RLPauseMusicStream(gMusic);
            else RLResumeMusicStream(gMusic);
        }

        if (RLIsKeyDown(RL_E_KEY_DOWN)) pitch -= 0.01f;
        else if (RLIsKeyDown(RL_E_KEY_UP)) pitch += 0.01f;

        RLSetMusicPitch(gMusic, pitch);

        // Get timePlayed scaled to bar dimensions
        timePlayed = RLGetMusicTimePlayed(gMusic) / RLGetMusicTimeLength(gMusic) * (screenWidth - 40);

        // Color circles animation
        for (int i = MAX_CIRCLES - 1; (i >= 0) && !pause; i--)
        {
            circles[i].alpha += circles[i].speed;
            circles[i].radius += circles[i].speed * 10.0f;

            if (circles[i].alpha > 1.0f) circles[i].speed *= -1;

            if (circles[i].alpha <= 0.0f)
            {
                circles[i].alpha = 0.0f;
                circles[i].radius = (float)RLGetRandomValue(10, 40);
                circles[i].position.x = (float)RLGetRandomValue((int)circles[i].radius, (int)(screenWidth - circles[i].radius));
                circles[i].position.y = (float)RLGetRandomValue((int)circles[i].radius, (int)(screenHeight - circles[i].radius));
                circles[i].color = colors[RLGetRandomValue(0, 13)];
                circles[i].speed = (float)RLGetRandomValue(1, 100) / 2000.0f;
            }
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

        RLClearBackground(RAYWHITE);

        for (int i = MAX_CIRCLES - 1; i >= 0; i--)
        {
            RLDrawCircleV(circles[i].position, circles[i].radius, RLFade(circles[i].color, circles[i].alpha));
        }

        // Draw time bar
        RLDrawRectangle(20, screenHeight - 20 - 12, screenWidth - 40, 12, LIGHTGRAY);
        RLDrawRectangle(20, screenHeight - 20 - 12, (int)timePlayed, 12, MAROON);
        RLDrawRectangleLines(20, screenHeight - 20 - 12, screenWidth - 40, 12, GRAY);

        // Draw help instructions
        RLDrawRectangle(20, 20, 425, 145, WHITE);
        RLDrawRectangleLines(20, 20, 425, 145, GRAY);
        RLDrawText("PRESS SPACE TO RESTART MUSIC", 40, 40, 20, BLACK);
        RLDrawText("PRESS P TO PAUSE/RESUME", 40, 70, 20, BLACK);
        RLDrawText("PRESS UP/DOWN TO CHANGE SPEED", 40, 100, 20, BLACK);
        RLDrawText(RLTextFormat("SPEED: %f", pitch), 40, 130, 20, MAROON);

        RLEndDrawing();

        int nWindows = RLWin32GetAllWindowHandles(hwnds, 32);
        for (int i = 0; i < nWindows; i++) {
            void* hwnd = hwnds[i];
            if (hwnd == RLGetWindowHandle()) continue;
            RLInvokeOnWindowRenderThreadByHandle(hwnd, DoRender, NULL, 0);
        }
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
#if defined(_WIN32)
    // Stop secondary window thread first (it owns its own window/GL context).
    RequestQuit();
    if (hThread)
    {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        hThread = NULL;
    }
#endif

    RLUnloadMusicStream(gMusic);          // Unload music stream buffers from RAM

    RLCloseAudioDevice();     // Close audio device (music streaming is automatically stopped)

    RLCloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
