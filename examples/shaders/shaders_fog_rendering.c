/*******************************************************************************************
*
*   raylib [shaders] example - fog rendering
*
*   Example complexity rating: [★★★☆] 3/4
*
*   NOTE: This example requires raylib OpenGL 3.3 or ES2 versions for shaders support,
*         OpenGL 1.1 does not support shaders, recompile raylib to OpenGL 3.3 version
*
*   NOTE: Shaders used in this example are #version 330 (OpenGL 3.3)
*
*   Example originally created with raylib 2.5, last time updated with raylib 3.7
*
*   Example contributed by Chris Camacho (@chriscamacho) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2019-2025 Chris Camacho (@chriscamacho) and Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#include "raymath.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

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

    RLSetConfigFlags(RL_E_FLAG_MSAA_4X_HINT);  // Enable Multi Sampling Anti Aliasing 4x (if available)
    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - fog rendering");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 2.0f, 2.0f, 6.0f };    // Camera position
    camera.target = (RLVector3){ 0.0f, 0.5f, 0.0f };      // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = RL_E_CAMERA_PERSPECTIVE;             // Camera projection type

    // Load models and texture
    RLModel modelA = RLLoadModelFromMesh(RLGenMeshTorus(0.4f, 1.0f, 16, 32));
    RLModel modelB = RLLoadModelFromMesh(RLGenMeshCube(1.0f, 1.0f, 1.0f));
    RLModel modelC = RLLoadModelFromMesh(RLGenMeshSphere(0.5f, 32, 32));
    RLTexture texture = RLLoadTexture("resources/texel_checker.png");

    // Assign texture to default model material
    modelA.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    modelB.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    modelC.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

    // Load shader and set up some uniforms
    RLShader shader = RLLoadShader(RLTextFormat("resources/shaders/glsl%i/lighting.vs", GLSL_VERSION),
                               RLTextFormat("resources/shaders/glsl%i/fog.fs", GLSL_VERSION));
    shader.locs[RL_E_SHADER_LOC_MATRIX_MODEL] = RLGetShaderLocation(shader, "matModel");
    shader.locs[RL_E_SHADER_LOC_VECTOR_VIEW] = RLGetShaderLocation(shader, "viewPos");

    // Ambient light level
    int ambientLoc = RLGetShaderLocation(shader, "ambient");
    RLSetShaderValue(shader, ambientLoc, (float[4]){ 0.2f, 0.2f, 0.2f, 1.0f }, RL_E_SHADER_UNIFORM_VEC4);

    float fogDensity = 0.15f;
    int fogDensityLoc = RLGetShaderLocation(shader, "fogDensity");
    RLSetShaderValue(shader, fogDensityLoc, &fogDensity, RL_E_SHADER_UNIFORM_FLOAT);

    // NOTE: All models share the same shader
    modelA.materials[0].shader = shader;
    modelB.materials[0].shader = shader;
    modelC.materials[0].shader = shader;

    // Using just 1 point lights
    CreateLight(LIGHT_POINT, (RLVector3){ 0, 2, 6 }, RLVector3Zero(), WHITE, shader);

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, RL_E_CAMERA_ORBITAL);

        if (RLIsKeyDown(RL_E_KEY_UP))
        {
            fogDensity += 0.001f;
            if (fogDensity > 1.0f) fogDensity = 1.0f;
        }

        if (RLIsKeyDown(RL_E_KEY_DOWN))
        {
            fogDensity -= 0.001f;
            if (fogDensity < 0.0f) fogDensity = 0.0f;
        }

        RLSetShaderValue(shader, fogDensityLoc, &fogDensity, RL_E_SHADER_UNIFORM_FLOAT);

        // Rotate the torus
        modelA.transform = RLMatrixMultiply(modelA.transform, RLMatrixRotateX(-0.025f));
        modelA.transform = RLMatrixMultiply(modelA.transform, RLMatrixRotateZ(0.012f));

        // Update the light shader with the camera view position
        RLSetShaderValue(shader, shader.locs[RL_E_SHADER_LOC_VECTOR_VIEW], &camera.position.x, RL_E_SHADER_UNIFORM_VEC3);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(GRAY);

            RLBeginMode3D(camera);

                // Draw the three models
                RLDrawModel(modelA, RLVector3Zero(), 1.0f, WHITE);
                RLDrawModel(modelB, (RLVector3){ -2.6f, 0, 0 }, 1.0f, WHITE);
                RLDrawModel(modelC, (RLVector3){ 2.6f, 0, 0 }, 1.0f, WHITE);

                for (int i = -20; i < 20; i += 2) RLDrawModel(modelA,(RLVector3){ (float)i, 0, 2 }, 1.0f, WHITE);

            RLEndMode3D();

            RLDrawText(RLTextFormat("Use KEY_UP/KEY_DOWN to change fog density [%.2f]", fogDensity), 10, 10, 20, RAYWHITE);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadModel(modelA);        // Unload the model A
    RLUnloadModel(modelB);        // Unload the model B
    RLUnloadModel(modelC);        // Unload the model C
    RLUnloadTexture(texture);     // Unload the texture
    RLUnloadShader(shader);       // Unload shader

    RLCloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
