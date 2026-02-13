/*******************************************************************************************
*
*   raylib [models] example - cubicmap rendering
*
*   Example complexity rating: [★★☆☆] 2/4
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

    RLInitWindow(screenWidth, screenHeight, "raylib [models] example - cubicmap rendering");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 16.0f, 14.0f, 16.0f };     // Camera position
    camera.target = (RLVector3){ 0.0f, 0.0f, 0.0f };          // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };              // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                    // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;                 // Camera projection type

    RLImage image = RLLoadImage("resources/cubicmap.png");      // Load cubicmap image (RAM)
    RLTexture2D cubicmap = RLLoadTextureFromImage(image);       // Convert image to texture to display (VRAM)

    RLMesh mesh = RLGenMeshCubicmap(image, (RLVector3){ 1.0f, 1.0f, 1.0f });
    RLModel model = RLLoadModelFromMesh(mesh);

    // NOTE: By default each cube is mapped to one part of texture atlas
    RLTexture2D texture = RLLoadTexture("resources/cubicmap_atlas.png");    // Load map texture
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;    // Set map diffuse texture

    RLVector3 mapPosition = { -16.0f, 0.0f, -8.0f };          // Set model position

    RLUnloadImage(image);     // Unload cubesmap image from RAM, already uploaded to VRAM

    bool pause = false;     // Pause camera orbital rotation (and zoom)

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (RLIsKeyPressed(KEY_P)) pause = !pause;

        if (!pause) RLUpdateCamera(&camera, CAMERA_ORBITAL);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);

                RLDrawModel(model, mapPosition, 1.0f, WHITE);

            RLEndMode3D();

            RLDrawTextureEx(cubicmap, (RLVector2){ screenWidth - cubicmap.width*4.0f - 20, 20.0f }, 0.0f, 4.0f, WHITE);
            RLDrawRectangleLines(screenWidth - cubicmap.width*4 - 20, 20, cubicmap.width*4, cubicmap.height*4, GREEN);

            RLDrawText("cubicmap image used to", 658, 90, 10, GRAY);
            RLDrawText("generate map 3d model", 658, 104, 10, GRAY);

            RLDrawFPS(10, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(cubicmap);    // Unload cubicmap texture
    RLUnloadTexture(texture);     // Unload map texture
    RLUnloadModel(model);         // Unload map model

    RLCloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
