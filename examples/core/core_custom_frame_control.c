/*******************************************************************************************
*
*   raylib [core] example - custom frame control
*
*   Example complexity rating: [★★★★] 4/4
*
*   NOTE: WARNING: This is an example for advanced users willing to have full control over
*   the frame processes. By default, EndDrawing() calls the following processes:
*       1. Draw remaining batch data: rlDrawRenderBatchActive()
*       2. SwapScreenBuffer()
*       3. Frame time control: WaitTime()
*       4. PollInputEvents()
*
*   To avoid steps 2, 3 and 4, flag SUPPORT_CUSTOM_FRAME_CONTROL can be enabled in
*   config.h (it requires recompiling raylib). This way those steps are up to the user
*
*   Note that enabling this flag invalidates some functions:
*       - GetFrameTime()
*       - SetTargetFPS()
*       - GetFPS()
*
*   Example originally created with raylib 4.0, last time updated with raylib 4.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2021-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - custom frame control");

    // Custom timming variables
    double previousTime = RLGetTime();    // Previous time measure
    double currentTime = 0.0;           // Current time measure
    double updateDrawTime = 0.0;        // Update + Draw time
    double waitTime = 0.0;              // Wait time (if target fps required)
    float deltaTime = 0.0f;             // Frame time (Update + Draw + Wait time)

    float timeCounter = 0.0f;           // Accumulative time counter (seconds)
    float position = 0.0f;              // Circle position
    bool pause = false;                 // Pause control flag

    int targetFPS = 60;                 // Our initial target fps
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        #ifndef PLATFORM_WEB            // NOTE: On non web platforms the PollInputEvents just works before the inputs checks
            RLPollInputEvents();          // Poll input events (SUPPORT_CUSTOM_FRAME_CONTROL)
        #endif

        if (RLIsKeyPressed(KEY_SPACE)) pause = !pause;

        if (RLIsKeyPressed(KEY_UP)) targetFPS += 20;
        else if (RLIsKeyPressed(KEY_DOWN)) targetFPS -= 20;

        if (targetFPS < 0) targetFPS = 0;

        if (!pause)
        {
            position += 200*deltaTime;  // We move at 200 pixels per second
            if (position >= RLGetScreenWidth()) position = 0;
            timeCounter += deltaTime;   // We count time (seconds)
        }

        #ifdef PLATFORM_WEB             // NOTE: On web platform for some reason the PollInputEvents only works after the inputs check, so just call it after check all your inputs (on web)
            RLPollInputEvents();          // Poll input events (SUPPORT_CUSTOM_FRAME_CONTROL)
        #endif
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            for (int i = 0; i < RLGetScreenWidth()/200; i++) RLDrawRectangle(200*i, 0, 1, RLGetScreenHeight(), SKYBLUE);

            RLDrawCircle((int)position, RLGetScreenHeight()/2 - 25, 50, RED);

            RLDrawText(RLTextFormat("%03.0f ms", timeCounter*1000.0f), (int)position - 40, RLGetScreenHeight()/2 - 100, 20, MAROON);
            RLDrawText(RLTextFormat("PosX: %03.0f", position), (int)position - 50, RLGetScreenHeight()/2 + 40, 20, BLACK);

            RLDrawText("Circle is moving at a constant 200 pixels/sec,\nindependently of the frame rate.", 10, 10, 20, DARKGRAY);
            RLDrawText("PRESS SPACE to PAUSE MOVEMENT", 10, RLGetScreenHeight() - 60, 20, GRAY);
            RLDrawText("PRESS UP | DOWN to CHANGE TARGET FPS", 10, RLGetScreenHeight() - 30, 20, GRAY);
            RLDrawText(RLTextFormat("TARGET FPS: %i", targetFPS), RLGetScreenWidth() - 220, 10, 20, LIME);
            if (deltaTime != 0)
            {
                RLDrawText(RLTextFormat("CURRENT FPS: %i", (int)(1.0f/deltaTime)), RLGetScreenWidth() - 220, 40, 20, GREEN);
            }

        RLEndDrawing();

        // NOTE: In case raylib is configured to SUPPORT_CUSTOM_FRAME_CONTROL,
        // Events polling, screen buffer swap and frame time control must be managed by the user

        RLSwapScreenBuffer();         // Flip the back buffer to screen (front buffer)

        currentTime = RLGetTime();
        updateDrawTime = currentTime - previousTime;

        if (targetFPS > 0)          // We want a fixed frame rate
        {
            waitTime = (1.0f/(float)targetFPS) - updateDrawTime;
            if (waitTime > 0.0)
            {
                RLWaitTime((float)waitTime);
                currentTime = RLGetTime();
                deltaTime = (float)(currentTime - previousTime);
            }
        }
        else deltaTime = (float)updateDrawTime;    // Framerate could be variable

        previousTime = currentTime;
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
