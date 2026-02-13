/*******************************************************************************************
*
*   raylib [core] example - smooth pixelperfect
*
*   Example complexity rating: [★★★☆] 3/4
*
*   Example originally created with raylib 3.7, last time updated with raylib 4.0
*
*   Example contributed by Giancamillo Alessandroni (@NotManyIdeasDev) and
*   reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2021-2025 Giancamillo Alessandroni (@NotManyIdeasDev) and Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#include <math.h>       // Required for: sinf(), cosf()

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    const int virtualScreenWidth = 160;
    const int virtualScreenHeight = 90;

    const float virtualRatio = (float)screenWidth/(float)virtualScreenWidth;

    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - smooth pixelperfect");

    RLCamera2D worldSpaceCamera = { 0 };  // Game world camera
    worldSpaceCamera.zoom = 1.0f;

    RLCamera2D screenSpaceCamera = { 0 }; // Smoothing camera
    screenSpaceCamera.zoom = 1.0f;

    // Load render texture to draw all our objects
    RLRenderTexture2D target = RLLoadRenderTexture(virtualScreenWidth, virtualScreenHeight);

    RLRectangle rec01 = { 70.0f, 35.0f, 20.0f, 20.0f };
    RLRectangle rec02 = { 90.0f, 55.0f, 30.0f, 10.0f };
    RLRectangle rec03 = { 80.0f, 65.0f, 15.0f, 25.0f };

    // The target's height is flipped (in the source Rectangle), due to OpenGL reasons
    RLRectangle sourceRec = { 0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height };
    RLRectangle destRec = { -virtualRatio, -virtualRatio, screenWidth + (virtualRatio*2), screenHeight + (virtualRatio*2) };

    RLVector2 origin = { 0.0f, 0.0f };

    float rotation = 0.0f;

    float cameraX = 0.0f;
    float cameraY = 0.0f;

    RLSetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        rotation += 60.0f*RLGetFrameTime();   // Rotate the rectangles, 60 degrees per second

        // Make the camera move to demonstrate the effect
        cameraX = (sinf((float)RLGetTime())*50.0f) - 10.0f;
        cameraY = cosf((float)RLGetTime())*30.0f;

        // Set the camera's target to the values computed above
        screenSpaceCamera.target = (RLVector2){ cameraX, cameraY };

        // Round worldSpace coordinates, keep decimals into screenSpace coordinates
        worldSpaceCamera.target.x = truncf(screenSpaceCamera.target.x);
        screenSpaceCamera.target.x -= worldSpaceCamera.target.x;
        screenSpaceCamera.target.x *= virtualRatio;

        worldSpaceCamera.target.y = truncf(screenSpaceCamera.target.y);
        screenSpaceCamera.target.y -= worldSpaceCamera.target.y;
        screenSpaceCamera.target.y *= virtualRatio;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginTextureMode(target);
            RLClearBackground(RAYWHITE);

            RLBeginMode2D(worldSpaceCamera);
                RLDrawRectanglePro(rec01, origin, rotation, BLACK);
                RLDrawRectanglePro(rec02, origin, -rotation, RED);
                RLDrawRectanglePro(rec03, origin, rotation + 45.0f, BLUE);
            RLEndMode2D();
        RLEndTextureMode();

        RLBeginDrawing();
            RLClearBackground(RED);

            RLBeginMode2D(screenSpaceCamera);
                RLDrawTexturePro(target.texture, sourceRec, destRec, origin, 0.0f, WHITE);
            RLEndMode2D();

            RLDrawText(RLTextFormat("Screen resolution: %ix%i", screenWidth, screenHeight), 10, 10, 20, DARKBLUE);
            RLDrawText(RLTextFormat("World resolution: %ix%i", virtualScreenWidth, virtualScreenHeight), 10, 40, 20, DARKGREEN);
            RLDrawFPS(RLGetScreenWidth() - 95, 10);
        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadRenderTexture(target);    // Unload render texture

    RLCloseWindow();                  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}