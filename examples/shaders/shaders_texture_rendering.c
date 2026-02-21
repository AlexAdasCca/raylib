/*******************************************************************************************
*
*   raylib [shaders] example - texture rendering
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 2.0, last time updated with raylib 3.7
*
*   Example contributed by Michał Ciesielski (@ciessielski) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2019-2025 Michał Ciesielski (@ciessielski) and Ramon Santamaria (@raysan5)
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

    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - texture rendering");

    RLImage imBlank = RLGenImageColor(1024, 1024, BLANK);
    RLTexture2D texture = RLLoadTextureFromImage(imBlank);  // Load blank texture to fill on shader
    RLUnloadImage(imBlank);

    // NOTE: Using GLSL 330 shader version, on OpenGL ES 2.0 use GLSL 100 shader version
    RLShader shader = RLLoadShader(0, RLTextFormat("resources/shaders/glsl%i/cubes_panning.fs", GLSL_VERSION));

    float time = 0.0f;
    int timeLoc = RLGetShaderLocation(shader, "uTime");
    RLSetShaderValue(shader, timeLoc, &time, RL_E_SHADER_UNIFORM_FLOAT);

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    // -------------------------------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        time = (float)RLGetTime();
        RLSetShaderValue(shader, timeLoc, &time, RL_E_SHADER_UNIFORM_FLOAT);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginShaderMode(shader);    // Enable our custom shader for next shapes/textures drawings
                RLDrawTexture(texture, 0, 0, WHITE);  // Drawing BLANK texture, all rendering magic happens on shader
            RLEndShaderMode();            // Disable our custom shader, return to default shader

            RLDrawText("BACKGROUND is PAINTED and ANIMATED on SHADER!", 10, 10, 20, MAROON);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadShader(shader);
    RLUnloadTexture(texture);

    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
