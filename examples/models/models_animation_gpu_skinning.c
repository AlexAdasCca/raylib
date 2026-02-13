/*******************************************************************************************
*
*   raylib [models] example - animation gpu skinning
*
*   Example complexity rating: [★★★☆] 3/4
*
*   Example originally created with raylib 4.5, last time updated with raylib 4.5
*
*   Example contributed by Daniel Holden (@orangeduck) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2024-2025 Daniel Holden (@orangeduck)
*
*   Note: Due to limitations in the Apple OpenGL driver, this feature does not work on MacOS
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

    RLInitWindow(screenWidth, screenHeight, "raylib [models] example - animation gpu skinning");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 5.0f, 5.0f, 5.0f }; // Camera position
    camera.target = (RLVector3){ 0.0f, 2.0f, 0.0f };  // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };      // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                            // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;         // Camera projection type

    // Load gltf model
    RLModel characterModel = RLLoadModel("resources/models/gltf/greenman.glb"); // Load character model

    // Load skinning shader
    RLShader skinningShader = RLLoadShader(RLTextFormat("resources/shaders/glsl%i/skinning.vs", GLSL_VERSION),
                                       RLTextFormat("resources/shaders/glsl%i/skinning.fs", GLSL_VERSION));

    characterModel.materials[1].shader = skinningShader;

    // Load gltf model animations
    int animsCount = 0;
    unsigned int animIndex = 0;
    unsigned int animCurrentFrame = 0;
    RLModelAnimation *modelAnimations = RLLoadModelAnimations("resources/models/gltf/greenman.glb", &animsCount);

    RLVector3 position = { 0.0f, 0.0f, 0.0f }; // Set model position

    RLDisableCursor();                    // Limit cursor to relative movement inside the window

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, CAMERA_THIRD_PERSON);

        // Select current animation
        if (RLIsKeyPressed(KEY_T)) animIndex = (animIndex + 1)%animsCount;
        else if (RLIsKeyPressed(KEY_G)) animIndex = (animIndex + animsCount - 1)%animsCount;

        // Update model animation
        RLModelAnimation anim = modelAnimations[animIndex];
        animCurrentFrame = (animCurrentFrame + 1)%anim.frameCount;
        characterModel.transform = MatrixTranslate(position.x, position.y, position.z);
        RLUpdateModelAnimationBones(characterModel, anim, animCurrentFrame);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);

                // Draw character mesh, pose calculation is done in shader (GPU skinning)
                RLDrawMesh(characterModel.meshes[0], characterModel.materials[1], characterModel.transform);

                RLDrawGrid(10, 1.0f);

            RLEndMode3D();

            RLDrawText("Use the T/G to switch animation", 10, 10, 20, GRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadModelAnimations(modelAnimations, animsCount); // Unload model animation
    RLUnloadModel(characterModel);    // Unload model and meshes/material
    RLUnloadShader(skinningShader);   // Unload GPU skinning shader

    RLCloseWindow();                  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}