/*******************************************************************************************
*
*   raylib [audio] example - sound loading
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 1.1, last time updated with raylib 3.5
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

    RLInitWindow(screenWidth, screenHeight, "raylib [audio] example - sound loading");

    RLInitAudioDevice();      // Initialize audio device

    RLSound fxWav = RLLoadSound("resources/sound.wav");         // Load WAV audio file
    RLSound fxOgg = RLLoadSound("resources/target.ogg");        // Load OGG audio file

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (RLIsKeyPressed(RL_E_KEY_SPACE)) RLPlaySound(fxWav);      // Play WAV sound
        if (RLIsKeyPressed(RL_E_KEY_ENTER)) RLPlaySound(fxOgg);      // Play OGG sound
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawText("Press SPACE to PLAY the WAV sound!", 200, 180, 20, LIGHTGRAY);
            RLDrawText("Press ENTER to PLAY the OGG sound!", 200, 220, 20, LIGHTGRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadSound(fxWav);     // Unload sound data
    RLUnloadSound(fxOgg);     // Unload sound data

    RLCloseAudioDevice();     // Close audio device

    RLCloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}