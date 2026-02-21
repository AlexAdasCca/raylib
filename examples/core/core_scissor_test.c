/*******************************************************************************************
*
*   raylib [core] example - scissor test
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 2.5, last time updated with raylib 3.0
*
*   Example contributed by Chris Dill (@MysteriousSpace) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2019-2025 Chris Dill (@MysteriousSpace)
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

    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - scissor test");

    RLRectangle scissorArea = { 0, 0, 300, 300 };
    bool scissorMode = true;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (RLIsKeyPressed(RL_E_KEY_S)) scissorMode = !scissorMode;

        // Centre the scissor area around the mouse position
        scissorArea.x = RLGetMouseX() - scissorArea.width/2;
        scissorArea.y = RLGetMouseY() - scissorArea.height/2;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            if (scissorMode) RLBeginScissorMode((int)scissorArea.x, (int)scissorArea.y, (int)scissorArea.width, (int)scissorArea.height);

            // Draw full screen rectangle and some text
            // NOTE: Only part defined by scissor area will be rendered
            RLDrawRectangle(0, 0, RLGetScreenWidth(), RLGetScreenHeight(), RED);
            RLDrawText("Move the mouse around to reveal this text!", 190, 200, 20, LIGHTGRAY);

            if (scissorMode) RLEndScissorMode();

            RLDrawRectangleLinesEx(scissorArea, 1, BLACK);
            RLDrawText("Press S to toggle scissor test", 10, 10, 20, BLACK);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
