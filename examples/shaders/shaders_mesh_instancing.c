/*******************************************************************************************
*
*   raylib [shaders] example - mesh instancing
*
*   Example complexity rating: [★★★★] 4/4
*
*   Example originally created with raylib 3.7, last time updated with raylib 4.2
*
*   Example contributed by seanpringle (@seanpringle) and reviewed by Max (@moliad) and Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2020-2025 seanpringle (@seanpringle), Max (@moliad) and Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#include <stdlib.h>         // Required for: calloc(), free()

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

#define MAX_INSTANCES  10000

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - mesh instancing");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ -125.0f, 125.0f, -125.0f };    // Camera position
    camera.target = (RLVector3){ 0.0f, 0.0f, 0.0f };              // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };                  // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                        // Camera field-of-view Y
    camera.projection = RL_E_CAMERA_PERSPECTIVE;                     // Camera projection type

    // Define mesh to be instanced
    RLMesh cube = RLGenMeshCube(1.0f, 1.0f, 1.0f);

    // Define transforms to be uploaded to GPU for instances
    RLMatrix *transforms = (RLMatrix *)RL_CALLOC(MAX_INSTANCES, sizeof(RLMatrix));   // Pre-multiplied transformations passed to rlgl

    // Translate and rotate cubes randomly
    for (int i = 0; i < MAX_INSTANCES; i++)
    {
        RLMatrix translation = RLMatrixTranslate((float)RLGetRandomValue(-50, 50), (float)RLGetRandomValue(-50, 50), (float)RLGetRandomValue(-50, 50));
        RLVector3 axis = RLVector3Normalize((RLVector3){ (float)RLGetRandomValue(0, 360), (float)RLGetRandomValue(0, 360), (float)RLGetRandomValue(0, 360) });
        float angle = (float)RLGetRandomValue(0, 180)*DEG2RAD;
        RLMatrix rotation = RLMatrixRotate(axis, angle);

        transforms[i] = RLMatrixMultiply(rotation, translation);
    }

    // Load lighting shader
    RLShader shader = RLLoadShader(RLTextFormat("resources/shaders/glsl%i/lighting_instancing.vs", GLSL_VERSION),
                               RLTextFormat("resources/shaders/glsl%i/lighting.fs", GLSL_VERSION));
    // Get shader locations
    shader.locs[RL_E_SHADER_LOC_MATRIX_MVP] = RLGetShaderLocation(shader, "mvp");
    shader.locs[RL_E_SHADER_LOC_VECTOR_VIEW] = RLGetShaderLocation(shader, "viewPos");

    // Set shader value: ambient light level
    int ambientLoc = RLGetShaderLocation(shader, "ambient");
    RLSetShaderValue(shader, ambientLoc, (float[4]){ 0.2f, 0.2f, 0.2f, 1.0f }, RL_E_SHADER_UNIFORM_VEC4);

    // Create one light
    CreateLight(LIGHT_DIRECTIONAL, (RLVector3){ 50.0f, 50.0f, 0.0f }, RLVector3Zero(), WHITE, shader);

    // NOTE: We are assigning the intancing shader to material.shader
    // to be used on mesh drawing with DrawMeshInstanced()
    RLMaterial matInstances = RLLoadMaterialDefault();
    matInstances.shader = shader;
    matInstances.maps[MATERIAL_MAP_DIFFUSE].color = RED;

    // Load default material (using raylib intenral default shader) for non-instanced mesh drawing
    // WARNING: Default shader enables vertex color attribute BUT GenMeshCube() does not generate vertex colors, so,
    // when drawing the color attribute is disabled and a default color value is provided as input for thevertex attribute
    RLMaterial matDefault = RLLoadMaterialDefault();
    matDefault.maps[MATERIAL_MAP_DIFFUSE].color = BLUE;

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, RL_E_CAMERA_ORBITAL);

        // Update the light shader with the camera view position
        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        RLSetShaderValue(shader, shader.locs[RL_E_SHADER_LOC_VECTOR_VIEW], cameraPos, RL_E_SHADER_UNIFORM_VEC3);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);

                // Draw cube mesh with default material (BLUE)
                RLDrawMesh(cube, matDefault, RLMatrixTranslate(-10.0f, 0.0f, 0.0f));

                // Draw meshes instanced using material containing instancing shader (RED + lighting),
                // transforms[] for the instances should be provided, they are dynamically
                // updated in GPU every frame, so we can animate the different mesh instances
                RLDrawMeshInstanced(cube, matInstances, transforms, MAX_INSTANCES);

                // Draw cube mesh with default material (BLUE)
                RLDrawMesh(cube, matDefault, RLMatrixTranslate(10.0f, 0.0f, 0.0f));

            RLEndMode3D();

            RLDrawFPS(10, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RL_FREE(transforms);    // Free transforms

    RLCloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
