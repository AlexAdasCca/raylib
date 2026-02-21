/*******************************************************************************************
*
*   raylib [models] example - bone socket
*
*   Example complexity rating: [★★★★] 4/4
*
*   Example originally created with raylib 4.5, last time updated with raylib 4.5
*
*   Example contributed by iP (@ipzaur) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2024-2025 iP (@ipzaur)
*
********************************************************************************************/

#include "raylib.h"

#include "raymath.h"

#define BONE_SOCKETS        3
#define BONE_SOCKET_HAT     0
#define BONE_SOCKET_HAND_R  1
#define BONE_SOCKET_HAND_L  2

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [models] example - bone socket");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 5.0f, 5.0f, 5.0f }; // Camera position
    camera.target = (RLVector3){ 0.0f, 2.0f, 0.0f };  // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };      // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                            // Camera field-of-view Y
    camera.projection = RL_E_CAMERA_PERSPECTIVE;         // Camera projection type

    // Load gltf model
    RLModel characterModel = RLLoadModel("resources/models/gltf/greenman.glb"); // Load character model
    RLModel equipModel[BONE_SOCKETS] = {
        RLLoadModel("resources/models/gltf/greenman_hat.glb"),    // Index for the hat model is the same as BONE_SOCKET_HAT
        RLLoadModel("resources/models/gltf/greenman_sword.glb"),  // Index for the sword model is the same as BONE_SOCKET_HAND_R
        RLLoadModel("resources/models/gltf/greenman_shield.glb")  // Index for the shield model is the same as BONE_SOCKET_HAND_L
    };

    bool showEquip[3] = { true, true, true };   // Toggle on/off equip

    // Load gltf model animations
    int animsCount = 0;
    unsigned int animIndex = 0;
    unsigned int animCurrentFrame = 0;
    RLModelAnimation *modelAnimations = RLLoadModelAnimations("resources/models/gltf/greenman.glb", &animsCount);

    // indices of bones for sockets
    int boneSocketIndex[BONE_SOCKETS] = { -1, -1, -1 };

    // search bones for sockets
    for (int i = 0; i < characterModel.boneCount; i++)
    {
        if (RLTextIsEqual(characterModel.bones[i].name, "socket_hat"))
        {
            boneSocketIndex[BONE_SOCKET_HAT] = i;
            continue;
        }

        if (RLTextIsEqual(characterModel.bones[i].name, "socket_hand_R"))
        {
            boneSocketIndex[BONE_SOCKET_HAND_R] = i;
            continue;
        }

        if (RLTextIsEqual(characterModel.bones[i].name, "socket_hand_L"))
        {
            boneSocketIndex[BONE_SOCKET_HAND_L] = i;
            continue;
        }
    }

    RLVector3 position = { 0.0f, 0.0f, 0.0f }; // Set model position
    unsigned short angle = 0;           // Set angle for rotate character

    RLDisableCursor();                    // Limit cursor to relative movement inside the window

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, RL_E_CAMERA_THIRD_PERSON);

        // Rotate character
        if (RLIsKeyDown(RL_E_KEY_F)) angle = (angle + 1)%360;
        else if (RLIsKeyDown(RL_E_KEY_H)) angle = (360 + angle - 1)%360;

        // Select current animation
        if (RLIsKeyPressed(RL_E_KEY_T)) animIndex = (animIndex + 1)%animsCount;
        else if (RLIsKeyPressed(RL_E_KEY_G)) animIndex = (animIndex + animsCount - 1)%animsCount;

        // Toggle shown of equip
        if (RLIsKeyPressed(RL_E_KEY_ONE)) showEquip[BONE_SOCKET_HAT] = !showEquip[BONE_SOCKET_HAT];
        if (RLIsKeyPressed(RL_E_KEY_TWO)) showEquip[BONE_SOCKET_HAND_R] = !showEquip[BONE_SOCKET_HAND_R];
        if (RLIsKeyPressed(RL_E_KEY_THREE)) showEquip[BONE_SOCKET_HAND_L] = !showEquip[BONE_SOCKET_HAND_L];

        // Update model animation
        RLModelAnimation anim = modelAnimations[animIndex];
        animCurrentFrame = (animCurrentFrame + 1)%anim.frameCount;
        RLUpdateModelAnimation(characterModel, anim, animCurrentFrame);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);
                // Draw character
                RLQuaternion characterRotate = RLQuaternionFromAxisAngle((RLVector3){ 0.0f, 1.0f, 0.0f }, angle*DEG2RAD);
                characterModel.transform = RLMatrixMultiply(RLQuaternionToMatrix(characterRotate), RLMatrixTranslate(position.x, position.y, position.z));
                RLUpdateModelAnimation(characterModel, anim, animCurrentFrame);
                RLDrawMesh(characterModel.meshes[0], characterModel.materials[1], characterModel.transform);

                // Draw equipments (hat, sword, shield)
                for (int i = 0; i < BONE_SOCKETS; i++)
                {
                    if (!showEquip[i]) continue;

                    RLTransform *transform = &anim.framePoses[animCurrentFrame][boneSocketIndex[i]];
                    RLQuaternion inRotation = characterModel.bindPose[boneSocketIndex[i]].rotation;
                    RLQuaternion outRotation = transform->rotation;

                    // Calculate socket rotation (angle between bone in initial pose and same bone in current animation frame)
                    RLQuaternion rotate = RLQuaternionMultiply(outRotation, RLQuaternionInvert(inRotation));
                    RLMatrix matrixTransform = RLQuaternionToMatrix(rotate);
                    // Translate socket to its position in the current animation
                    matrixTransform = RLMatrixMultiply(matrixTransform, RLMatrixTranslate(transform->translation.x, transform->translation.y, transform->translation.z));
                    // Transform the socket using the transform of the character (angle and translate)
                    matrixTransform = RLMatrixMultiply(matrixTransform, characterModel.transform);

                    // Draw mesh at socket position with socket angle rotation
                    RLDrawMesh(equipModel[i].meshes[0], equipModel[i].materials[1], matrixTransform);
                }

                RLDrawGrid(10, 1.0f);
            RLEndMode3D();

            RLDrawText("Use the T/G to switch animation", 10, 10, 20, GRAY);
            RLDrawText("Use the F/H to rotate character left/right", 10, 35, 20, GRAY);
            RLDrawText("Use the 1,2,3 to toggle shown of hat, sword and shield", 10, 60, 20, GRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadModelAnimations(modelAnimations, animsCount);
    RLUnloadModel(characterModel);         // Unload character model and meshes/material

    // Unload equipment model and meshes/material
    for (int i = 0; i < BONE_SOCKETS; i++) RLUnloadModel(equipModel[i]);

    RLCloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}