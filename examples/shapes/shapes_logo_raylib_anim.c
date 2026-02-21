/*******************************************************************************************
*
*   raylib [shapes] example - logo raylib anim
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 2.5, last time updated with raylib 4.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2014-2025 Ramon Santamaria (@raysan5)
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

    RLInitWindow(screenWidth, screenHeight, "raylib [shapes] example - logo raylib anim");

    int logoPositionX = screenWidth/2 - 128;
    int logoPositionY = screenHeight/2 - 128;

    int framesCounter = 0;
    int lettersCount = 0;

    int topSideRecWidth = 16;
    int leftSideRecHeight = 16;

    int bottomSideRecWidth = 16;
    int rightSideRecHeight = 16;

    int state = 0;                  // Tracking animation states (State Machine)
    float alpha = 1.0f;             // Useful for fading

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (state == 0)                 // State 0: Small box blinking
        {
            framesCounter++;

            if (framesCounter == 120)
            {
                state = 1;
                framesCounter = 0;      // Reset counter... will be used later...
            }
        }
        else if (state == 1)            // State 1: Top and left bars growing
        {
            topSideRecWidth += 4;
            leftSideRecHeight += 4;

            if (topSideRecWidth == 256) state = 2;
        }
        else if (state == 2)            // State 2: Bottom and right bars growing
        {
            bottomSideRecWidth += 4;
            rightSideRecHeight += 4;

            if (bottomSideRecWidth == 256) state = 3;
        }
        else if (state == 3)            // State 3: Letters appearing (one by one)
        {
            framesCounter++;

            if (framesCounter/12)       // Every 12 frames, one more letter!
            {
                lettersCount++;
                framesCounter = 0;
            }

            if (lettersCount >= 10)     // When all letters have appeared, just fade out everything
            {
                alpha -= 0.02f;

                if (alpha <= 0.0f)
                {
                    alpha = 0.0f;
                    state = 4;
                }
            }
        }
        else if (state == 4)            // State 4: Reset and Replay
        {
            if (RLIsKeyPressed(RL_E_KEY_R))
            {
                framesCounter = 0;
                lettersCount = 0;

                topSideRecWidth = 16;
                leftSideRecHeight = 16;

                bottomSideRecWidth = 16;
                rightSideRecHeight = 16;

                alpha = 1.0f;
                state = 0;          // Return to State 0
            }
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            if (state == 0)
            {
                if ((framesCounter/15)%2) RLDrawRectangle(logoPositionX, logoPositionY, 16, 16, BLACK);
            }
            else if (state == 1)
            {
                RLDrawRectangle(logoPositionX, logoPositionY, topSideRecWidth, 16, BLACK);
                RLDrawRectangle(logoPositionX, logoPositionY, 16, leftSideRecHeight, BLACK);
            }
            else if (state == 2)
            {
                RLDrawRectangle(logoPositionX, logoPositionY, topSideRecWidth, 16, BLACK);
                RLDrawRectangle(logoPositionX, logoPositionY, 16, leftSideRecHeight, BLACK);

                RLDrawRectangle(logoPositionX + 240, logoPositionY, 16, rightSideRecHeight, BLACK);
                RLDrawRectangle(logoPositionX, logoPositionY + 240, bottomSideRecWidth, 16, BLACK);
            }
            else if (state == 3)
            {
                RLDrawRectangle(logoPositionX, logoPositionY, topSideRecWidth, 16, RLFade(BLACK, alpha));
                RLDrawRectangle(logoPositionX, logoPositionY + 16, 16, leftSideRecHeight - 32, RLFade(BLACK, alpha));

                RLDrawRectangle(logoPositionX + 240, logoPositionY + 16, 16, rightSideRecHeight - 32, RLFade(BLACK, alpha));
                RLDrawRectangle(logoPositionX, logoPositionY + 240, bottomSideRecWidth, 16, RLFade(BLACK, alpha));

                RLDrawRectangle(RLGetScreenWidth()/2 - 112, RLGetScreenHeight()/2 - 112, 224, 224, RLFade(RAYWHITE, alpha));

                RLDrawText(RLTextSubtext("raylib", 0, lettersCount), RLGetScreenWidth()/2 - 44, RLGetScreenHeight()/2 + 48, 50, RLFade(BLACK, alpha));
            }
            else if (state == 4)
            {
                RLDrawText("[R] REPLAY", 340, 200, 20, GRAY);
            }

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
