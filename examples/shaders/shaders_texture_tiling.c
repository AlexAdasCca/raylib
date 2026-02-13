/*******************************************************************************************
*
*   raylib [shaders] example - texture tiling
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example demonstrates how to tile a texture on a 3D model using raylib
*
*   Example originally created with raylib 4.5, last time updated with raylib 4.5
*
*   Example contributed by Luis Almeida (@luis605) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2023-2025 Luis Almeida (@luis605)
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

    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - texture tiling");

    // Define the camera to look into our 3d world
    RLCamera3D camera = { 0 };
    camera.position = (RLVector3){ 4.0f, 4.0f, 4.0f }; // Camera position
    camera.target = (RLVector3){ 0.0f, 0.5f, 0.0f };      // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    // Load a cube model
    RLMesh cube = RLGenMeshCube(1.0f, 1.0f, 1.0f);
    RLModel model = RLLoadModelFromMesh(cube);

    // Load a texture and assign to cube model
    RLTexture2D texture = RLLoadTexture("resources/cubicmap_atlas.png");
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

    // Set the texture tiling using a shader
    float tiling[2] = { 3.0f, 3.0f };
    RLShader shader = RLLoadShader(0, RLTextFormat("resources/shaders/glsl%i/tiling.fs", GLSL_VERSION));
    RLSetShaderValue(shader, RLGetShaderLocation(shader, "tiling"), tiling, SHADER_UNIFORM_VEC2);
    model.materials[0].shader = shader;

    RLDisableCursor();                    // Limit cursor to relative movement inside the window

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, CAMERA_FREE);

        if (RLIsKeyPressed('Z')) camera.target = (RLVector3){ 0.0f, 0.5f, 0.0f };
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);

                RLBeginShaderMode(shader);
                    RLDrawModel(model, (RLVector3){ 0.0f, 0.0f, 0.0f }, 2.0f, WHITE);
                RLEndShaderMode();

                RLDrawGrid(10, 1.0f);

            RLEndMode3D();

            RLDrawText("Use mouse to rotate the camera", 10, 10, 20, DARKGRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadModel(model);         // Unload model
    RLUnloadShader(shader);       // Unload shader
    RLUnloadTexture(texture);     // Unload texture

    RLCloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
