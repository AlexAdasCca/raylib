/*******************************************************************************************
*
*   raylib [shaders] example - texture waves
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
*   Example originally created with raylib 2.5, last time updated with raylib 3.7
*
*   Example contributed by Anata (@anatagawa) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2019-2025 Anata (@anatagawa) and Ramon Santamaria (@raysan5)
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

    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - texture waves");

    // Load texture texture to apply shaders
    RLTexture2D texture = RLLoadTexture("resources/space.png");

    // Load shader and setup location points and values
    RLShader shader = RLLoadShader(0, RLTextFormat("resources/shaders/glsl%i/wave.fs", GLSL_VERSION));

    int secondsLoc = RLGetShaderLocation(shader, "seconds");
    int freqXLoc = RLGetShaderLocation(shader, "freqX");
    int freqYLoc = RLGetShaderLocation(shader, "freqY");
    int ampXLoc = RLGetShaderLocation(shader, "ampX");
    int ampYLoc = RLGetShaderLocation(shader, "ampY");
    int speedXLoc = RLGetShaderLocation(shader, "speedX");
    int speedYLoc = RLGetShaderLocation(shader, "speedY");

    // Shader uniform values that can be updated at any time
    float freqX = 25.0f;
    float freqY = 25.0f;
    float ampX = 5.0f;
    float ampY = 5.0f;
    float speedX = 8.0f;
    float speedY = 8.0f;

    float screenSize[2] = { (float)RLGetScreenWidth(), (float)RLGetScreenHeight() };
    RLSetShaderValue(shader, RLGetShaderLocation(shader, "size"), &screenSize, SHADER_UNIFORM_VEC2);
    RLSetShaderValue(shader, freqXLoc, &freqX, SHADER_UNIFORM_FLOAT);
    RLSetShaderValue(shader, freqYLoc, &freqY, SHADER_UNIFORM_FLOAT);
    RLSetShaderValue(shader, ampXLoc, &ampX, SHADER_UNIFORM_FLOAT);
    RLSetShaderValue(shader, ampYLoc, &ampY, SHADER_UNIFORM_FLOAT);
    RLSetShaderValue(shader, speedXLoc, &speedX, SHADER_UNIFORM_FLOAT);
    RLSetShaderValue(shader, speedYLoc, &speedY, SHADER_UNIFORM_FLOAT);

    float seconds = 0.0f;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    // -------------------------------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        seconds += RLGetFrameTime();

        RLSetShaderValue(shader, secondsLoc, &seconds, SHADER_UNIFORM_FLOAT);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginShaderMode(shader);

                RLDrawTexture(texture, 0, 0, WHITE);
                RLDrawTexture(texture, texture.width, 0, WHITE);

            RLEndShaderMode();

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadShader(shader);         // Unload shader
    RLUnloadTexture(texture);       // Unload texture

    RLCloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
