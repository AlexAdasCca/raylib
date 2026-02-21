/*******************************************************************************************
*
*   raylib [models] example - directional billboard
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 5.6-dev, last time updated with raylib 5.6
*
*   Example contributed by Robin (@RobinsAviary) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Robin (@RobinsAviary)
*   Killbot art by patvanmackelberg https://opengameart.org/content/killbot-8-directional under CC0
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [models] example - directional billboard");

    // Set up the camera
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 2.0f, 1.0f, 2.0f }; // Starting position
    camera.target = (RLVector3){ 0.0f, 0.5f, 0.0f };  // Target position
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f }; // Up vector
    camera.fovy = 45.0f; // FOV
    camera.projection = RL_E_CAMERA_PERSPECTIVE; // Projection type (Standard 3D perspective)

    // Load billboard texture
    RLTexture skillbot = RLLoadTexture("resources/skillbot.png");

    // Timer to update animation
    float anim_timer = 0.0f;
    // Animation frame
    unsigned int anim = 0;

    RLSetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, RL_E_CAMERA_ORBITAL);

        // Update timer with delta time
        anim_timer += RLGetFrameTime();

        // Update frame index after a certain amount of time (half a second)
        if (anim_timer > 0.5f)
        {
            anim_timer = 0.0f;
            anim += 1;
        }

        // Reset frame index to zero on overflow
        if (anim >= 4) anim = 0;

        // Find the current direction frame based on the camera position to the billboard object
        float dir = (float)floor(((RLVector2Angle((RLVector2){ 2.0f, 0.0f }, (RLVector2){ camera.position.x, camera.position.z })/PI)*4.0f) + 0.25f);

        // Correct frame index if angle is negative
        if (dir < 0.0f)
        {
            dir = 8.0f - (float)abs((int)dir);
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

        RLClearBackground(RAYWHITE);

        RLBeginMode3D(camera);

            RLDrawGrid(10, 1.0f);

            // Draw billboard pointing straight up to the sky, rotated relative to the camera and offset from the bottom
            RLDrawBillboardPro(camera, skillbot, (RLRectangle){ 0.0f + (anim*24.0f), 0.0f + (dir*24.0f), 24.0f, 24.0f }, RLVector3Zero(), (RLVector3){ 0.0f, 1.0f, 0.0f }, RLVector2One(), (RLVector2){ 0.5f, 0.0f }, 0, WHITE);

        RLEndMode3D();

        // Render various variables for reference
        RLDrawText(RLTextFormat("animation: %d", anim), 10, 10, 20, DARKGRAY);
        RLDrawText(RLTextFormat("direction frame: %.0f", dir), 10, 40, 20, DARKGRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unload billboard texture
    RLUnloadTexture(skillbot);

    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}