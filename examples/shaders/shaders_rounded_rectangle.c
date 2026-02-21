/*******************************************************************************************
*
*   raylib [shaders] example - rounded rectangle
*
*   Example complexity rating: [★★★☆] 3/4
*
*   Example originally created with raylib 5.5, last time updated with raylib 5.5
*
*   Example contributed by Anstro Pleuton (@anstropleuton) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Anstro Pleuton (@anstropleuton)
*
********************************************************************************************/

#include "raylib.h"

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Rounded rectangle data
typedef struct {
    RLVector4 cornerRadius; // Individual corner radius (top-left, top-right, bottom-left, bottom-right)

    // Shadow variables
    float shadowRadius;
    RLVector2 shadowOffset;
    float shadowScale;

    // Border variables
    float borderThickness; // Inner-border thickness

    // Shader locations
    int rectangleLoc;
    int radiusLoc;
    int colorLoc;
    int shadowRadiusLoc;
    int shadowOffsetLoc;
    int shadowScaleLoc;
    int shadowColorLoc;
    int borderThicknessLoc;
    int borderColorLoc;
} RoundedRectangle;

//------------------------------------------------------------------------------------
// Module Functions Declaration
//------------------------------------------------------------------------------------
// Create a rounded rectangle and set uniform locations
static RoundedRectangle CreateRoundedRectangle(RLVector4 cornerRadius, float shadowRadius, RLVector2 shadowOffset, float shadowScale, float borderThickness, RLShader shader);

// Update rounded rectangle uniforms
static void UpdateRoundedRectangle(RoundedRectangle rec, RLShader shader);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - rounded rectangle");

    // Load the shader
    RLShader shader = RLLoadShader(RLTextFormat("resources/shaders/glsl%i/base.vs", GLSL_VERSION),
                               RLTextFormat("resources/shaders/glsl%i/rounded_rectangle.fs", GLSL_VERSION));

    // Create a rounded rectangle
    RoundedRectangle roundedRectangle = CreateRoundedRectangle(
        (RLVector4){ 5.0f, 10.0f, 15.0f, 20.0f },     // Corner radius
        20.0f,                                      // Shadow radius
        (RLVector2){ 0.0f, -5.0f },                   // Shadow offset
        0.95f,                                      // Shadow scale
        5.0f,                                       // Border thickness
        shader                                      // Shader
    );

    // Update shader uniforms
    UpdateRoundedRectangle(roundedRectangle, shader);

    const RLColor rectangleColor = BLUE;
    const RLColor shadowColor = DARKBLUE;
    const RLColor borderColor = SKYBLUE;

    RLSetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            // Draw rectangle box with rounded corners using shader
            RLRectangle rec = { 50, 70, 110, 60 };
            RLDrawRectangleLines((int)rec.x - 20, (int)rec.y - 20, (int)rec.width + 40, (int)rec.height + 40, DARKGRAY);
            RLDrawText("Rounded rectangle", (int)rec.x - 20, (int)rec.y - 35, 10, DARKGRAY);

            // Flip Y axis to match shader coordinate system
            rec.y = screenHeight - rec.y - rec.height;
            RLSetShaderValue(shader, roundedRectangle.rectangleLoc, (float[]){ rec.x, rec.y, rec.width, rec.height }, RL_E_SHADER_UNIFORM_VEC4);

            // Only rectangle color
            RLSetShaderValue(shader, roundedRectangle.colorLoc, (float[]) { rectangleColor.r/255.0f, rectangleColor.g/255.0f, rectangleColor.b/255.0f, rectangleColor.a/255.0f }, RL_E_SHADER_UNIFORM_VEC4);
            RLSetShaderValue(shader, roundedRectangle.shadowColorLoc, (float[]) { 0.0f, 0.0f, 0.0f, 0.0f }, RL_E_SHADER_UNIFORM_VEC4);
            RLSetShaderValue(shader, roundedRectangle.borderColorLoc, (float[]) { 0.0f, 0.0f, 0.0f, 0.0f }, RL_E_SHADER_UNIFORM_VEC4);

            RLBeginShaderMode(shader);
                RLDrawRectangle(0, 0, screenWidth, screenHeight, WHITE);
            RLEndShaderMode();

            // Draw rectangle shadow using shader
            rec = (RLRectangle){ 50, 200, 110, 60 };
            RLDrawRectangleLines((int)rec.x - 20, (int)rec.y - 20, (int)rec.width + 40, (int)rec.height + 40, DARKGRAY);
            RLDrawText("Rounded rectangle shadow", (int)rec.x - 20, (int)rec.y - 35, 10, DARKGRAY);

            rec.y = screenHeight - rec.y - rec.height;
            RLSetShaderValue(shader, roundedRectangle.rectangleLoc, (float[]){ rec.x, rec.y, rec.width, rec.height }, RL_E_SHADER_UNIFORM_VEC4);

            // Only shadow color
            RLSetShaderValue(shader, roundedRectangle.colorLoc, (float[]) { 0.0f, 0.0f, 0.0f, 0.0f }, RL_E_SHADER_UNIFORM_VEC4);
            RLSetShaderValue(shader, roundedRectangle.shadowColorLoc, (float[]) { shadowColor.r/255.0f, shadowColor.g/255.0f, shadowColor.b/255.0f, shadowColor.a/255.0f }, RL_E_SHADER_UNIFORM_VEC4);
            RLSetShaderValue(shader, roundedRectangle.borderColorLoc, (float[]) { 0.0f, 0.0f, 0.0f, 0.0f }, RL_E_SHADER_UNIFORM_VEC4);

            RLBeginShaderMode(shader);
                RLDrawRectangle(0, 0, screenWidth, screenHeight, WHITE);
            RLEndShaderMode();

            // Draw rectangle's border using shader
            rec = (RLRectangle){ 50, 330, 110, 60 };
            RLDrawRectangleLines((int)rec.x - 20, (int)rec.y - 20, (int)rec.width + 40, (int)rec.height + 40, DARKGRAY);
            RLDrawText("Rounded rectangle border", (int)rec.x - 20, (int)rec.y - 35, 10, DARKGRAY);

            rec.y = screenHeight - rec.y - rec.height;
            RLSetShaderValue(shader, roundedRectangle.rectangleLoc, (float[]){ rec.x, rec.y, rec.width, rec.height }, RL_E_SHADER_UNIFORM_VEC4);

            // Only border color
            RLSetShaderValue(shader, roundedRectangle.colorLoc, (float[]) { 0.0f, 0.0f, 0.0f, 0.0f }, RL_E_SHADER_UNIFORM_VEC4);
            RLSetShaderValue(shader, roundedRectangle.shadowColorLoc, (float[]) { 0.0f, 0.0f, 0.0f, 0.0f }, RL_E_SHADER_UNIFORM_VEC4);
            RLSetShaderValue(shader, roundedRectangle.borderColorLoc, (float[]) { borderColor.r/255.0f, borderColor.g/255.0f, borderColor.b/255.0f, borderColor.a/255.0f }, RL_E_SHADER_UNIFORM_VEC4);

            RLBeginShaderMode(shader);
                RLDrawRectangle(0, 0, screenWidth, screenHeight, WHITE);
            RLEndShaderMode();

            // Draw one more rectangle with all three colors
            rec = (RLRectangle){ 240, 80, 500, 300 };
            RLDrawRectangleLines((int)rec.x - 30, (int)rec.y - 30, (int)rec.width + 60, (int)rec.height + 60, DARKGRAY);
            RLDrawText("Rectangle with all three combined", (int)rec.x - 30, (int)rec.y - 45, 10, DARKGRAY);

            rec.y = screenHeight - rec.y - rec.height;
            RLSetShaderValue(shader, roundedRectangle.rectangleLoc, (float[]){ rec.x, rec.y, rec.width, rec.height }, RL_E_SHADER_UNIFORM_VEC4);

            // All three colors
            RLSetShaderValue(shader, roundedRectangle.colorLoc, (float[]) { rectangleColor.r/255.0f, rectangleColor.g/255.0f, rectangleColor.b/255.0f, rectangleColor.a/255.0f }, RL_E_SHADER_UNIFORM_VEC4);
            RLSetShaderValue(shader, roundedRectangle.shadowColorLoc, (float[]) { shadowColor.r/255.0f, shadowColor.g/255.0f, shadowColor.b/255.0f, shadowColor.a/255.0f }, RL_E_SHADER_UNIFORM_VEC4);
            RLSetShaderValue(shader, roundedRectangle.borderColorLoc, (float[]) { borderColor.r/255.0f, borderColor.g/255.0f, borderColor.b/255.0f, borderColor.a/255.0f }, RL_E_SHADER_UNIFORM_VEC4);

            RLBeginShaderMode(shader);
                RLDrawRectangle(0, 0, screenWidth, screenHeight, WHITE);
            RLEndShaderMode();

            RLDrawText("(c) Rounded rectangle SDF by Iñigo Quilez. MIT License.", screenWidth - 300, screenHeight - 20, 10, BLACK);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadShader(shader); // Unload shader

    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definitions
//------------------------------------------------------------------------------------

// Create a rounded rectangle and set uniform locations
RoundedRectangle CreateRoundedRectangle(RLVector4 cornerRadius, float shadowRadius, RLVector2 shadowOffset, float shadowScale, float borderThickness, RLShader shader)
{
    RoundedRectangle rec;
    rec.cornerRadius = cornerRadius;
    rec.shadowRadius = shadowRadius;
    rec.shadowOffset = shadowOffset;
    rec.shadowScale = shadowScale;
    rec.borderThickness = borderThickness;

    // Get shader uniform locations
    rec.rectangleLoc = RLGetShaderLocation(shader, "rectangle");
    rec.radiusLoc = RLGetShaderLocation(shader, "radius");
    rec.colorLoc = RLGetShaderLocation(shader, "color");
    rec.shadowRadiusLoc = RLGetShaderLocation(shader, "shadowRadius");
    rec.shadowOffsetLoc = RLGetShaderLocation(shader, "shadowOffset");
    rec.shadowScaleLoc = RLGetShaderLocation(shader, "shadowScale");
    rec.shadowColorLoc = RLGetShaderLocation(shader, "shadowColor");
    rec.borderThicknessLoc = RLGetShaderLocation(shader, "borderThickness");
    rec.borderColorLoc = RLGetShaderLocation(shader, "borderColor");

    UpdateRoundedRectangle(rec, shader);

    return rec;
}

// Update rounded rectangle uniforms
void UpdateRoundedRectangle(RoundedRectangle rec, RLShader shader)
{
    RLSetShaderValue(shader, rec.radiusLoc, (float[]){ rec.cornerRadius.x, rec.cornerRadius.y, rec.cornerRadius.z, rec.cornerRadius.w }, RL_E_SHADER_UNIFORM_VEC4);
    RLSetShaderValue(shader, rec.shadowRadiusLoc, &rec.shadowRadius, RL_E_SHADER_UNIFORM_FLOAT);
    RLSetShaderValue(shader, rec.shadowOffsetLoc, (float[]){ rec.shadowOffset.x, rec.shadowOffset.y }, RL_E_SHADER_UNIFORM_VEC2);
    RLSetShaderValue(shader, rec.shadowScaleLoc, &rec.shadowScale, RL_E_SHADER_UNIFORM_FLOAT);
    RLSetShaderValue(shader, rec.borderThicknessLoc, &rec.borderThickness, RL_E_SHADER_UNIFORM_FLOAT);
}
