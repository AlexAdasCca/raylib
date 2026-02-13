/*******************************************************************************************
*
*   raylib [textures] example - to image
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   NOTE: Images are loaded in CPU memory (RAM); textures are loaded in GPU memory (VRAM)
*
*   Example originally created with raylib 1.3, last time updated with raylib 4.0
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

    RLInitWindow(screenWidth, screenHeight, "raylib [textures] example - to image");

    // NOTE: Textures MUST be loaded after Window initialization (OpenGL context is required)

    RLImage image = RLLoadImage("resources/raylib_logo.png");  // Load image data into CPU memory (RAM)
    RLTexture2D texture = RLLoadTextureFromImage(image);       // Image converted to texture, GPU memory (RAM -> VRAM)
    RLUnloadImage(image);                                    // Unload image data from CPU memory (RAM)

    image = RLLoadImageFromTexture(texture);                 // Load image from GPU texture (VRAM -> RAM)
    RLUnloadTexture(texture);                                // Unload texture from GPU memory (VRAM)

    texture = RLLoadTextureFromImage(image);                 // Recreate texture from retrieved image data (RAM -> VRAM)
    RLUnloadImage(image);                                    // Unload retrieved image data from CPU memory (RAM)

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
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

            RLDrawTexture(texture, screenWidth/2 - texture.width/2, screenHeight/2 - texture.height/2, WHITE);

            RLDrawText("this IS a texture loaded from an image!", 300, 370, 10, GRAY);

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