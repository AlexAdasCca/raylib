/*******************************************************************************************
*
*   raylib [shapes] example - dashed line
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 5.5, last time updated with raylib 5.5
*
*   Example contributed by Luís Almeida (@luis605)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Luís Almeida (@luis605)
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

    RLInitWindow(screenWidth, screenHeight, "raylib [shapes] example - dashed line");

    // Line Properties
    RLVector2 lineStartPosition = { 20.0f, 50.0f };
    RLVector2 lineEndPosition = { 780.0f, 400.0f };
    float dashLength = 25.0f;
    float blankLength = 15.0f;

    // Color selection
    RLColor lineColors[] = { RED, ORANGE, GOLD, GREEN, BLUE, VIOLET, PINK, BLACK };
    int colorIndex = 0;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        lineEndPosition = RLGetMousePosition(); // Line endpoint follows the mouse

        // Change Dash Length (UP/DOWN arrows)
        if (RLIsKeyDown(RL_E_KEY_UP)) dashLength += 1.0f;
        if (RLIsKeyDown(RL_E_KEY_DOWN) && dashLength > 1.0f) dashLength -= 1.0f;

        // Change Space Length (LEFT/RIGHT arrows)
        if (RLIsKeyDown(RL_E_KEY_RIGHT)) blankLength += 1.0f;
        if (RLIsKeyDown(RL_E_KEY_LEFT) && blankLength > 1.0f) blankLength -= 1.0f;

        // Cycle through colors ('C' key)
        if (RLIsKeyPressed(RL_E_KEY_C)) colorIndex = (colorIndex + 1)%(sizeof(lineColors)/sizeof(RLColor));
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            // Draw the dashed line with the current properties
            RLDrawLineDashed(lineStartPosition, lineEndPosition, (int)dashLength, (int)blankLength, lineColors[colorIndex]);

            // Draw UI and Instructions
            RLDrawRectangle(5, 5, 265, 95, RLFade(SKYBLUE, 0.5f));
            RLDrawRectangleLines(5, 5, 265, 95, BLUE);

            RLDrawText("CONTROLS:", 15, 15, 10, BLACK);
            RLDrawText("UP/DOWN: Change Dash Length", 15, 35, 10, BLACK);
            RLDrawText("LEFT/RIGHT: Change Space Length", 15, 55, 10, BLACK);
            RLDrawText("C: Cycle Color", 15, 75, 10, BLACK);

            RLDrawText(RLTextFormat("Dash: %.0f | Space: %.0f", dashLength, blankLength), 15, 115, 10, DARKGRAY);

            RLDrawFPS(screenWidth - 80, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}