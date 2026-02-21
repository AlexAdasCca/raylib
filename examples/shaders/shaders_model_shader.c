/*******************************************************************************************
*
*   raylib [shaders] example - model shader
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
*   Example originally created with raylib 1.3, last time updated with raylib 3.7
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2014-2025 Ramon Santamaria (@raysan5)
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

    RLSetConfigFlags(RL_E_FLAG_MSAA_4X_HINT);      // Enable Multi Sampling Anti Aliasing 4x (if available)

    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - model shader");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 4.0f, 4.0f, 4.0f };    // Camera position
    camera.target = (RLVector3){ 0.0f, 1.0f, -1.0f };     // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = RL_E_CAMERA_PERSPECTIVE;             // Camera projection type

    RLModel model = RLLoadModel("resources/models/watermill.obj");                   // Load OBJ model
    RLTexture2D texture = RLLoadTexture("resources/models/watermill_diffuse.png");   // Load model texture

    // Load shader for model
    // NOTE: Defining 0 (NULL) for vertex shader forces usage of internal default vertex shader
    RLShader shader = RLLoadShader(0, RLTextFormat("resources/shaders/glsl%i/grayscale.fs", GLSL_VERSION));

    model.materials[0].shader = shader;                 // Set shader effect to 3d model
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture; // Bind texture to model

    RLVector3 position = { 0.0f, 0.0f, 0.0f };    // Set model position

    RLDisableCursor();                    // Limit cursor to relative movement inside the window
    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, RL_E_CAMERA_FREE);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);

                RLDrawModel(model, position, 0.2f, WHITE);   // Draw 3d model with texture

                RLDrawGrid(10, 1.0f);     // Draw a grid

            RLEndMode3D();

            RLDrawText("(c) Watermill 3D model by Alberto Cano", screenWidth - 210, screenHeight - 20, 10, GRAY);

            RLDrawFPS(10, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadShader(shader);       // Unload shader
    RLUnloadTexture(texture);     // Unload texture
    RLUnloadModel(model);         // Unload model

    RLCloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}