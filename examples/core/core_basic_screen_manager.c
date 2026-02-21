/*******************************************************************************************
*
*   raylib [core] example - basic screen manager
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   NOTE: This example illustrates a very simple screen manager based on a states machines
*
*   Example originally created with raylib 4.0, last time updated with raylib 4.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2021-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

//------------------------------------------------------------------------------------------
// Types and Structures Definition
//------------------------------------------------------------------------------------------
typedef enum GameScreen { LOGO = 0, TITLE, GAMEPLAY, ENDING } GameScreen;

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - basic screen manager");

    GameScreen currentScreen = LOGO;

    // TODO: Initialize all required variables and load all required data here!

    int framesCounter = 0;          // Useful to count frames

    RLSetTargetFPS(60);               // Set desired framerate (frames-per-second)
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        switch (currentScreen)
        {
            case LOGO:
            {
                // TODO: Update LOGO screen variables here!

                framesCounter++;    // Count frames

                // Wait for 2 seconds (120 frames) before jumping to TITLE screen
                if (framesCounter > 120)
                {
                    currentScreen = TITLE;
                }
            } break;
            case TITLE:
            {
                // TODO: Update TITLE screen variables here!

                // Press enter to change to GAMEPLAY screen
                if (RLIsKeyPressed(RL_E_KEY_ENTER) || RLIsGestureDetected(RL_E_GESTURE_TAP))
                {
                    currentScreen = GAMEPLAY;
                }
            } break;
            case GAMEPLAY:
            {
                // TODO: Update GAMEPLAY screen variables here!

                // Press enter to change to ENDING screen
                if (RLIsKeyPressed(RL_E_KEY_ENTER) || RLIsGestureDetected(RL_E_GESTURE_TAP))
                {
                    currentScreen = ENDING;
                }
            } break;
            case ENDING:
            {
                // TODO: Update ENDING screen variables here!

                // Press enter to return to TITLE screen
                if (RLIsKeyPressed(RL_E_KEY_ENTER) || RLIsGestureDetected(RL_E_GESTURE_TAP))
                {
                    currentScreen = TITLE;
                }
            } break;
            default: break;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            switch(currentScreen)
            {
                case LOGO:
                {
                    // TODO: Draw LOGO screen here!
                    RLDrawText("LOGO SCREEN", 20, 20, 40, LIGHTGRAY);
                    RLDrawText("WAIT for 2 SECONDS...", 290, 220, 20, GRAY);

                } break;
                case TITLE:
                {
                    // TODO: Draw TITLE screen here!
                    RLDrawRectangle(0, 0, screenWidth, screenHeight, GREEN);
                    RLDrawText("TITLE SCREEN", 20, 20, 40, DARKGREEN);
                    RLDrawText("PRESS ENTER or TAP to JUMP to GAMEPLAY SCREEN", 120, 220, 20, DARKGREEN);

                } break;
                case GAMEPLAY:
                {
                    // TODO: Draw GAMEPLAY screen here!
                    RLDrawRectangle(0, 0, screenWidth, screenHeight, PURPLE);
                    RLDrawText("GAMEPLAY SCREEN", 20, 20, 40, MAROON);
                    RLDrawText("PRESS ENTER or TAP to JUMP to ENDING SCREEN", 130, 220, 20, MAROON);

                } break;
                case ENDING:
                {
                    // TODO: Draw ENDING screen here!
                    RLDrawRectangle(0, 0, screenWidth, screenHeight, BLUE);
                    RLDrawText("ENDING SCREEN", 20, 20, 40, DARKBLUE);
                    RLDrawText("PRESS ENTER or TAP to RETURN to TITLE SCREEN", 120, 220, 20, DARKBLUE);

                } break;
                default: break;
            }

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------

    // TODO: Unload all loaded data (textures, fonts, audio) here!

    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
