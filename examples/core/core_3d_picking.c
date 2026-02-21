/*******************************************************************************************
*
*   raylib [core] example - 3d picking
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 1.3, last time updated with raylib 4.0
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

    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - 3d picking");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 10.0f, 10.0f, 10.0f }; // Camera position
    camera.target = (RLVector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = RL_E_CAMERA_PERSPECTIVE;             // Camera projection type

    RLVector3 cubePosition = { 0.0f, 1.0f, 0.0f };
    RLVector3 cubeSize = { 2.0f, 2.0f, 2.0f };

    RLRay ray = { 0 };                    // Picking line ray
    RLRayCollision collision = { 0 };     // Ray collision hit info

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (RLIsCursorHidden()) RLUpdateCamera(&camera, RL_E_CAMERA_FIRST_PERSON);

        // Toggle camera controls
        if (RLIsMouseButtonPressed(RL_E_MOUSE_BUTTON_RIGHT))
        {
            if (RLIsCursorHidden()) RLEnableCursor();
            else RLDisableCursor();
        }

        if (RLIsMouseButtonPressed(RL_E_MOUSE_BUTTON_LEFT))
        {
            if (!collision.hit)
            {
                ray = RLGetScreenToWorldRay(RLGetMousePosition(), camera);

                // Check collision between ray and box
                collision = RLGetRayCollisionBox(ray,
                            (RLBoundingBox){(RLVector3){ cubePosition.x - cubeSize.x/2, cubePosition.y - cubeSize.y/2, cubePosition.z - cubeSize.z/2 },
                                          (RLVector3){ cubePosition.x + cubeSize.x/2, cubePosition.y + cubeSize.y/2, cubePosition.z + cubeSize.z/2 }});
            }
            else collision.hit = false;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);

                if (collision.hit)
                {
                    RLDrawCube(cubePosition, cubeSize.x, cubeSize.y, cubeSize.z, RED);
                    RLDrawCubeWires(cubePosition, cubeSize.x, cubeSize.y, cubeSize.z, MAROON);

                    RLDrawCubeWires(cubePosition, cubeSize.x + 0.2f, cubeSize.y + 0.2f, cubeSize.z + 0.2f, GREEN);
                }
                else
                {
                    RLDrawCube(cubePosition, cubeSize.x, cubeSize.y, cubeSize.z, GRAY);
                    RLDrawCubeWires(cubePosition, cubeSize.x, cubeSize.y, cubeSize.z, DARKGRAY);
                }

                RLDrawRay(ray, MAROON);
                RLDrawGrid(10, 1.0f);

            RLEndMode3D();

            RLDrawText("Try clicking on the box with your mouse!", 240, 10, 20, DARKGRAY);

            if (collision.hit) RLDrawText("BOX SELECTED", (screenWidth - RLMeasureText("BOX SELECTED", 30))/2, (int)(screenHeight*0.1f), 30, GREEN);

            RLDrawText("Right click mouse to toggle camera controls", 10, 430, 10, GRAY);

            RLDrawFPS(10, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
