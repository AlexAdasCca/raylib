/*******************************************************************************************
*
*   raylib [shapes] example - vector angle
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 1.0, last time updated with raylib 5.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2023-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#include "raymath.h"        // Required for: Vector2LineAngle()

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;

    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [shapes] example - vector angle");

    RLVector2 v0 = { screenWidth/2.0f, screenHeight/2.0f };
    RLVector2 v1 = Vector2Add(v0, (RLVector2){ 100.0f, 80.0f });
    RLVector2 v2 = { 0 };             // Updated with mouse position

    float angle = 0.0f;             // Angle in degrees
    int angleMode = 0;              // 0-Vector2Angle(), 1-Vector2LineAngle()

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        float startangle = 0.0f;

        if (angleMode == 0) startangle = -Vector2LineAngle(v0, v1)*RAD2DEG;
        if (angleMode == 1) startangle = 0.0f;

        v2 = RLGetMousePosition();

        if (RLIsKeyPressed(KEY_SPACE)) angleMode = !angleMode;

        if ((angleMode == 0) && RLIsMouseButtonDown(MOUSE_BUTTON_RIGHT)) v1 = RLGetMousePosition();

        if (angleMode == 0)
        {
            // Calculate angle between two vectors, considering a common origin (v0)
            RLVector2 v1Normal = Vector2Normalize(Vector2Subtract(v1, v0));
            RLVector2 v2Normal = Vector2Normalize(Vector2Subtract(v2, v0));

            angle = Vector2Angle(v1Normal, v2Normal)*RAD2DEG;
        }
        else if (angleMode == 1)
        {
            // Calculate angle defined by a two vectors line, in reference to horizontal line
            angle = Vector2LineAngle(v0, v2)*RAD2DEG;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            if (angleMode == 0)
            {
                RLDrawText("MODE 0: Angle between V1 and V2", 10, 10, 20, BLACK);
                RLDrawText("Right Click to Move V2", 10, 30, 20, DARKGRAY);

                RLDrawLineEx(v0, v1, 2.0f, BLACK);
                RLDrawLineEx(v0, v2, 2.0f, RED);

                RLDrawCircleSector(v0, 40.0f, startangle, startangle + angle, 32, RLFade(GREEN, 0.6f));
            }
            else if (angleMode == 1)
            {
                RLDrawText("MODE 1: Angle formed by line V1 to V2", 10, 10, 20, BLACK);

                RLDrawLine(0, screenHeight/2, screenWidth, screenHeight/2, LIGHTGRAY);
                RLDrawLineEx(v0, v2, 2.0f, RED);

                RLDrawCircleSector(v0, 40.0f, startangle, startangle - angle, 32, RLFade(GREEN, 0.6f));
            }

            RLDrawText("v0", (int)v0.x, (int)v0.y, 10, DARKGRAY);

            // If the line from v0 to v1 would overlap the text, move it's position up 10
            if (angleMode == 0 && Vector2Subtract(v0, v1).y > 0.0f) RLDrawText("v1", (int)v1.x, (int)v1.y-10, 10, DARKGRAY);
            if (angleMode == 0 && Vector2Subtract(v0, v1).y < 0.0f) RLDrawText("v1", (int)v1.x, (int)v1.y, 10, DARKGRAY);

            // If angle mode 1, use v1 to emphasize the horizontal line
            if (angleMode == 1) RLDrawText("v1", (int)v0.x + 40, (int)v0.y, 10, DARKGRAY);

            // position adjusted by -10 so it isn't hidden by cursor
            RLDrawText("v2", (int)v2.x-10, (int)v2.y-10, 10, DARKGRAY);

            RLDrawText("Press SPACE to change MODE", 460, 10, 20, DARKGRAY);
            RLDrawText(RLTextFormat("ANGLE: %2.2f", angle), 10, 70, 20, LIME);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
