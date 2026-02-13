/*******************************************************************************************
*
*   raylib [textures] example - image kernel
*
*   Example complexity rating: [★★★★] 4/4
*
*   NOTE: Images are loaded in CPU memory (RAM); textures are loaded in GPU memory (VRAM)
*
*   Example contributed by Karim Salem (@kimo-s) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example originally created with raylib 1.3, last time updated with raylib 1.3
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2015-2025 Karim Salem (@kimo-s)
*
********************************************************************************************/

#include "raylib.h"

//------------------------------------------------------------------------------------
// Module Functions Declaration
//------------------------------------------------------------------------------------
static void NormalizeKernel(float *kernel, int size);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [textures] example - image kernel");

    RLImage image = RLLoadImage("resources/cat.png"); // Loaded in CPU memory (RAM)

    float gaussiankernel[] = {
        1.0f, 2.0f, 1.0f,
        2.0f, 4.0f, 2.0f,
        1.0f, 2.0f, 1.0f
    };

    float sobelkernel[] = {
        1.0f, 0.0f, -1.0f,
        2.0f, 0.0f, -2.0f,
        1.0f, 0.0f, -1.0f
    };

    float sharpenkernel[] = {
        0.0f, -1.0f, 0.0f,
       -1.0f, 5.0f, -1.0f,
        0.0f, -1.0f, 0.0f
    };

    NormalizeKernel(gaussiankernel, 9);
    NormalizeKernel(sharpenkernel, 9);
    NormalizeKernel(sobelkernel, 9);

    RLImage catSharpend = RLImageCopy(image);
    RLImageKernelConvolution(&catSharpend, sharpenkernel, 9);

    RLImage catSobel = RLImageCopy(image);
    RLImageKernelConvolution(&catSobel, sobelkernel, 9);

    RLImage catGaussian = RLImageCopy(image);

    for (int i = 0; i < 6; i++)
    {
        RLImageKernelConvolution(&catGaussian, gaussiankernel, 9);
    }

    RLImageCrop(&image, (RLRectangle){ 0, 0, (float)200, (float)450 });
    RLImageCrop(&catGaussian, (RLRectangle){ 0, 0, (float)200, (float)450 });
    RLImageCrop(&catSobel, (RLRectangle){ 0, 0, (float)200, (float)450 });
    RLImageCrop(&catSharpend, (RLRectangle){ 0, 0, (float)200, (float)450 });

    // Images converted to texture, GPU memory (VRAM)
    RLTexture2D texture = RLLoadTextureFromImage(image);
    RLTexture2D catSharpendTexture = RLLoadTextureFromImage(catSharpend);
    RLTexture2D catSobelTexture = RLLoadTextureFromImage(catSobel);
    RLTexture2D catGaussianTexture = RLLoadTextureFromImage(catGaussian);

    // Once images have been converted to texture and uploaded to VRAM,
    // they can be unloaded from RAM
    RLUnloadImage(image);
    RLUnloadImage(catGaussian);
    RLUnloadImage(catSobel);
    RLUnloadImage(catSharpend);

    RLSetTargetFPS(60);     // Set our game to run at 60 frames-per-second
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

            RLDrawTexture(catSharpendTexture, 0, 0, WHITE);
            RLDrawTexture(catSobelTexture, 200, 0, WHITE);
            RLDrawTexture(catGaussianTexture, 400, 0, WHITE);
            RLDrawTexture(texture, 600, 0, WHITE);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(texture);
    RLUnloadTexture(catGaussianTexture);
    RLUnloadTexture(catSobelTexture);
    RLUnloadTexture(catSharpendTexture);

    RLCloseWindow();                // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definition
//------------------------------------------------------------------------------------
static void NormalizeKernel(float *kernel, int size)
{
    float sum = 0.0f;
    for (int i = 0; i < size; i++) sum += kernel[i];

    if (sum != 0.0f)
    {
        for (int i = 0; i < size; i++) kernel[i] /= sum;
    }
}
