/*******************************************************************************************
*
*   raylib [textures] example - image text
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 1.8, last time updated with raylib 4.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2017-2025 Ramon Santamaria (@raysan5)
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

    RLInitWindow(screenWidth, screenHeight, "raylib [textures] example - image text");

    RLImage parrots = RLLoadImage("resources/parrots.png"); // Load image in CPU memory (RAM)

    // TTF Font loading with custom generation parameters
    RLFont font = RLLoadFontEx("resources/KAISG.ttf", 64, 0, 0);

    // Draw over image using custom font
    RLImageDrawTextEx(&parrots, font, "[Parrots font drawing]", (RLVector2){ 20.0f, 20.0f }, (float)font.baseSize, 0.0f, RED);

    RLTexture2D texture = RLLoadTextureFromImage(parrots);  // Image converted to texture, uploaded to GPU memory (VRAM)
    RLUnloadImage(parrots);   // Once image has been converted to texture and uploaded to VRAM, it can be unloaded from RAM

    RLVector2 position = { (float)screenWidth/2 - (float)texture.width/2, (float)screenHeight/2 - (float)texture.height/2 - 20 };

    bool showFont = false;

    RLSetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (RLIsKeyDown(KEY_SPACE)) showFont = true;
        else showFont = false;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            if (!showFont)
            {
                // Draw texture with text already drawn inside
                RLDrawTextureV(texture, position, WHITE);

                // Draw text directly using sprite font
                RLDrawTextEx(font, "[Parrots font drawing]", (RLVector2){ position.x + 20,
                           position.y + 20 + 280 }, (float)font.baseSize, 0.0f, WHITE);
            }
            else RLDrawTexture(font.texture, screenWidth/2 - font.texture.width/2, 50, BLACK);

            RLDrawText("PRESS SPACE to SHOW FONT ATLAS USED", 290, 420, 10, DARKGRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(texture);     // Texture unloading

    RLUnloadFont(font);           // Unload custom font

    RLCloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}