/*******************************************************************************************
*
*   raylib [text] example - font sdf
*
*   Example complexity rating: [★★★☆] 3/4
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

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

#include <stdlib.h>

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [text] example - font sdf");

    // NOTE: Textures/Fonts MUST be loaded after Window initialization (OpenGL context is required)

    const char msg[50] = "Signed Distance Fields";

    // Loading file to memory
    int fileSize = 0;
    unsigned char *fileData = RLLoadFileData("resources/anonymous_pro_bold.ttf", &fileSize);

    // Default font generation from TTF font
    RLFont fontDefault = { 0 };
    fontDefault.baseSize = 16;
    fontDefault.glyphCount = 95;

    // Loading font data from memory data
    // Parameters > font size: 16, no glyphs array provided (0), glyphs count: 95 (autogenerate chars array)
    fontDefault.glyphs = RLLoadFontData(fileData, fileSize, 16, 0, 95, RL_E_FONT_DEFAULT, &fontDefault.glyphCount);
    // Parameters > glyphs count: 95, font size: 16, glyphs padding in image: 4 px, pack method: 0 (default)
    RLImage atlas = RLGenImageFontAtlas(fontDefault.glyphs, &fontDefault.recs, 95, 16, 4, 0);
    fontDefault.texture = RLLoadTextureFromImage(atlas);
    RLUnloadImage(atlas);

    // SDF font generation from TTF font
    RLFont fontSDF = { 0 };
    fontSDF.baseSize = 16;
    fontSDF.glyphCount = 95;
    // Parameters > font size: 16, no glyphs array provided (0), glyphs count: 0 (defaults to 95)
    fontSDF.glyphs = RLLoadFontData(fileData, fileSize, 16, 0, 0, RL_E_FONT_SDF, &
    fontSDF.glyphCount);
    // Parameters > glyphs count: 95, font size: 16, glyphs padding in image: 0 px, pack method: 1 (Skyline algorythm)
    atlas = RLGenImageFontAtlas(fontSDF.glyphs, &fontSDF.recs, 95, 16, 0, 1);
    fontSDF.texture = RLLoadTextureFromImage(atlas);
    RLUnloadImage(atlas);

    RLUnloadFileData(fileData);      // Free memory from loaded file

    // Load SDF required shader (we use default vertex shader)
    RLShader shader = RLLoadShader(0, RLTextFormat("resources/shaders/glsl%i/sdf.fs", GLSL_VERSION));
    RLSetTextureFilter(fontSDF.texture, RL_E_TEXTURE_FILTER_BILINEAR);    // Required for SDF font

    RLVector2 fontPosition = { 40, screenHeight/2.0f - 50 };
    RLVector2 textSize = { 0.0f, 0.0f };
    float fontSize = 16.0f;
    int currentFont = 0;            // 0 - fontDefault, 1 - fontSDF

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        fontSize += RLGetMouseWheelMove()*8.0f;

        if (fontSize < 6) fontSize = 6;

        if (RLIsKeyDown(RL_E_KEY_SPACE)) currentFont = 1;
        else currentFont = 0;

        if (currentFont == 0) textSize = RLMeasureTextEx(fontDefault, msg, fontSize, 0);
        else textSize = RLMeasureTextEx(fontSDF, msg, fontSize, 0);

        fontPosition.x = (float)RLGetScreenWidth()/2 - textSize.x/2;
        fontPosition.y = (float)RLGetScreenHeight()/2 - textSize.y/2 + 80;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            if (currentFont == 1)
            {
                // NOTE: SDF fonts require a custom SDf shader to compute fragment color
                RLBeginShaderMode(shader);    // Activate SDF font shader
                    RLDrawTextEx(fontSDF, msg, fontPosition, fontSize, 0, BLACK);
                RLEndShaderMode();            // Activate our default shader for next drawings

                RLDrawTexture(fontSDF.texture, 10, 10, BLACK);
            }
            else
            {
                RLDrawTextEx(fontDefault, msg, fontPosition, fontSize, 0, BLACK);
                RLDrawTexture(fontDefault.texture, 10, 10, BLACK);
            }

            if (currentFont == 1) RLDrawText("SDF!", 320, 20, 80, RED);
            else RLDrawText("default font", 315, 40, 30, GRAY);

            RLDrawText("FONT SIZE: 16.0", RLGetScreenWidth() - 240, 20, 20, DARKGRAY);
            RLDrawText(RLTextFormat("RENDER SIZE: %02.02f", fontSize), RLGetScreenWidth() - 240, 50, 20, DARKGRAY);
            RLDrawText("Use MOUSE WHEEL to SCALE TEXT!", RLGetScreenWidth() - 240, 90, 10, DARKGRAY);

            RLDrawText("HOLD SPACE to USE SDF FONT VERSION!", 340, RLGetScreenHeight() - 30, 20, MAROON);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadFont(fontDefault);    // Default font unloading
    RLUnloadFont(fontSDF);        // SDF font unloading

    RLUnloadShader(shader);       // Unload SDF shader

    RLCloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}