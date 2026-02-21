/*******************************************************************************************
*
*   raylib [models] example - orthographic projection
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 2.0, last time updated with raylib 3.7
*
*   Example contributed by Max Danielsson (@autious) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2018-2025 Max Danielsson (@autious) and Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#define FOVY_PERSPECTIVE    45.0f
#define WIDTH_ORTHOGRAPHIC  10.0f

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [models] example - orthographic projection");

    // Define the camera to look into our 3d world
    RLCamera camera = { { 0.0f, 10.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, FOVY_PERSPECTIVE, RL_E_CAMERA_PERSPECTIVE };

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (RLIsKeyPressed(RL_E_KEY_SPACE))
        {
            if (camera.projection == RL_E_CAMERA_PERSPECTIVE)
            {
                camera.fovy = WIDTH_ORTHOGRAPHIC;
                camera.projection = RL_E_CAMERA_ORTHOGRAPHIC;
            }
            else
            {
                camera.fovy = FOVY_PERSPECTIVE;
                camera.projection = RL_E_CAMERA_PERSPECTIVE;
            }
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);

                RLDrawCube((RLVector3){-4.0f, 0.0f, 2.0f}, 2.0f, 5.0f, 2.0f, RED);
                RLDrawCubeWires((RLVector3){-4.0f, 0.0f, 2.0f}, 2.0f, 5.0f, 2.0f, GOLD);
                RLDrawCubeWires((RLVector3){-4.0f, 0.0f, -2.0f}, 3.0f, 6.0f, 2.0f, MAROON);

                RLDrawSphere((RLVector3){-1.0f, 0.0f, -2.0f}, 1.0f, GREEN);
                RLDrawSphereWires((RLVector3){1.0f, 0.0f, 2.0f}, 2.0f, 16, 16, LIME);

                RLDrawCylinder((RLVector3){4.0f, 0.0f, -2.0f}, 1.0f, 2.0f, 3.0f, 4, SKYBLUE);
                RLDrawCylinderWires((RLVector3){4.0f, 0.0f, -2.0f}, 1.0f, 2.0f, 3.0f, 4, DARKBLUE);
                RLDrawCylinderWires((RLVector3){4.5f, -1.0f, 2.0f}, 1.0f, 1.0f, 2.0f, 6, BROWN);

                RLDrawCylinder((RLVector3){1.0f, 0.0f, -4.0f}, 0.0f, 1.5f, 3.0f, 8, GOLD);
                RLDrawCylinderWires((RLVector3){1.0f, 0.0f, -4.0f}, 0.0f, 1.5f, 3.0f, 8, PINK);

                RLDrawGrid(10, 1.0f);        // Draw a grid

            RLEndMode3D();

            RLDrawText("Press Spacebar to switch camera type", 10, RLGetScreenHeight() - 30, 20, DARKGRAY);

            if (camera.projection == RL_E_CAMERA_ORTHOGRAPHIC) RLDrawText("ORTHOGRAPHIC", 10, 40, 20, BLACK);
            else if (camera.projection == RL_E_CAMERA_PERSPECTIVE) RLDrawText("PERSPECTIVE", 10, 40, 20, BLACK);

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
