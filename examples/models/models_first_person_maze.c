/*******************************************************************************************
*
*   raylib [models] example - first person maze
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 2.5, last time updated with raylib 3.5
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2019-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#include <stdlib.h>           // Required for: free()

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [models] example - first person maze");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 0.2f, 0.4f, 0.2f };    // Camera position
    camera.target = (RLVector3){ 0.185f, 0.4f, 0.0f };    // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = RL_E_CAMERA_PERSPECTIVE;             // Camera projection type

    RLImage imMap = RLLoadImage("resources/cubicmap.png");      // Load cubicmap image (RAM)
    RLTexture2D cubicmap = RLLoadTextureFromImage(imMap);       // Convert image to texture to display (VRAM)
    RLMesh mesh = RLGenMeshCubicmap(imMap, (RLVector3){ 1.0f, 1.0f, 1.0f });
    RLModel model = RLLoadModelFromMesh(mesh);

    // NOTE: By default each cube is mapped to one part of texture atlas
    RLTexture2D texture = RLLoadTexture("resources/cubicmap_atlas.png");    // Load map texture
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;    // Set map diffuse texture

    // Get map image data to be used for collision detection
    RLColor *mapPixels = RLLoadImageColors(imMap);
    RLUnloadImage(imMap);             // Unload image from RAM

    RLVector3 mapPosition = { -16.0f, 0.0f, -8.0f };  // Set model position

    RLDisableCursor();                // Limit cursor to relative movement inside the window

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLVector3 oldCamPos = camera.position;    // Store old camera position

        RLUpdateCamera(&camera, RL_E_CAMERA_FIRST_PERSON);

        // Check player collision (we simplify to 2D collision detection)
        RLVector2 playerPos = { camera.position.x, camera.position.z };
        float playerRadius = 0.1f;  // Collision radius (player is modelled as a cilinder for collision)

        int playerCellX = (int)(playerPos.x - mapPosition.x + 0.5f);
        int playerCellY = (int)(playerPos.y - mapPosition.z + 0.5f);

        // Out-of-limits security check
        if (playerCellX < 0) playerCellX = 0;
        else if (playerCellX >= cubicmap.width) playerCellX = cubicmap.width - 1;

        if (playerCellY < 0) playerCellY = 0;
        else if (playerCellY >= cubicmap.height) playerCellY = cubicmap.height - 1;

        // Check map collisions using image data and player position against surrounding cells only
        for (int y = playerCellY - 1; y <= playerCellY + 1; y++)
        {
            // Avoid map accessing out of bounds
            if ((y >= 0) && (y < cubicmap.height))
            {
                for (int x = playerCellX - 1; x <= playerCellX + 1; x++)
                {
                    // NOTE: Collision: Only checking R channel for white pixel
                    if (((x >= 0) && (x < cubicmap.width)) &&
                        (mapPixels[y*cubicmap.width + x].r == 255) &&
                        (RLCheckCollisionCircleRec(playerPos, playerRadius,
                        (RLRectangle){ mapPosition.x - 0.5f + x*1.0f, mapPosition.z - 0.5f + y*1.0f, 1.0f, 1.0f })))
                    {
                        // Collision detected, reset camera position
                        camera.position = oldCamPos;
                    }
                }
            }
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);
                RLDrawModel(model, mapPosition, 1.0f, WHITE);                     // Draw maze map
            RLEndMode3D();

            RLDrawTextureEx(cubicmap, (RLVector2){ RLGetScreenWidth() - cubicmap.width*4.0f - 20, 20.0f }, 0.0f, 4.0f, WHITE);
            RLDrawRectangleLines(RLGetScreenWidth() - cubicmap.width*4 - 20, 20, cubicmap.width*4, cubicmap.height*4, GREEN);

            // Draw player position radar
            RLDrawRectangle(RLGetScreenWidth() - cubicmap.width*4 - 20 + playerCellX*4, 20 + playerCellY*4, 4, 4, RED);

            RLDrawFPS(10, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadImageColors(mapPixels);   // Unload color array

    RLUnloadTexture(cubicmap);        // Unload cubicmap texture
    RLUnloadTexture(texture);         // Unload map texture
    RLUnloadModel(model);             // Unload map model

    RLCloseWindow();                  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
