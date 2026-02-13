/*******************************************************************************************
*
*   raylib [core] example - window flags
*
*   Example complexity rating: [★★★☆] 3/4
*
*   Example originally created with raylib 3.5, last time updated with raylib 3.5
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2020-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //---------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    // Possible window flags
    /*
    FLAG_VSYNC_HINT
    FLAG_FULLSCREEN_MODE    -> not working properly -> wrong scaling!
    FLAG_WINDOW_RESIZABLE
    FLAG_WINDOW_UNDECORATED
    FLAG_WINDOW_TRANSPARENT
    FLAG_WINDOW_HIDDEN
    FLAG_WINDOW_MINIMIZED   -> Not supported on window creation
    FLAG_WINDOW_MAXIMIZED   -> Not supported on window creation
    FLAG_WINDOW_UNFOCUSED
    FLAG_WINDOW_TOPMOST
    FLAG_WINDOW_HIGHDPI     -> errors after minimize-resize, fb size is recalculated
    FLAG_WINDOW_ALWAYS_RUN
    FLAG_MSAA_4X_HINT
    */

    // Set configuration flags for window creation
    //SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);// | FLAG_WINDOW_TRANSPARENT);
    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - window flags");

    RLVector2 ballPosition = { RLGetScreenWidth()/2.0f, RLGetScreenHeight()/2.0f };
    RLVector2 ballSpeed = { 5.0f, 4.0f };
    float ballRadius = 20;

    int framesCounter = 0;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //----------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //-----------------------------------------------------
        if (RLIsKeyPressed(KEY_F)) RLToggleFullscreen();  // modifies window size when scaling!

        if (RLIsKeyPressed(KEY_R))
        {
            if (RLIsWindowState(FLAG_WINDOW_RESIZABLE)) RLClearWindowState(FLAG_WINDOW_RESIZABLE);
            else RLSetWindowState(FLAG_WINDOW_RESIZABLE);
        }

        if (RLIsKeyPressed(KEY_D))
        {
            if (RLIsWindowState(FLAG_WINDOW_UNDECORATED)) RLClearWindowState(FLAG_WINDOW_UNDECORATED);
            else RLSetWindowState(FLAG_WINDOW_UNDECORATED);
        }

        if (RLIsKeyPressed(KEY_H))
        {
            if (!RLIsWindowState(FLAG_WINDOW_HIDDEN)) RLSetWindowState(FLAG_WINDOW_HIDDEN);

            framesCounter = 0;
        }

        if (RLIsWindowState(FLAG_WINDOW_HIDDEN))
        {
            framesCounter++;
            if (framesCounter >= 240) RLClearWindowState(FLAG_WINDOW_HIDDEN); // Show window after 3 seconds
        }

        if (RLIsKeyPressed(KEY_N))
        {
            if (!RLIsWindowState(FLAG_WINDOW_MINIMIZED)) RLMinimizeWindow();

            framesCounter = 0;
        }

        if (RLIsWindowState(FLAG_WINDOW_MINIMIZED))
        {
            framesCounter++;
            if (framesCounter >= 240)
            {
                RLRestoreWindow(); // Restore window after 3 seconds
                framesCounter = 0;
            }
        }

        if (RLIsKeyPressed(KEY_M))
        {
            // NOTE: Requires FLAG_WINDOW_RESIZABLE enabled!
            if (RLIsWindowState(FLAG_WINDOW_MAXIMIZED)) RLRestoreWindow();
            else RLMaximizeWindow();
        }

        if (RLIsKeyPressed(KEY_U))
        {
            if (RLIsWindowState(FLAG_WINDOW_UNFOCUSED)) RLClearWindowState(FLAG_WINDOW_UNFOCUSED);
            else RLSetWindowState(FLAG_WINDOW_UNFOCUSED);
        }

        if (RLIsKeyPressed(KEY_T))
        {
            if (RLIsWindowState(FLAG_WINDOW_TOPMOST)) RLClearWindowState(FLAG_WINDOW_TOPMOST);
            else RLSetWindowState(FLAG_WINDOW_TOPMOST);
        }

        if (RLIsKeyPressed(KEY_A))
        {
            if (RLIsWindowState(FLAG_WINDOW_ALWAYS_RUN)) RLClearWindowState(FLAG_WINDOW_ALWAYS_RUN);
            else RLSetWindowState(FLAG_WINDOW_ALWAYS_RUN);
        }

        if (RLIsKeyPressed(KEY_V))
        {
            if (RLIsWindowState(FLAG_VSYNC_HINT)) RLClearWindowState(FLAG_VSYNC_HINT);
            else RLSetWindowState(FLAG_VSYNC_HINT);
        }

        if (RLIsKeyPressed(KEY_B)) RLToggleBorderlessWindowed();


        // Bouncing ball logic
        ballPosition.x += ballSpeed.x;
        ballPosition.y += ballSpeed.y;
        if ((ballPosition.x >= (RLGetScreenWidth() - ballRadius)) || (ballPosition.x <= ballRadius)) ballSpeed.x *= -1.0f;
        if ((ballPosition.y >= (RLGetScreenHeight() - ballRadius)) || (ballPosition.y <= ballRadius)) ballSpeed.y *= -1.0f;
        //-----------------------------------------------------

        // Draw
        //-----------------------------------------------------
        RLBeginDrawing();

        if (RLIsWindowState(FLAG_WINDOW_TRANSPARENT)) RLClearBackground(BLANK);
        else RLClearBackground(RAYWHITE);

        RLDrawCircleV(ballPosition, ballRadius, MAROON);
        RLDrawRectangleLinesEx((RLRectangle) { 0, 0, (float)RLGetScreenWidth(), (float)RLGetScreenHeight() }, 4, RAYWHITE);

        RLDrawCircleV(RLGetMousePosition(), 10, DARKBLUE);

        RLDrawFPS(10, 10);

        RLDrawText(RLTextFormat("Screen Size: [%i, %i]", RLGetScreenWidth(), RLGetScreenHeight()), 10, 40, 10, GREEN);

        // Draw window state info
        RLDrawText("Following flags can be set after window creation:", 10, 60, 10, GRAY);
        if (RLIsWindowState(FLAG_FULLSCREEN_MODE)) RLDrawText("[F] FLAG_FULLSCREEN_MODE: on", 10, 80, 10, LIME);
        else RLDrawText("[F] FLAG_FULLSCREEN_MODE: off", 10, 80, 10, MAROON);
        if (RLIsWindowState(FLAG_WINDOW_RESIZABLE)) RLDrawText("[R] FLAG_WINDOW_RESIZABLE: on", 10, 100, 10, LIME);
        else RLDrawText("[R] FLAG_WINDOW_RESIZABLE: off", 10, 100, 10, MAROON);
        if (RLIsWindowState(FLAG_WINDOW_UNDECORATED)) RLDrawText("[D] FLAG_WINDOW_UNDECORATED: on", 10, 120, 10, LIME);
        else RLDrawText("[D] FLAG_WINDOW_UNDECORATED: off", 10, 120, 10, MAROON);
        if (RLIsWindowState(FLAG_WINDOW_HIDDEN)) RLDrawText("[H] FLAG_WINDOW_HIDDEN: on", 10, 140, 10, LIME);
        else RLDrawText("[H] FLAG_WINDOW_HIDDEN: off (hides for 3 seconds)", 10, 140, 10, MAROON);
        if (RLIsWindowState(FLAG_WINDOW_MINIMIZED)) RLDrawText("[N] FLAG_WINDOW_MINIMIZED: on", 10, 160, 10, LIME);
        else RLDrawText("[N] FLAG_WINDOW_MINIMIZED: off (restores after 3 seconds)", 10, 160, 10, MAROON);
        if (RLIsWindowState(FLAG_WINDOW_MAXIMIZED)) RLDrawText("[M] FLAG_WINDOW_MAXIMIZED: on", 10, 180, 10, LIME);
        else RLDrawText("[M] FLAG_WINDOW_MAXIMIZED: off", 10, 180, 10, MAROON);
        if (RLIsWindowState(FLAG_WINDOW_UNFOCUSED)) RLDrawText("[G] FLAG_WINDOW_UNFOCUSED: on", 10, 200, 10, LIME);
        else RLDrawText("[U] FLAG_WINDOW_UNFOCUSED: off", 10, 200, 10, MAROON);
        if (RLIsWindowState(FLAG_WINDOW_TOPMOST)) RLDrawText("[T] FLAG_WINDOW_TOPMOST: on", 10, 220, 10, LIME);
        else RLDrawText("[T] FLAG_WINDOW_TOPMOST: off", 10, 220, 10, MAROON);
        if (RLIsWindowState(FLAG_WINDOW_ALWAYS_RUN)) RLDrawText("[A] FLAG_WINDOW_ALWAYS_RUN: on", 10, 240, 10, LIME);
        else RLDrawText("[A] FLAG_WINDOW_ALWAYS_RUN: off", 10, 240, 10, MAROON);
        if (RLIsWindowState(FLAG_VSYNC_HINT)) RLDrawText("[V] FLAG_VSYNC_HINT: on", 10, 260, 10, LIME);
        else RLDrawText("[V] FLAG_VSYNC_HINT: off", 10, 260, 10, MAROON);
        if (RLIsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) RLDrawText("[B] FLAG_BORDERLESS_WINDOWED_MODE: on", 10, 280, 10, LIME);
        else RLDrawText("[B] FLAG_BORDERLESS_WINDOWED_MODE: off", 10, 280, 10, MAROON);

        RLDrawText("Following flags can only be set before window creation:", 10, 320, 10, GRAY);
        if (RLIsWindowState(FLAG_WINDOW_HIGHDPI)) RLDrawText("FLAG_WINDOW_HIGHDPI: on", 10, 340, 10, LIME);
        else RLDrawText("FLAG_WINDOW_HIGHDPI: off", 10, 340, 10, MAROON);
        if (RLIsWindowState(FLAG_WINDOW_TRANSPARENT)) RLDrawText("FLAG_WINDOW_TRANSPARENT: on", 10, 360, 10, LIME);
        else RLDrawText("FLAG_WINDOW_TRANSPARENT: off", 10, 360, 10, MAROON);
        if (RLIsWindowState(FLAG_MSAA_4X_HINT)) RLDrawText("FLAG_MSAA_4X_HINT: on", 10, 380, 10, LIME);
        else RLDrawText("FLAG_MSAA_4X_HINT: off", 10, 380, 10, MAROON);

        RLEndDrawing();
        //-----------------------------------------------------
    }

    // De-Initialization
    //---------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //----------------------------------------------------------

    return 0;
}
