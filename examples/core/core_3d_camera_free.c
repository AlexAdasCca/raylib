/*******************************************************************************************
*
*   raylib [core] example - 3d camera free
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 1.3, last time updated with raylib 1.3
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

    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - 3d camera free");

    // Define the camera to look into our 3d world
    RLCamera3D camera = { 0 };
    camera.position = (RLVector3){ 10.0f, 10.0f, 10.0f }; // Camera position
    camera.target = (RLVector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    RLVector3 cubePosition = { 0.0f, 0.0f, 0.0f };

    RLDisableCursor();                    // Limit cursor to relative movement inside the window

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, CAMERA_FREE);

        if (RLIsKeyPressed(KEY_Z)) camera.target = (RLVector3){ 0.0f, 0.0f, 0.0f };
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);

                RLDrawCube(cubePosition, 2.0f, 2.0f, 2.0f, RED);
                RLDrawCubeWires(cubePosition, 2.0f, 2.0f, 2.0f, MAROON);

                RLDrawGrid(10, 1.0f);

            RLEndMode3D();

            RLDrawRectangle( 10, 10, 320, 93, RLFade(SKYBLUE, 0.5f));
            RLDrawRectangleLines( 10, 10, 320, 93, BLUE);

            RLDrawText("Free camera default controls:", 20, 20, 10, BLACK);
            RLDrawText("- Mouse Wheel to Zoom in-out", 40, 40, 10, DARKGRAY);
            RLDrawText("- Mouse Wheel Pressed to Pan", 40, 60, 10, DARKGRAY);
            RLDrawText("- Z to zoom to (0, 0, 0)", 40, 80, 10, DARKGRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
