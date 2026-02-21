/*******************************************************************************************
*
*   raylib [textures] example - image generation
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 1.8, last time updated with raylib 1.8
*
*   Example contributed by Wilhem Barbier (@nounoursheureux) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2017-2025 Wilhem Barbier (@nounoursheureux) and Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#define NUM_TEXTURES  9      // Currently we have 8 generation algorithms but some have multiple purposes (Linear and Square Gradients)

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [textures] example - image generation");

    RLImage verticalGradient = RLGenImageGradientLinear(screenWidth, screenHeight, 0, RED, BLUE);
    RLImage horizontalGradient = RLGenImageGradientLinear(screenWidth, screenHeight, 90, RED, BLUE);
    RLImage diagonalGradient = RLGenImageGradientLinear(screenWidth, screenHeight, 45, RED, BLUE);
    RLImage radialGradient = RLGenImageGradientRadial(screenWidth, screenHeight, 0.0f, WHITE, BLACK);
    RLImage squareGradient = RLGenImageGradientSquare(screenWidth, screenHeight, 0.0f, WHITE, BLACK);
    RLImage checked = RLGenImageChecked(screenWidth, screenHeight, 32, 32, RED, BLUE);
    RLImage whiteNoise = RLGenImageWhiteNoise(screenWidth, screenHeight, 0.5f);
    RLImage perlinNoise = RLGenImagePerlinNoise(screenWidth, screenHeight, 50, 50, 4.0f);
    RLImage cellular = RLGenImageCellular(screenWidth, screenHeight, 32);

    RLTexture2D textures[NUM_TEXTURES] = { 0 };

    textures[0] = RLLoadTextureFromImage(verticalGradient);
    textures[1] = RLLoadTextureFromImage(horizontalGradient);
    textures[2] = RLLoadTextureFromImage(diagonalGradient);
    textures[3] = RLLoadTextureFromImage(radialGradient);
    textures[4] = RLLoadTextureFromImage(squareGradient);
    textures[5] = RLLoadTextureFromImage(checked);
    textures[6] = RLLoadTextureFromImage(whiteNoise);
    textures[7] = RLLoadTextureFromImage(perlinNoise);
    textures[8] = RLLoadTextureFromImage(cellular);

    // Unload image data (CPU RAM)
    RLUnloadImage(verticalGradient);
    RLUnloadImage(horizontalGradient);
    RLUnloadImage(diagonalGradient);
    RLUnloadImage(radialGradient);
    RLUnloadImage(squareGradient);
    RLUnloadImage(checked);
    RLUnloadImage(whiteNoise);
    RLUnloadImage(perlinNoise);
    RLUnloadImage(cellular);

    int currentTexture = 0;

    RLSetTargetFPS(60);
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())
    {
        // Update
        //----------------------------------------------------------------------------------
        if (RLIsMouseButtonPressed(RL_E_MOUSE_BUTTON_LEFT) || RLIsKeyPressed(RL_E_KEY_RIGHT))
        {
            currentTexture = (currentTexture + 1)%NUM_TEXTURES; // Cycle between the textures
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawTexture(textures[currentTexture], 0, 0, WHITE);

            RLDrawRectangle(30, 400, 325, 30, RLFade(SKYBLUE, 0.5f));
            RLDrawRectangleLines(30, 400, 325, 30, RLFade(WHITE, 0.5f));
            RLDrawText("MOUSE LEFT BUTTON to CYCLE PROCEDURAL TEXTURES", 40, 410, 10, WHITE);

            switch (currentTexture)
            {
                case 0: RLDrawText("VERTICAL GRADIENT", 560, 10, 20, RAYWHITE); break;
                case 1: RLDrawText("HORIZONTAL GRADIENT", 540, 10, 20, RAYWHITE); break;
                case 2: RLDrawText("DIAGONAL GRADIENT", 540, 10, 20, RAYWHITE); break;
                case 3: RLDrawText("RADIAL GRADIENT", 580, 10, 20, LIGHTGRAY); break;
                case 4: RLDrawText("SQUARE GRADIENT", 580, 10, 20, LIGHTGRAY); break;
                case 5: RLDrawText("CHECKED", 680, 10, 20, RAYWHITE); break;
                case 6: RLDrawText("WHITE NOISE", 640, 10, 20, RED); break;
                case 7: RLDrawText("PERLIN NOISE", 640, 10, 20, RED); break;
                case 8: RLDrawText("CELLULAR", 670, 10, 20, RAYWHITE); break;
                default: break;
            }

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------

    // Unload textures data (GPU VRAM)
    for (int i = 0; i < NUM_TEXTURES; i++) RLUnloadTexture(textures[i]);

    RLCloseWindow();                // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
