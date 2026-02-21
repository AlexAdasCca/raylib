/*******************************************************************************************
*
*   raylib [audio] example - sound positioning
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 5.5, last time updated with raylib 5.5
*
*   Example contributed by Le Juez Victor (@Bigfoot71) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Le Juez Victor (@Bigfoot71)
*
********************************************************************************************/

#include "raylib.h"

#include "raymath.h"

//------------------------------------------------------------------------------------
// Module Functions Declaration
//------------------------------------------------------------------------------------
static void SetSoundPosition(RLCamera listener, RLSound sound, RLVector3 position, float maxDist);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [audio] example - sound positioning");

    RLInitAudioDevice();

    RLSound sound = RLLoadSound("resources/coin.wav");

    RLCamera camera = {
        .position = (RLVector3) { 0, 5, 5 },
        .target = (RLVector3) { 0, 0, 0 },
        .up = (RLVector3) { 0, 1, 0 },
        .fovy = 60,
        .projection = RL_E_CAMERA_PERSPECTIVE
    };

    RLDisableCursor();

    RLSetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, RL_E_CAMERA_FREE);

        float th = (float)RLGetTime();

        RLVector3 spherePos = {
            .x = 5.0f*cosf(th),
            .y = 0.0f,
            .z = 5.0f*sinf(th)
        };

        SetSoundPosition(camera, sound, spherePos, 20.0f);

        if (!RLIsSoundPlaying(sound)) RLPlaySound(sound);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);
                RLDrawGrid(10, 2);
                RLDrawSphere(spherePos, 0.5f, RED);
            RLEndMode3D();

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadSound(sound);
    RLCloseAudioDevice();     // Close audio device

    RLCloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definition
//------------------------------------------------------------------------------------
// Set sound 3d position
static void SetSoundPosition(RLCamera listener, RLSound sound, RLVector3 position, float maxDist)
{
    // Calculate direction vector and distance between listener and sound source
    RLVector3 direction = RLVector3Subtract(position, listener.position);
    float distance = RLVector3Length(direction);

    // Apply logarithmic distance attenuation and clamp between 0-1
    float attenuation = 1.0f/(1.0f + (distance/maxDist));
    attenuation = RLClamp(attenuation, 0.0f, 1.0f);

    // Calculate normalized vectors for spatial positioning
    RLVector3 normalizedDirection = RLVector3Normalize(direction);
    RLVector3 forward = RLVector3Normalize(RLVector3Subtract(listener.target, listener.position));
    RLVector3 right = RLVector3Normalize(RLVector3CrossProduct(listener.up, forward));

    // Reduce volume for sounds behind the listener
    float dotProduct = RLVector3DotProduct(forward, normalizedDirection);
    if (dotProduct < 0.0f) attenuation *= (1.0f + dotProduct*0.5f);

    // Set stereo panning based on sound position relative to listener
    float pan = 0.5f + 0.5f*RLVector3DotProduct(normalizedDirection, right);

    // Apply final sound properties
    RLSetSoundVolume(sound, attenuation);
    RLSetSoundPan(sound, pan);
}
