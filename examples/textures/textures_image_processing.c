/*******************************************************************************************
*
*   raylib [textures] example - image processing
*
*   Example complexity rating: [★★★☆] 3/4
*
*   NOTE: Images are loaded in CPU memory (RAM); textures are loaded in GPU memory (VRAM)
*
*   Example originally created with raylib 1.4, last time updated with raylib 3.5
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2016-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#include <stdlib.h>             // Required for: free()

#define NUM_PROCESSES    9

typedef enum {
    NONE = 0,
    COLOR_GRAYSCALE,
    COLOR_TINT,
    COLOR_INVERT,
    COLOR_CONTRAST,
    COLOR_BRIGHTNESS,
    GAUSSIAN_BLUR,
    FLIP_VERTICAL,
    FLIP_HORIZONTAL
} ImageProcess;

static const char *processText[] = {
    "NO PROCESSING",
    "COLOR GRAYSCALE",
    "COLOR TINT",
    "COLOR INVERT",
    "COLOR CONTRAST",
    "COLOR BRIGHTNESS",
    "GAUSSIAN BLUR",
    "FLIP VERTICAL",
    "FLIP HORIZONTAL"
};

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [textures] example - image processing");

    // NOTE: Textures MUST be loaded after Window initialization (OpenGL context is required)

    RLImage imOrigin = RLLoadImage("resources/parrots.png");   // Loaded in CPU memory (RAM)
    RLImageFormat(&imOrigin, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);         // Format image to RGBA 32bit (required for texture update) <-- ISSUE
    RLTexture2D texture = RLLoadTextureFromImage(imOrigin);    // Image converted to texture, GPU memory (VRAM)

    RLImage imCopy = RLImageCopy(imOrigin);

    int currentProcess = NONE;
    bool textureReload = false;

    RLRectangle toggleRecs[NUM_PROCESSES] = { 0 };
    int mouseHoverRec = -1;

    for (int i = 0; i < NUM_PROCESSES; i++) toggleRecs[i] = (RLRectangle){ 40.0f, (float)(50 + 32*i), 150.0f, 30.0f };

    RLSetTargetFPS(60);
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------

        // Mouse toggle group logic
        for (int i = 0; i < NUM_PROCESSES; i++)
        {
            if (RLCheckCollisionPointRec(RLGetMousePosition(), toggleRecs[i]))
            {
                mouseHoverRec = i;

                if (RLIsMouseButtonReleased(MOUSE_BUTTON_LEFT))
                {
                    currentProcess = i;
                    textureReload = true;
                }
                break;
            }
            else mouseHoverRec = -1;
        }

        // Keyboard toggle group logic
        if (RLIsKeyPressed(KEY_DOWN))
        {
            currentProcess++;
            if (currentProcess > (NUM_PROCESSES - 1)) currentProcess = 0;
            textureReload = true;
        }
        else if (RLIsKeyPressed(KEY_UP))
        {
            currentProcess--;
            if (currentProcess < 0) currentProcess = 7;
            textureReload = true;
        }

        // Reload texture when required
        if (textureReload)
        {
            RLUnloadImage(imCopy);                // Unload image-copy data
            imCopy = RLImageCopy(imOrigin);     // Restore image-copy from image-origin

            // NOTE: Image processing is a costly CPU process to be done every frame,
            // If image processing is required in a frame-basis, it should be done
            // with a texture and by shaders
            switch (currentProcess)
            {
                case COLOR_GRAYSCALE: RLImageColorGrayscale(&imCopy); break;
                case COLOR_TINT: RLImageColorTint(&imCopy, GREEN); break;
                case COLOR_INVERT: RLImageColorInvert(&imCopy); break;
                case COLOR_CONTRAST: RLImageColorContrast(&imCopy, -40); break;
                case COLOR_BRIGHTNESS: RLImageColorBrightness(&imCopy, -80); break;
                case GAUSSIAN_BLUR: RLImageBlurGaussian(&imCopy, 10); break;
                case FLIP_VERTICAL: RLImageFlipVertical(&imCopy); break;
                case FLIP_HORIZONTAL: RLImageFlipHorizontal(&imCopy); break;
                default: break;
            }

            RLColor *pixels = RLLoadImageColors(imCopy);    // Load pixel data from image (RGBA 32bit)
            RLUpdateTexture(texture, pixels);             // Update texture with new image data
            RLUnloadImageColors(pixels);                  // Unload pixels data from RAM

            textureReload = false;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawText("IMAGE PROCESSING:", 40, 30, 10, DARKGRAY);

            // Draw rectangles
            for (int i = 0; i < NUM_PROCESSES; i++)
            {
                RLDrawRectangleRec(toggleRecs[i], ((i == currentProcess) || (i == mouseHoverRec)) ? SKYBLUE : LIGHTGRAY);
                RLDrawRectangleLines((int)toggleRecs[i].x, (int) toggleRecs[i].y, (int) toggleRecs[i].width, (int) toggleRecs[i].height, ((i == currentProcess) || (i == mouseHoverRec)) ? BLUE : GRAY);
                RLDrawText( processText[i], (int)( toggleRecs[i].x + toggleRecs[i].width/2 - (float)RLMeasureText(processText[i], 10)/2), (int) toggleRecs[i].y + 11, 10, ((i == currentProcess) || (i == mouseHoverRec)) ? DARKBLUE : DARKGRAY);
            }

            RLDrawTexture(texture, screenWidth - texture.width - 60, screenHeight/2 - texture.height/2, WHITE);
            RLDrawRectangleLines(screenWidth - texture.width - 60, screenHeight/2 - texture.height/2, texture.width, texture.height, BLACK);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(texture);       // Unload texture from VRAM
    RLUnloadImage(imOrigin);        // Unload image-origin from RAM
    RLUnloadImage(imCopy);          // Unload image-copy from RAM

    RLCloseWindow();                // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}