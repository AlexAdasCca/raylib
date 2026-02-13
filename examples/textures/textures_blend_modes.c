/*******************************************************************************************
*
*   raylib [textures] example - blend modes
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   NOTE: Images are loaded in CPU memory (RAM); textures are loaded in GPU memory (VRAM)
*
*   Example originally created with raylib 3.5, last time updated with raylib 3.5
*
*   Example contributed by Karlo Licudine (@accidentalrebel) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2020-2025 Karlo Licudine (@accidentalrebel)
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

    RLInitWindow(screenWidth, screenHeight, "raylib [textures] example - blend modes");

    // NOTE: Textures MUST be loaded after Window initialization (OpenGL context is required)
    RLImage bgImage = RLLoadImage("resources/cyberpunk_street_background.png");     // Loaded in CPU memory (RAM)
    RLTexture2D bgTexture = RLLoadTextureFromImage(bgImage);          // Image converted to texture, GPU memory (VRAM)

    RLImage fgImage = RLLoadImage("resources/cyberpunk_street_foreground.png");     // Loaded in CPU memory (RAM)
    RLTexture2D fgTexture = RLLoadTextureFromImage(fgImage);          // Image converted to texture, GPU memory (VRAM)

    // Once image has been converted to texture and uploaded to VRAM, it can be unloaded from RAM
    RLUnloadImage(bgImage);
    RLUnloadImage(fgImage);

    const int blendCountMax = 4;
    RLBlendMode blendMode = 0;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (RLIsKeyPressed(KEY_SPACE))
        {
            if (blendMode >= (blendCountMax - 1)) blendMode = 0;
            else blendMode++;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawTexture(bgTexture, screenWidth/2 - bgTexture.width/2, screenHeight/2 - bgTexture.height/2, WHITE);

            // Apply the blend mode and then draw the foreground texture
            RLBeginBlendMode(blendMode);
                RLDrawTexture(fgTexture, screenWidth/2 - fgTexture.width/2, screenHeight/2 - fgTexture.height/2, WHITE);
            RLEndBlendMode();

            // Draw the texts
            RLDrawText("Press SPACE to change blend modes.", 310, 350, 10, GRAY);

            switch (blendMode)
            {
                case BLEND_ALPHA: RLDrawText("Current: BLEND_ALPHA", (screenWidth/2) - 60, 370, 10, GRAY); break;
                case BLEND_ADDITIVE: RLDrawText("Current: BLEND_ADDITIVE", (screenWidth/2) - 60, 370, 10, GRAY); break;
                case BLEND_MULTIPLIED: RLDrawText("Current: BLEND_MULTIPLIED", (screenWidth/2) - 60, 370, 10, GRAY); break;
                case BLEND_ADD_COLORS: RLDrawText("Current: BLEND_ADD_COLORS", (screenWidth/2) - 60, 370, 10, GRAY); break;
                default: break;
            }

            RLDrawText("(c) Cyberpunk Street Environment by Luis Zuno (@ansimuz)", screenWidth - 330, screenHeight - 20, 10, GRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(fgTexture); // Unload foreground texture
    RLUnloadTexture(bgTexture); // Unload background texture

    RLCloseWindow();            // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
