/*******************************************************************************************
*
*   raylib [core] example - highdpi testbed
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 5.6-dev, last time updated with raylib 5.6-dev
*
*   Example contributed by Ramon Santamaria (@raysan5) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Ramon Santamaria (@raysan5)
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

    RLSetConfigFlags(RL_E_FLAG_WINDOW_RESIZABLE | RL_E_FLAG_WINDOW_HIGHDPI);
    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - highdpi testbed");

    RLVector2 scaleDpi = RLGetWindowScaleDPI();
    RLVector2 mousePos = RLGetMousePosition();
    int currentMonitor = RLGetCurrentMonitor();
    RLVector2 windowPos = RLGetWindowPosition();

    int gridSpacing = 40;   // Grid spacing in pixels

    RLSetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        mousePos = RLGetMousePosition();
        currentMonitor = RLGetCurrentMonitor();
        scaleDpi = RLGetWindowScaleDPI();
        windowPos = RLGetWindowPosition();

        if (RLIsKeyPressed(RL_E_KEY_SPACE)) RLToggleBorderlessWindowed();
        if (RLIsKeyPressed(RL_E_KEY_F)) RLToggleFullscreen();
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            // Draw grid
            for (int h = 0; h < RLGetScreenHeight()/gridSpacing + 1; h++)
            {
                RLDrawText(RLTextFormat("%02i", h*gridSpacing), 4, h*gridSpacing - 4, 10, GRAY);
                RLDrawLine(24, h*gridSpacing, RLGetScreenWidth(), h*gridSpacing, LIGHTGRAY);
            }
            for (int v = 0; v < RLGetScreenWidth()/gridSpacing + 1; v++)
            {
                RLDrawText(RLTextFormat("%02i", v*gridSpacing), v*gridSpacing - 10, 4, 10, GRAY);
                RLDrawLine(v*gridSpacing, 20, v*gridSpacing, RLGetScreenHeight(), LIGHTGRAY);
            }

            // Draw UI info
            RLDrawText(RLTextFormat("CURRENT MONITOR: %i/%i (%ix%i)", currentMonitor + 1, RLGetMonitorCount(), 
                RLGetMonitorWidth(currentMonitor), RLGetMonitorHeight(currentMonitor)), 50, 50, 20, DARKGRAY);
            RLDrawText(RLTextFormat("WINDOW POSITION: %ix%i", (int)windowPos.x, (int)windowPos.y), 50, 90, 20, DARKGRAY);
            RLDrawText(RLTextFormat("SCREEN SIZE: %ix%i", RLGetScreenWidth(), RLGetScreenHeight()), 50, 130, 20, DARKGRAY);
            RLDrawText(RLTextFormat("RENDER SIZE: %ix%i", RLGetRenderWidth(), RLGetRenderHeight()), 50, 170, 20, DARKGRAY);
            RLDrawText(RLTextFormat("SCALE FACTOR: %.1fx%.1f", scaleDpi.x, scaleDpi.y), 50, 210, 20, GRAY);

            // Draw reference rectangles, top-left and bottom-right corners
            RLDrawRectangle(0, 0, 30, 60, RED);
            RLDrawRectangle(RLGetScreenWidth() - 30, RLGetScreenHeight() - 60, 30, 60, BLUE);

            // Draw mouse position
            RLDrawCircleV(RLGetMousePosition(), 20, MAROON);
            RLDrawRectangle(mousePos.x - 25, mousePos.y, 50, 2, BLACK);
            RLDrawRectangle(mousePos.x, mousePos.y - 25, 2, 50, BLACK);
            RLDrawText(RLTextFormat("[%i,%i]", RLGetMouseX(), RLGetMouseY()), mousePos.x - 44,
                (mousePos.y > RLGetScreenHeight() - 60)? mousePos.y - 46 : mousePos.y + 30, 20, BLACK);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------

    // TODO: Unload all loaded resources at this point

    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
