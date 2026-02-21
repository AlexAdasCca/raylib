/*******************************************************************************************
*
*   raylib [shaders] example - simple mask
*
*   Example complexity rating: [★★☆☆] 2/4
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
********************************************************************************************
*
*   After a model is loaded it has a default material, this material can be
*   modified in place rather than creating one from scratch...
*   While all of the maps have particular names, they can be used for any purpose
*   except for three maps that are applied as cubic maps (see below)
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"

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

    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - simple mask");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 0.0f, 1.0f, 2.0f };    // Camera position
    camera.target = (RLVector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = RL_E_CAMERA_PERSPECTIVE;             // Camera projection type

    // Define our three models to show the shader on
    RLMesh torus = RLGenMeshTorus(0.3f, 1, 16, 32);
    RLModel model1 = RLLoadModelFromMesh(torus);

    RLMesh cube = RLGenMeshCube(0.8f,0.8f,0.8f);
    RLModel model2 = RLLoadModelFromMesh(cube);

    // Generate model to be shaded just to see the gaps in the other two
    RLMesh sphere = RLGenMeshSphere(1, 16, 16);
    RLModel model3 = RLLoadModelFromMesh(sphere);

    // Load the shader
    RLShader shader = RLLoadShader(0, RLTextFormat("resources/shaders/glsl%i/mask.fs", GLSL_VERSION));

    // Load and apply the diffuse texture (colour map)
    RLTexture texDiffuse = RLLoadTexture("resources/plasma.png");
    model1.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texDiffuse;
    model2.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texDiffuse;

    // Using MATERIAL_MAP_EMISSION as a spare slot to use for 2nd texture
    // NOTE: Don't use MATERIAL_MAP_IRRADIANCE, MATERIAL_MAP_PREFILTER or  MATERIAL_MAP_CUBEMAP as they are bound as cube maps
    RLTexture texMask = RLLoadTexture("resources/mask.png");
    model1.materials[0].maps[RL_E_MATERIAL_MAP_EMISSION].texture = texMask;
    model2.materials[0].maps[RL_E_MATERIAL_MAP_EMISSION].texture = texMask;
    shader.locs[RL_E_SHADER_LOC_MAP_EMISSION] = RLGetShaderLocation(shader, "mask");

    // Frame is incremented each frame to animate the shader
    int shaderFrame = RLGetShaderLocation(shader, "frame");

    // Apply the shader to the two models
    model1.materials[0].shader = shader;
    model2.materials[0].shader = shader;

    int framesCounter = 0;
    RLVector3 rotation = { 0 };           // Model rotation angles

    RLDisableCursor();                    // Limit cursor to relative movement inside the window
    RLSetTargetFPS(60);                   // Set  to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, RL_E_CAMERA_FIRST_PERSON);

        framesCounter++;
        rotation.x += 0.01f;
        rotation.y += 0.005f;
        rotation.z -= 0.0025f;

        // Send frames counter to shader for animation
        RLSetShaderValue(shader, shaderFrame, &framesCounter, RL_E_SHADER_UNIFORM_INT);

        // Rotate one of the models
        model1.transform = RLMatrixRotateXYZ(rotation);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(DARKBLUE);

            RLBeginMode3D(camera);

                RLDrawModel(model1, (RLVector3){ 0.5f, 0.0f, 0.0f }, 1, WHITE);
                RLDrawModelEx(model2, (RLVector3){ -0.5f, 0.0f, 0.0f }, (RLVector3){ 1.0f, 1.0f, 0.0f }, 50, (RLVector3){ 1.0f, 1.0f, 1.0f }, WHITE);
                RLDrawModel(model3,(RLVector3){ 0.0f, 0.0f, -1.5f }, 1, WHITE);
                RLDrawGrid(10, 1.0f);        // Draw a grid

            RLEndMode3D();

            RLDrawRectangle(16, 698, RLMeasureText(RLTextFormat("Frame: %i", framesCounter), 20) + 8, 42, BLUE);
            RLDrawText(RLTextFormat("Frame: %i", framesCounter), 20, 700, 20, WHITE);

            RLDrawFPS(10, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadModel(model1);
    RLUnloadModel(model2);
    RLUnloadModel(model3);

    RLUnloadTexture(texDiffuse);  // Unload default diffuse texture
    RLUnloadTexture(texMask);     // Unload texture mask

    RLUnloadShader(shader);       // Unload shader

    RLCloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
