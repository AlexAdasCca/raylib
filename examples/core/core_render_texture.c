/*******************************************************************************************
*
*   raylib [core] example - render texture
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 5.6-dev, last time updated with raylib 5.6-dev
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Ramon Santamaria (@raysan5)
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

    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - render texture");

    // Define a render texture to render
    int renderTextureWidth = 300;
    int renderTextureHeight = 300;
    RLRenderTexture2D target = RLLoadRenderTexture(renderTextureWidth, renderTextureHeight);

    RLVector2 ballPosition = { renderTextureWidth/2.0f, renderTextureHeight/2.0f };
    RLVector2 ballSpeed = { 5.0f, 4.0f };
    int ballRadius = 20;

    float rotation = 0.0f;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //----------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //-----------------------------------------------------
        // Ball movement logic
        ballPosition.x += ballSpeed.x;
        ballPosition.y += ballSpeed.y;

        // Check walls collision for bouncing
        if ((ballPosition.x >= (renderTextureWidth - ballRadius)) || (ballPosition.x <= ballRadius)) ballSpeed.x *= -1.0f;
        if ((ballPosition.y >= (renderTextureHeight - ballRadius)) || (ballPosition.y <= ballRadius)) ballSpeed.y *= -1.0f;

        // Render texture rotation
        rotation += 0.5f;
        //-----------------------------------------------------

        // Draw
        //-----------------------------------------------------
        // Draw our scene to the render texture
        RLBeginTextureMode(target);

            RLClearBackground(SKYBLUE);

            RLDrawRectangle(0, 0, 20, 20, RED);
            RLDrawCircleV(ballPosition, (float)ballRadius, MAROON);

        RLEndTextureMode();

        // Draw render texture to main framebuffer
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            // Draw our render texture with rotation applied
            // NOTE 1: We set the origin of the texture to the center of the render texture
            // NOTE 2: We flip vertically the texture setting negative source rectangle height
            RLDrawTexturePro(target.texture,
                (RLRectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height },
                (RLRectangle){ screenWidth/2.0f, screenHeight/2.0f, (float)target.texture.width, (float)target.texture.height },
                (RLVector2){ target.texture.width/2.0f, target.texture.height/2.0f }, rotation, WHITE);

            RLDrawText("DRAWING BOUNCING BALL INSIDE RENDER TEXTURE!", 10, screenHeight - 40, 20, BLACK);


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