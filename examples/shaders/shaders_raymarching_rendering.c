/*******************************************************************************************
*
*   raylib [shaders] example - raymarching rendering
*
*   Example complexity rating: [★★★★] 4/4
*
*   NOTE: This example requires raylib OpenGL 3.3 for shaders support and only #version 330
*         is currently supported. OpenGL ES 2.0 platforms are not supported at the moment
*
*   Example originally created with raylib 2.0, last time updated with raylib 4.2
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2018-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB -> Not supported at this moment
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

    RLSetConfigFlags(RL_E_FLAG_WINDOW_RESIZABLE);
    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - raymarching rendering");

    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 2.5f, 2.5f, 3.0f };    // Camera position
    camera.target = (RLVector3){ 0.0f, 0.0f, 0.7f };      // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 65.0f;                                // Camera field-of-view Y
    camera.projection = RL_E_CAMERA_PERSPECTIVE;             // Camera projection type

    // Load raymarching shader
    // NOTE: Defining 0 (NULL) for vertex shader forces usage of internal default vertex shader
    RLShader shader = RLLoadShader(0, RLTextFormat("resources/shaders/glsl%i/raymarching.fs", GLSL_VERSION));

    // Get shader locations for required uniforms
    int viewEyeLoc = RLGetShaderLocation(shader, "viewEye");
    int viewCenterLoc = RLGetShaderLocation(shader, "viewCenter");
    int runTimeLoc = RLGetShaderLocation(shader, "runTime");
    int resolutionLoc = RLGetShaderLocation(shader, "resolution");

    float resolution[2] = { (float)screenWidth, (float)screenHeight };
    RLSetShaderValue(shader, resolutionLoc, resolution, RL_E_SHADER_UNIFORM_VEC2);

    float runTime = 0.0f;

    RLDisableCursor();                    // Limit cursor to relative movement inside the window
    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, RL_E_CAMERA_FIRST_PERSON);

        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        float cameraTarget[3] = { camera.target.x, camera.target.y, camera.target.z };

        float deltaTime = RLGetFrameTime();
        runTime += deltaTime;

        // Set shader required uniform values
        RLSetShaderValue(shader, viewEyeLoc, cameraPos, RL_E_SHADER_UNIFORM_VEC3);
        RLSetShaderValue(shader, viewCenterLoc, cameraTarget, RL_E_SHADER_UNIFORM_VEC3);
        RLSetShaderValue(shader, runTimeLoc, &runTime, RL_E_SHADER_UNIFORM_FLOAT);

        // Check if screen is resized
        if (RLIsWindowResized())
        {
            resolution[0] = (float)RLGetScreenWidth();
            resolution[1] = (float)RLGetScreenHeight();
            RLSetShaderValue(shader, resolutionLoc, resolution, RL_E_SHADER_UNIFORM_VEC2);
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            // We only draw a white full-screen rectangle,
            // frame is generated in shader using raymarching
            RLBeginShaderMode(shader);
                RLDrawRectangle(0, 0, RLGetScreenWidth(), RLGetScreenHeight(), WHITE);
            RLEndShaderMode();

            RLDrawText("(c) Raymarching shader by Iñigo Quilez. MIT License.", RLGetScreenWidth() - 280, RLGetScreenHeight() - 20, 10, BLACK);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadShader(shader);           // Unload shader

    RLCloseWindow();                  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
