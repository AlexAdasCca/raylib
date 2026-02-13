/*******************************************************************************************
*
*   raylib [shaders] example - texture outline
*
*   Example complexity rating: [★★★☆] 3/4
*
*   NOTE: This example requires raylib OpenGL 3.3 or ES2 versions for shaders support,
*         OpenGL 1.1 does not support shaders, recompile raylib to OpenGL 3.3 version
*
*   Example originally created with raylib 4.0, last time updated with raylib 4.0
*
*   Example contributed by Serenity Skiff (@GoldenThumbs) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2021-2025 Serenity Skiff (@GoldenThumbs) and Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - texture outline");

    RLTexture2D texture = RLLoadTexture("resources/fudesumi.png");

    RLShader shdrOutline = RLLoadShader(0, RLTextFormat("resources/shaders/glsl%i/outline.fs", GLSL_VERSION));

    float outlineSize = 2.0f;
    float outlineColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };     // Normalized RED color
    float textureSize[2] = { (float)texture.width, (float)texture.height };

    // Get shader locations
    int outlineSizeLoc = RLGetShaderLocation(shdrOutline, "outlineSize");
    int outlineColorLoc = RLGetShaderLocation(shdrOutline, "outlineColor");
    int textureSizeLoc = RLGetShaderLocation(shdrOutline, "textureSize");

    // Set shader values (they can be changed later)
    RLSetShaderValue(shdrOutline, outlineSizeLoc, &outlineSize, SHADER_UNIFORM_FLOAT);
    RLSetShaderValue(shdrOutline, outlineColorLoc, outlineColor, SHADER_UNIFORM_VEC4);
    RLSetShaderValue(shdrOutline, textureSizeLoc, textureSize, SHADER_UNIFORM_VEC2);

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        outlineSize += RLGetMouseWheelMove();
        if (outlineSize < 1.0f) outlineSize = 1.0f;

        RLSetShaderValue(shdrOutline, outlineSizeLoc, &outlineSize, SHADER_UNIFORM_FLOAT);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginShaderMode(shdrOutline);

                RLDrawTexture(texture, RLGetScreenWidth()/2 - texture.width/2, -30, WHITE);

            RLEndShaderMode();

            RLDrawText("Shader-based\ntexture\noutline", 10, 10, 20, GRAY);
            RLDrawText("Scroll mouse wheel to\nchange outline size", 10, 72, 20, GRAY);
            RLDrawText(RLTextFormat("Outline size: %i px", (int)outlineSize), 10, 120, 20, MAROON);

            RLDrawFPS(710, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(texture);
    RLUnloadShader(shdrOutline);

    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
