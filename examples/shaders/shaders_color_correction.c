/*******************************************************************************************
*
*   raylib [shaders] example - color correction
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   NOTE: This example requires raylib OpenGL 3.3 or ES2 versions for shaders support,
*         OpenGL 1.1 does not support shaders, recompile raylib to OpenGL 3.3 version
*
*   Example originally created with raylib 5.6, last time updated with raylib 5.6
*
*   Example contributed by Jordi Santonja (@JordSant) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Jordi Santonja (@JordSant)
*
********************************************************************************************/

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"                 // Required for GUI controls

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

#define MAX_TEXTURES 4

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - color correction");

    RLTexture2D texture[MAX_TEXTURES] = {
        RLLoadTexture("resources/parrots.png"),
        RLLoadTexture("resources/cat.png"),
        RLLoadTexture("resources/mandrill.png"),
        RLLoadTexture("resources/fudesumi.png")
    };

    RLShader shdrColorCorrection = RLLoadShader(0, RLTextFormat("resources/shaders/glsl%i/color_correction.fs", GLSL_VERSION));

    int imageIndex = 0;
    int resetButtonClicked = 0;

    float contrast = 0.0f;
    float saturation = 0.0f;
    float brightness = 0.0f;

    // Get shader locations
    int contrastLoc = RLGetShaderLocation(shdrColorCorrection, "contrast");
    int saturationLoc = RLGetShaderLocation(shdrColorCorrection, "saturation");
    int brightnessLoc = RLGetShaderLocation(shdrColorCorrection, "brightness");

    // Set shader values (they can be changed later)
    RLSetShaderValue(shdrColorCorrection, contrastLoc, &contrast, RL_E_SHADER_UNIFORM_FLOAT);
    RLSetShaderValue(shdrColorCorrection, saturationLoc, &saturation, RL_E_SHADER_UNIFORM_FLOAT);
    RLSetShaderValue(shdrColorCorrection, brightnessLoc, &brightness, RL_E_SHADER_UNIFORM_FLOAT);

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // Select texture to draw
        if (RLIsKeyPressed(RL_E_KEY_ONE)) imageIndex = 0;
        else if (RLIsKeyPressed(RL_E_KEY_TWO)) imageIndex = 1;
        else if (RLIsKeyPressed(RL_E_KEY_THREE)) imageIndex = 2;
        else if (RLIsKeyPressed(RL_E_KEY_FOUR)) imageIndex = 3;

        // Reset values to 0
        if (RLIsKeyPressed(RL_E_KEY_R) || resetButtonClicked)
        {
            contrast = 0.0f;
            saturation = 0.0f;
            brightness = 0.0f;
        }

        // Send the values to the shader
        RLSetShaderValue(shdrColorCorrection, contrastLoc, &contrast, RL_E_SHADER_UNIFORM_FLOAT);
        RLSetShaderValue(shdrColorCorrection, saturationLoc, &saturation, RL_E_SHADER_UNIFORM_FLOAT);
        RLSetShaderValue(shdrColorCorrection, brightnessLoc, &brightness, RL_E_SHADER_UNIFORM_FLOAT);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginShaderMode(shdrColorCorrection);

                RLDrawTexture(texture[imageIndex], 580/2 - texture[imageIndex].width/2, RLGetScreenHeight()/2 - texture[imageIndex].height/2, WHITE);

            RLEndShaderMode();

            RLDrawLine(580, 0, 580, RLGetScreenHeight(), (RLColor){ 218, 218, 218, 255 });
            RLDrawRectangle(580, 0, RLGetScreenWidth(), RLGetScreenHeight(), (RLColor){ 232, 232, 232, 255 });

            // Draw UI info text
            RLDrawText("Color Correction", 585, 40, 20, GRAY);

            RLDrawText("Picture", 602, 75, 10, GRAY);
            RLDrawText("Press [1] - [4] to Change Picture", 600, 230, 8, GRAY);
            RLDrawText("Press [R] to Reset Values", 600, 250, 8, GRAY);

            // Draw GUI controls
            //------------------------------------------------------------------------------
            GuiToggleGroup((RLRectangle){ 645, 70, 20, 20 }, "1;2;3;4", &imageIndex);

            GuiSliderBar((RLRectangle){ 645, 100, 120, 20 }, "Contrast", RLTextFormat("%.0f", contrast), &contrast, -100.0f, 100.0f);
            GuiSliderBar((RLRectangle){ 645, 130, 120, 20 }, "Saturation", RLTextFormat("%.0f", saturation), &saturation, -100.0f, 100.0f);
            GuiSliderBar((RLRectangle){ 645, 160, 120, 20 }, "Brightness", RLTextFormat("%.0f", brightness), &brightness, -100.0f, 100.0f);

            resetButtonClicked = GuiButton((RLRectangle){ 645, 190, 40, 20 }, "Reset");
            //------------------------------------------------------------------------------

            RLDrawFPS(710, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    for (int i = 0; i < MAX_TEXTURES; i++) RLUnloadTexture(texture[i]);
    RLUnloadShader(shdrColorCorrection);

    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
