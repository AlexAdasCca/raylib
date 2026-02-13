/*******************************************************************************************
*
*   raylib [shapes] example - bouncing ball
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 2.5, last time updated with raylib 2.5
*
*   Example contributed by Ramon Santamaria (@raysan5), reviewed by Jopestpe (@jopestpe)
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
    //---------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLSetConfigFlags(FLAG_MSAA_4X_HINT);
    RLInitWindow(screenWidth, screenHeight, "raylib [shapes] example - bouncing ball");

    RLVector2 ballPosition = { RLGetScreenWidth()/2.0f, RLGetScreenHeight()/2.0f };
    RLVector2 ballSpeed = { 5.0f, 4.0f };
    int ballRadius = 20;
    float gravity = 0.2f;

    bool useGravity = true;
    bool pause = 0;
    int framesCounter = 0;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //----------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //-----------------------------------------------------
        if (RLIsKeyPressed(KEY_G)) useGravity = !useGravity;
        if (RLIsKeyPressed(KEY_SPACE)) pause = !pause;

        if (!pause)
        {
            ballPosition.x += ballSpeed.x;
            ballPosition.y += ballSpeed.y;

            if (useGravity) ballSpeed.y += gravity;

            // Check walls collision for bouncing
            if ((ballPosition.x >= (RLGetScreenWidth() - ballRadius)) || (ballPosition.x <= ballRadius)) ballSpeed.x *= -1.0f;
            if ((ballPosition.y >= (RLGetScreenHeight() - ballRadius)) || (ballPosition.y <= ballRadius)) ballSpeed.y *= -0.95f;
        }
        else framesCounter++;
        //-----------------------------------------------------

        // Draw
        //-----------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawCircleV(ballPosition, (float)ballRadius, MAROON);
            RLDrawText("PRESS SPACE to PAUSE BALL MOVEMENT", 10, RLGetScreenHeight() - 25, 20, LIGHTGRAY);

            if (useGravity) RLDrawText("GRAVITY: ON (Press G to disable)", 10, RLGetScreenHeight() - 50, 20, DARKGREEN);
            else RLDrawText("GRAVITY: OFF (Press G to enable)", 10, RLGetScreenHeight() - 50, 20, RED);

            // On pause, we draw a blinking message
            if (pause && ((framesCounter/30)%2)) RLDrawText("PAUSED", 350, 200, 30, GRAY);

            RLDrawFPS(10, 10);

        RLEndDrawing();
        //-----------------------------------------------------
    }

    // De-Initialization
    //---------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //----------------------------------------------------------

    return 0;
}