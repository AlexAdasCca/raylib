/*******************************************************************************************
*
*   raylib [textures] example - image channel
*
*   NOTE: Images are loaded in CPU memory (RAM); textures are loaded in GPU memory (VRAM)
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 5.5, last time updated with raylib 5.5
*
*   Example contributed by Bruno Cabral (@brccabral) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2024-2025 Bruno Cabral (@brccabral) and Ramon Santamaria (@raysan5)
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

    RLInitWindow(screenWidth, screenHeight, "raylib [textures] example - image channel");

    RLImage fudesumiImage = RLLoadImage("resources/fudesumi.png");

    RLImage imageAlpha = RLImageFromChannel(fudesumiImage, 3);
    RLImageAlphaMask(&imageAlpha, imageAlpha);

    RLImage imageRed = RLImageFromChannel(fudesumiImage, 0);
    RLImageAlphaMask(&imageRed, imageAlpha);

    RLImage imageGreen = RLImageFromChannel(fudesumiImage, 1);
    RLImageAlphaMask(&imageGreen, imageAlpha);

    RLImage imageBlue = RLImageFromChannel(fudesumiImage, 2);
    RLImageAlphaMask(&imageBlue, imageAlpha);

    RLImage backgroundImage = RLGenImageChecked(screenWidth, screenHeight, screenWidth/20, screenHeight/20, ORANGE, YELLOW);

    RLTexture2D fudesumiTexture = RLLoadTextureFromImage(fudesumiImage);
    RLTexture2D textureAlpha = RLLoadTextureFromImage(imageAlpha);
    RLTexture2D textureRed = RLLoadTextureFromImage(imageRed);
    RLTexture2D textureGreen = RLLoadTextureFromImage(imageGreen);
    RLTexture2D textureBlue = RLLoadTextureFromImage(imageBlue);
    RLTexture2D backgroundTexture = RLLoadTextureFromImage(backgroundImage);

    RLUnloadImage(fudesumiImage);
    RLUnloadImage(imageAlpha);
    RLUnloadImage(imageRed);
    RLUnloadImage(imageGreen);
    RLUnloadImage(imageBlue);
    RLUnloadImage(backgroundImage);

    RLRectangle fudesumiRec = {0, 0, (float)fudesumiImage.width, (float)fudesumiImage.height};

    RLRectangle fudesumiPos = {50, 10, fudesumiImage.width*0.8f, fudesumiImage.height*0.8f};
    RLRectangle redPos = { 410, 10, fudesumiPos.width/2.0f, fudesumiPos.height/2.0f };
    RLRectangle greenPos = { 600, 10, fudesumiPos.width/2.0f, fudesumiPos.height/2.0f };
    RLRectangle bluePos = { 410, 230, fudesumiPos.width/2.0f, fudesumiPos.height/2.0f };
    RLRectangle alphaPos = { 600, 230, fudesumiPos.width/2.0f, fudesumiPos.height/2.0f };

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // Nothing to update...
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLDrawTexture(backgroundTexture, 0, 0, WHITE);
            RLDrawTexturePro(fudesumiTexture, fudesumiRec, fudesumiPos, (RLVector2) {0, 0}, 0, WHITE);

            RLDrawTexturePro(textureRed, fudesumiRec, redPos, (RLVector2) {0, 0}, 0, RED);
            RLDrawTexturePro(textureGreen, fudesumiRec, greenPos, (RLVector2) {0, 0}, 0, GREEN);
            RLDrawTexturePro(textureBlue, fudesumiRec, bluePos, (RLVector2) {0, 0}, 0, BLUE);
            RLDrawTexturePro(textureAlpha, fudesumiRec, alphaPos, (RLVector2) {0, 0}, 0, WHITE);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(backgroundTexture);
    RLUnloadTexture(fudesumiTexture);
    RLUnloadTexture(textureRed);
    RLUnloadTexture(textureGreen);
    RLUnloadTexture(textureBlue);
    RLUnloadTexture(textureAlpha);

    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
