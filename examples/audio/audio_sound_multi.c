/*******************************************************************************************
*
*   raylib [audio] example - sound multi
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 5.0, last time updated with raylib 5.0
*
*   Example contributed by Jeffery Myers (@JeffM2501) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2023-2025 Jeffery Myers (@JeffM2501)
*
********************************************************************************************/

#include "raylib.h"

#define MAX_SOUNDS 10
RLSound soundArray[MAX_SOUNDS] = { 0 };
int currentSound;

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [audio] example - sound multi");

    RLInitAudioDevice();      // Initialize audio device

    // Load audio file into the first slot as the 'source' sound,
    // this sound owns the sample data
    soundArray[0] = RLLoadSound("resources/sound.wav");

    // Load an alias of the sound into slots 1-9. These do not own the sound data, but can be played
    for (int i = 1; i < MAX_SOUNDS; i++) soundArray[i] = RLLoadSoundAlias(soundArray[0]);

    currentSound = 0;               // Set the sound list to the start

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (RLIsKeyPressed(KEY_SPACE))
        {
            RLPlaySound(soundArray[currentSound]);    // Play the next open sound slot
            currentSound++;                         // Increment the sound slot

            // If the sound slot is out of bounds, go back to 0
            if (currentSound >= MAX_SOUNDS) currentSound = 0;

            // NOTE: Another approach would be to look at the list for the first sound
            // that is not playing and use that slot
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawText("Press SPACE to PLAY a WAV sound!", 200, 180, 20, LIGHTGRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    for (int i = 1; i < MAX_SOUNDS; i++) RLUnloadSoundAlias(soundArray[i]); // Unload sound aliases
    RLUnloadSound(soundArray[0]); // Unload source sound data

    RLCloseAudioDevice();     // Close audio device

    RLCloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}