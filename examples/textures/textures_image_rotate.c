/*******************************************************************************************
*
*   raylib [textures] example - image rotate
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 1.0, last time updated with raylib 1.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2014-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#define NUM_TEXTURES  3

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [textures] example - image rotate");

    // NOTE: Textures MUST be loaded after Window initialization (OpenGL context is required)
    RLImage image45 = RLLoadImage("resources/raylib_logo.png");
    RLImage image90 = RLLoadImage("resources/raylib_logo.png");
    RLImage imageNeg90 = RLLoadImage("resources/raylib_logo.png");

    RLImageRotate(&image45, 45);
    RLImageRotate(&image90, 90);
    RLImageRotate(&imageNeg90, -90);

    RLTexture2D textures[NUM_TEXTURES] = { 0 };

    textures[0] = RLLoadTextureFromImage(image45);
    textures[1] = RLLoadTextureFromImage(image90);
    textures[2] = RLLoadTextureFromImage(imageNeg90);

    int currentTexture = 0;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (RLIsMouseButtonPressed(MOUSE_BUTTON_LEFT) || RLIsKeyPressed(KEY_RIGHT))
        {
            currentTexture = (currentTexture + 1)%NUM_TEXTURES; // Cycle between the textures
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawTexture(textures[currentTexture], screenWidth/2 - textures[currentTexture].width/2, screenHeight/2 - textures[currentTexture].height/2, WHITE);

            RLDrawText("Press LEFT MOUSE BUTTON to rotate the image clockwise", 250, 420, 10, DARKGRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    for (int i = 0; i < NUM_TEXTURES; i++) RLUnloadTexture(textures[i]);

    RLCloseWindow();                // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
