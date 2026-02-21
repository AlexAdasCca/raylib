/*******************************************************************************************
*
*   raylib [shapes] example - collision area
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 2.5, last time updated with raylib 2.5
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

    RLInitWindow(screenWidth, screenHeight, "raylib [shapes] example - collision area");

    // Box A: Moving box
    RLRectangle boxA = { 10, RLGetScreenHeight()/2.0f - 50, 200, 100 };
    int boxASpeedX = 4;

    // Box B: Mouse moved box
    RLRectangle boxB = { RLGetScreenWidth()/2.0f - 30, RLGetScreenHeight()/2.0f - 30, 60, 60 };

    RLRectangle boxCollision = { 0 }; // Collision rectangle

    int screenUpperLimit = 40;      // Top menu limits

    bool pause = false;             // Movement pause
    bool collision = false;         // Collision detection

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //----------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //-----------------------------------------------------
        // Move box if not paused
        if (!pause) boxA.x += boxASpeedX;

        // Bounce box on x screen limits
        if (((boxA.x + boxA.width) >= RLGetScreenWidth()) || (boxA.x <= 0)) boxASpeedX *= -1;

        // Update player-controlled-box (box02)
        boxB.x = RLGetMouseX() - boxB.width/2;
        boxB.y = RLGetMouseY() - boxB.height/2;

        // Make sure Box B does not go out of move area limits
        if ((boxB.x + boxB.width) >= RLGetScreenWidth()) boxB.x = RLGetScreenWidth() - boxB.width;
        else if (boxB.x <= 0) boxB.x = 0;

        if ((boxB.y + boxB.height) >= RLGetScreenHeight()) boxB.y = RLGetScreenHeight() - boxB.height;
        else if (boxB.y <= screenUpperLimit) boxB.y = (float)screenUpperLimit;

        // Check boxes collision
        collision = RLCheckCollisionRecs(boxA, boxB);

        // Get collision rectangle (only on collision)
        if (collision) boxCollision = RLGetCollisionRec(boxA, boxB);

        // Pause Box A movement
        if (RLIsKeyPressed(RL_E_KEY_SPACE)) pause = !pause;
        //-----------------------------------------------------

        // Draw
        //-----------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawRectangle(0, 0, screenWidth, screenUpperLimit, collision? RED : BLACK);

            RLDrawRectangleRec(boxA, GOLD);
            RLDrawRectangleRec(boxB, BLUE);

            if (collision)
            {
                // Draw collision area
                RLDrawRectangleRec(boxCollision, LIME);

                // Draw collision message
                RLDrawText("COLLISION!", RLGetScreenWidth()/2 - RLMeasureText("COLLISION!", 20)/2, screenUpperLimit/2 - 10, 20, BLACK);

                // Draw collision area
                RLDrawText(RLTextFormat("Collision Area: %i", (int)boxCollision.width*(int)boxCollision.height), RLGetScreenWidth()/2 - 100, screenUpperLimit + 10, 20, BLACK);
            }

            // Draw help instructions
            RLDrawText("Press SPACE to PAUSE/RESUME", 20, screenHeight - 35, 20, LIGHTGRAY);

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
