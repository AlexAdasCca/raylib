/*******************************************************************************************
*
*   raylib [audio] example - music stream
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 1.3, last time updated with raylib 4.2
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2015-2025 Ramon Santamaria (@raysan5)
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

    RLInitWindow(screenWidth, screenHeight, "raylib [audio] example - music stream");

    RLInitAudioDevice();              // Initialize audio device

    RLMusic music = RLLoadMusicStream("resources/country.mp3");

    RLPlayMusicStream(music);

    float timePlayed = 0.0f;        // Time played normalized [0.0f..1.0f]
    bool pause = false;             // Music playing paused

    float pan = 0.0f;               // Default audio pan center [-1.0f..1.0f]
    RLSetMusicPan(music, pan);

    float volume = 0.8f;            // Default audio volume [0.0f..1.0f]
    RLSetMusicVolume(music, volume);

    RLSetTargetFPS(30);               // Set our game to run at 30 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateMusicStream(music);   // Update music buffer with new stream data

        // Restart music playing (stop and play)
        if (RLIsKeyPressed(RL_E_KEY_SPACE))
        {
            RLStopMusicStream(music);
            RLPlayMusicStream(music);
        }

        // Pause/Resume music playing
        if (RLIsKeyPressed(RL_E_KEY_P))
        {
            pause = !pause;

            if (pause) RLPauseMusicStream(music);
            else RLResumeMusicStream(music);
        }

        // Set audio pan
        if (RLIsKeyDown(RL_E_KEY_LEFT))
        {
            pan -= 0.05f;
            if (pan < -1.0f) pan = -1.0f;
            RLSetMusicPan(music, pan);
        }
        else if (RLIsKeyDown(RL_E_KEY_RIGHT))
        {
            pan += 0.05f;
            if (pan > 1.0f) pan = 1.0f;
            RLSetMusicPan(music, pan);
        }

        // Set audio volume
        if (RLIsKeyDown(RL_E_KEY_DOWN))
        {
            volume -= 0.05f;
            if (volume < 0.0f) volume = 0.0f;
            RLSetMusicVolume(music, volume);
        }
        else if (RLIsKeyDown(RL_E_KEY_UP))
        {
            volume += 0.05f;
            if (volume > 1.0f) volume = 1.0f;
            RLSetMusicVolume(music, volume);
        }

        // Get normalized time played for current music stream
        timePlayed = RLGetMusicTimePlayed(music)/RLGetMusicTimeLength(music);

        if (timePlayed > 1.0f) timePlayed = 1.0f;   // Make sure time played is no longer than music
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawText("MUSIC SHOULD BE PLAYING!", 255, 150, 20, LIGHTGRAY);

            RLDrawText("LEFT-RIGHT for PAN CONTROL", 320, 74, 10, DARKBLUE);
            RLDrawRectangle(300, 100, 200, 12, LIGHTGRAY);
            RLDrawRectangleLines(300, 100, 200, 12, GRAY);
            RLDrawRectangle((int)(300 + (pan + 1.0f)/2.0f*200 - 5), 92, 10, 28, DARKGRAY);

            RLDrawRectangle(200, 200, 400, 12, LIGHTGRAY);
            RLDrawRectangle(200, 200, (int)(timePlayed*400.0f), 12, MAROON);
            RLDrawRectangleLines(200, 200, 400, 12, GRAY);

            RLDrawText("PRESS SPACE TO RESTART MUSIC", 215, 250, 20, LIGHTGRAY);
            RLDrawText("PRESS P TO PAUSE/RESUME MUSIC", 208, 280, 20, LIGHTGRAY);

            RLDrawText("UP-DOWN for VOLUME CONTROL", 320, 334, 10, DARKGREEN);
            RLDrawRectangle(300, 360, 200, 12, LIGHTGRAY);
            RLDrawRectangleLines(300, 360, 200, 12, GRAY);
            RLDrawRectangle((int)(300 + volume*200 - 5), 352, 10, 28, DARKGRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadMusicStream(music);   // Unload music stream buffers from RAM

    RLCloseAudioDevice();         // Close audio device (music streaming is automatically stopped)

    RLCloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
