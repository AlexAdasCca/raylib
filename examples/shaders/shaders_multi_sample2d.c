/*******************************************************************************************
*
*   raylib [shaders] example - multi sample2d
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   NOTE: This example requires raylib OpenGL 3.3 or ES2 versions for shaders support,
*         OpenGL 1.1 does not support shaders, recompile raylib to OpenGL 3.3 version
*
*   NOTE: Shaders used in this example are #version 330 (OpenGL 3.3), to test this example
*         on OpenGL ES 2.0 platforms (Android, Raspberry Pi, HTML5), use #version 100 shaders
*         raylib comes with shaders ready for both versions, check raylib/shaders install folder
*
*   Example originally created with raylib 3.5, last time updated with raylib 3.5
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2020-2025 Ramon Santamaria (@raysan5)
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

    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - multi sample2d");

    RLImage imRed = RLGenImageColor(800, 450, (RLColor){ 255, 0, 0, 255 });
    RLTexture texRed = RLLoadTextureFromImage(imRed);
    RLUnloadImage(imRed);

    RLImage imBlue = RLGenImageColor(800, 450, (RLColor){ 0, 0, 255, 255 });
    RLTexture texBlue = RLLoadTextureFromImage(imBlue);
    RLUnloadImage(imBlue);

    RLShader shader = RLLoadShader(0, RLTextFormat("resources/shaders/glsl%i/color_mix.fs", GLSL_VERSION));

    // Get an additional sampler2D location to be enabled on drawing
    int texBlueLoc = RLGetShaderLocation(shader, "texture1");

    // Get shader uniform for divider
    int dividerLoc = RLGetShaderLocation(shader, "divider");
    float dividerValue = 0.5f;

    RLSetTargetFPS(60);                           // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())                // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (RLIsKeyDown(RL_E_KEY_RIGHT)) dividerValue += 0.01f;
        else if (RLIsKeyDown(RL_E_KEY_LEFT)) dividerValue -= 0.01f;

        if (dividerValue < 0.0f) dividerValue = 0.0f;
        else if (dividerValue > 1.0f) dividerValue = 1.0f;

        RLSetShaderValue(shader, dividerLoc, &dividerValue, RL_E_SHADER_UNIFORM_FLOAT);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginShaderMode(shader);

                // WARNING: Additional textures (sampler2D) are enabled for ALL draw calls in the batch,
                // but EndShaderMode() forces batch drawing and resets active textures, this way
                // other textures (sampler2D) can be activated on consequent drawings (if required)
                // The downside of this approach is that SetShaderValue() must be called inside the loop,
                // to be set again after every EndShaderMode() reset
                RLSetShaderValueTexture(shader, texBlueLoc, texBlue);

                // We are drawing texRed using default [sampler2D texture0] but
                // an additional texture units is enabled for texBlue [sampler2D texture1]
                RLDrawTexture(texRed, 0, 0, WHITE);

            RLEndShaderMode(); // Texture sampler2D is reseted, needs to be set again for next frame

            RLDrawText("Use KEY_LEFT/KEY_RIGHT to move texture mixing in shader!", 80, RLGetScreenHeight() - 40, 20, RAYWHITE);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadShader(shader);       // Unload shader
    RLUnloadTexture(texRed);      // Unload texture
    RLUnloadTexture(texBlue);     // Unload texture

    RLCloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}