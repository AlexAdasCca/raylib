/*******************************************************************************************
*
*   raylib [core] example - delta time
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 5.5, last time updated with raylib 5.6-dev
*
*   Example contributed by Robin (@RobinsAviary) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Robin (@RobinsAviary)
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

    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - delta time");

    int currentFps = 60;

    // Store the position for the both of the circles
    RLVector2 deltaCircle = { 0, (float)screenHeight/3.0f };
    RLVector2 frameCircle = { 0, (float)screenHeight*(2.0f/3.0f) };

    // The speed applied to both circles
    const float speed = 10.0f;
    const float circleRadius = 32.0f;

    RLSetTargetFPS(currentFps);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose()) // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // Adjust the FPS target based on the mouse wheel
        float mouseWheel = RLGetMouseWheelMove();
        if (mouseWheel != 0)
        {
            currentFps += (int)mouseWheel;
            if (currentFps < 0) currentFps = 0;
            RLSetTargetFPS(currentFps);
        }

        // GetFrameTime() returns the time it took to draw the last frame, in seconds (usually called delta time)
        // Uses the delta time to make the circle look like it's moving at a "consistent" speed regardless of FPS

        // Multiply by 6.0 (an arbitrary value) in order to make the speed
        // visually closer to the other circle (at 60 fps), for comparison
        deltaCircle.x += RLGetFrameTime()*6.0f*speed;
        // This circle can move faster or slower visually depending on the FPS
        frameCircle.x += 0.1f*speed;

        // If either circle is off the screen, reset it back to the start
        if (deltaCircle.x > screenWidth) deltaCircle.x = 0;
        if (frameCircle.x > screenWidth) frameCircle.x = 0;

        // Reset both circles positions
        if (RLIsKeyPressed(RL_E_KEY_R))
        {
            deltaCircle.x = 0;
            frameCircle.x = 0;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();
            RLClearBackground(RAYWHITE);

            // Draw both circles to the screen
            RLDrawCircleV(deltaCircle, circleRadius, RED);
            RLDrawCircleV(frameCircle, circleRadius, BLUE);

            // Draw the help text
            // Determine what help text to show depending on the current FPS target
            const char *fpsText = 0;
            if (currentFps <= 0) fpsText = RLTextFormat("FPS: unlimited (%i)", RLGetFPS());
            else fpsText = RLTextFormat("FPS: %i (target: %i)", RLGetFPS(), currentFps);
            RLDrawText(fpsText, 10, 10, 20, DARKGRAY);
            RLDrawText(RLTextFormat("Frame time: %02.02f ms", RLGetFrameTime()), 10, 30, 20, DARKGRAY);
            RLDrawText("Use the scroll wheel to change the fps limit, r to reset", 10, 50, 20, DARKGRAY);

            // Draw the text above the circles
            RLDrawText("FUNC: x += GetFrameTime()*speed", 10, 90, 20, RED);
            RLDrawText("FUNC: x += speed", 10, 240, 20, BLUE);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}