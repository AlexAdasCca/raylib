/*******************************************************************************************
*
*   raylib [textures] example - image drawing
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   NOTE: Images are loaded in CPU memory (RAM); textures are loaded in GPU memory (VRAM)
*
*   Example originally created with raylib 1.4, last time updated with raylib 1.4
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2016-2025 Ramon Santamaria (@raysan5)
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

    RLInitWindow(screenWidth, screenHeight, "raylib [textures] example - image drawing");

    // NOTE: Textures MUST be loaded after Window initialization (OpenGL context is required)

    RLImage cat = RLLoadImage("resources/cat.png");             // Load image in CPU memory (RAM)
    RLImageCrop(&cat, (RLRectangle){ 100, 10, 280, 380 });      // Crop an image piece
    RLImageFlipHorizontal(&cat);                              // Flip cropped image horizontally
    RLImageResize(&cat, 150, 200);                            // Resize flipped-cropped image

    RLImage parrots = RLLoadImage("resources/parrots.png");     // Load image in CPU memory (RAM)

    // Draw one image over the other with a scaling of 1.5f
    RLImageDraw(&parrots, cat, (RLRectangle){ 0, 0, (float)cat.width, (float)cat.height }, (RLRectangle){ 30, 40, cat.width*1.5f, cat.height*1.5f }, WHITE);
    RLImageCrop(&parrots, (RLRectangle){ 0, 50, (float)parrots.width, (float)parrots.height - 100 }); // Crop resulting image

    // Draw on the image with a few image draw methods
    RLImageDrawPixel(&parrots, 10, 10, RAYWHITE);
    RLImageDrawCircleLines(&parrots, 10, 10, 5, RAYWHITE);
    RLImageDrawRectangle(&parrots, 5, 20, 10, 10, RAYWHITE);

    RLUnloadImage(cat);       // Unload image from RAM

    // Load custom font for drawing on image
    RLFont font = RLLoadFont("resources/custom_jupiter_crash.png");

    // Draw over image using custom font
    RLImageDrawTextEx(&parrots, font, "PARROTS & CAT", (RLVector2){ 300, 230 }, (float)font.baseSize, -2, WHITE);

    RLUnloadFont(font);       // Unload custom font (already drawn used on image)

    RLTexture2D texture = RLLoadTextureFromImage(parrots);      // Image converted to texture, uploaded to GPU memory (VRAM)
    RLUnloadImage(parrots);   // Once image has been converted to texture and uploaded to VRAM, it can be unloaded from RAM

    RLSetTargetFPS(60);
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Update your variables here
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawTexture(texture, screenWidth/2 - texture.width/2, screenHeight/2 - texture.height/2 - 40, WHITE);
            RLDrawRectangleLines(screenWidth/2 - texture.width/2, screenHeight/2 - texture.height/2 - 40, texture.width, texture.height, DARKGRAY);

            RLDrawText("We are drawing only one texture from various images composed!", 240, 350, 10, DARKGRAY);
            RLDrawText("Source images have been cropped, scaled, flipped and copied one over the other.", 190, 370, 10, DARKGRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(texture);       // Texture unloading

    RLCloseWindow();                // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
