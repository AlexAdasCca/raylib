/*******************************************************************************************
*
*   raylib [models] example - rotating cube
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 5.6-dev, last time updated with raylib 5.6-dev
*
*   Example contributed by Jopestpe (@jopestpe)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Jopestpe (@jopestpe)
*
********************************************************************************************/

#include "raylib.h"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [models] example - rotating cube");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 0.0f, 3.0f, 3.0f };
    camera.target = (RLVector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = RL_E_CAMERA_PERSPECTIVE;

    // Load image to create texture for the cube
    RLModel model = RLLoadModelFromMesh(RLGenMeshCube(1.0f, 1.0f, 1.0f));
    RLImage img = RLLoadImage("resources/cubicmap_atlas.png");
    RLImage crop = RLImageFromImage(img, (RLRectangle){0, img.height/2.0f, img.width/2.0f, img.height/2.0f});
    RLTexture2D texture = RLLoadTextureFromImage(crop);
    RLUnloadImage(img);
    RLUnloadImage(crop);

    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

    float rotation = 0.0f;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        rotation += 1.0f;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);

                // Draw model defining: position, size, rotation-axis, rotation (degrees), size, and tint-color
                RLDrawModelEx(model, (RLVector3){ 0.0f, 0.0f, 0.0f }, (RLVector3){ 0.5f, 1.0f, 0.0f },
                    rotation, (RLVector3){ 1.0f, 1.0f, 1.0f }, WHITE);

                RLDrawGrid(10, 1.0f);

            RLEndMode3D();

            RLDrawFPS(10, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(texture); // Unload texture
    RLUnloadModel(model);     // Unload model

    RLCloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
