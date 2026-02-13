/*******************************************************************************************
*
*   raylib [models] example - heightmap rendering
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 1.8, last time updated with raylib 3.5
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2015-2025 Ramon Santamaria (@raysan5)
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

    RLInitWindow(screenWidth, screenHeight, "raylib [models] example - heightmap rendering");

    // Define our custom camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 18.0f, 21.0f, 18.0f };     // Camera position
    camera.target = (RLVector3){ 0.0f, 0.0f, 0.0f };          // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };              // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                    // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;                 // Camera projection type

    RLImage image = RLLoadImage("resources/heightmap.png");     // Load heightmap image (RAM)
    RLTexture2D texture = RLLoadTextureFromImage(image);        // Convert image to texture (VRAM)

    RLMesh mesh = RLGenMeshHeightmap(image, (RLVector3){ 16, 8, 16 }); // Generate heightmap mesh (RAM and VRAM)
    RLModel model = RLLoadModelFromMesh(mesh);                  // Load model from generated mesh

    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture; // Set map diffuse texture
    RLVector3 mapPosition = { -8.0f, 0.0f, -8.0f };           // Define model position

    RLUnloadImage(image);             // Unload heightmap image from RAM, already uploaded to VRAM

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, CAMERA_ORBITAL);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);

                RLDrawModel(model, mapPosition, 1.0f, RED);

                RLDrawGrid(20, 1.0f);

            RLEndMode3D();

            RLDrawTexture(texture, screenWidth - texture.width - 20, 20, WHITE);
            RLDrawRectangleLines(screenWidth - texture.width - 20, 20, texture.width, texture.height, GREEN);

            RLDrawFPS(10, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(texture);     // Unload texture
    RLUnloadModel(model);         // Unload model

    RLCloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}