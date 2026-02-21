/*******************************************************************************************
*
*   raylib [core] example - window should close
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 4.2, last time updated with raylib 4.2
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2013-2025 Ramon Santamaria (@raysan5)
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

    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - window should close");

    RLSetExitKey(RL_E_KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works

    bool exitWindowRequested = false;   // Flag to request window to exit
    bool exitWindow = false;    // Flag to set window to exit

    RLSetTargetFPS(60);           // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!exitWindow)
    {
        // Update
        //----------------------------------------------------------------------------------
        // Detect if X-button or KEY_ESCAPE have been pressed to close window
        if (RLWindowShouldClose() || RLIsKeyPressed(RL_E_KEY_ESCAPE)) exitWindowRequested = true;

        if (exitWindowRequested)
        {
            // A request for close window has been issued, we can save data before closing
            // or just show a message asking for confirmation

            if (RLIsKeyPressed(RL_E_KEY_Y)) exitWindow = true;
            else if (RLIsKeyPressed(RL_E_KEY_N)) exitWindowRequested = false;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            if (exitWindowRequested)
            {
                RLDrawRectangle(0, 100, screenWidth, 200, BLACK);
                RLDrawText("Are you sure you want to exit program? [Y/N]", 40, 180, 30, WHITE);
            }
            else RLDrawText("Try to close the window to get confirmation message!", 120, 200, 20, LIGHTGRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
