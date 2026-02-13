/*******************************************************************************************
*
*   raylib [core] example - 2d camera mouse zoom
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 4.2, last time updated with raylib 4.2
*
*   Example contributed by Jeffery Myers (@JeffM2501) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2022-2025 Jeffery Myers (@JeffM2501)
*
********************************************************************************************/

#include "raylib.h"

#include "rlgl.h"
#include "raymath.h"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - 2d camera mouse zoom");

    RLCamera2D camera = { 0 };
    camera.zoom = 1.0f;

    int zoomMode = 0;       // 0-Mouse Wheel, 1-Mouse Move

    RLSetTargetFPS(60);       // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (RLIsKeyPressed(KEY_ONE)) zoomMode = 0;
        else if (RLIsKeyPressed(KEY_TWO)) zoomMode = 1;

        // Translate based on mouse right click
        if (RLIsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            RLVector2 delta = RLGetMouseDelta();
            delta = Vector2Scale(delta, -1.0f/camera.zoom);
            camera.target = Vector2Add(camera.target, delta);
        }

        if (zoomMode == 0)
        {
            // Zoom based on mouse wheel
            float wheel = RLGetMouseWheelMove();
            if (wheel != 0)
            {
                // Get the world point that is under the mouse
                RLVector2 mouseWorldPos = RLGetScreenToWorld2D(RLGetMousePosition(), camera);

                // Set the offset to where the mouse is
                camera.offset = RLGetMousePosition();

                // Set the target to match, so that the camera maps the world space point
                // under the cursor to the screen space point under the cursor at any zoom
                camera.target = mouseWorldPos;

                // Zoom increment
                // Uses log scaling to provide consistent zoom speed
                float scale = 0.2f*wheel;
                camera.zoom = Clamp(expf(logf(camera.zoom)+scale), 0.125f, 64.0f);
            }
        }
        else
        {
            // Zoom based on mouse right click
            if (RLIsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            {
                // Get the world point that is under the mouse
                RLVector2 mouseWorldPos = RLGetScreenToWorld2D(RLGetMousePosition(), camera);

                // Set the offset to where the mouse is
                camera.offset = RLGetMousePosition();

                // Set the target to match, so that the camera maps the world space point
                // under the cursor to the screen space point under the cursor at any zoom
                camera.target = mouseWorldPos;
            }

            if (RLIsMouseButtonDown(MOUSE_BUTTON_RIGHT))
            {
                // Zoom increment
                // Uses log scaling to provide consistent zoom speed
                float deltaX = RLGetMouseDelta().x;
                float scale = 0.005f*deltaX;
                camera.zoom = Clamp(expf(logf(camera.zoom)+scale), 0.125f, 64.0f);
            }
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();
            RLClearBackground(RAYWHITE);

            RLBeginMode2D(camera);
                // Draw the 3d grid, rotated 90 degrees and centered around 0,0
                // just so we have something in the XY plane
                rlPushMatrix();
                    rlTranslatef(0, 25*50, 0);
                    rlRotatef(90, 1, 0, 0);
                    RLDrawGrid(100, 50);
                rlPopMatrix();

                // Draw a reference circle
                RLDrawCircle(RLGetScreenWidth()/2, RLGetScreenHeight()/2, 50, MAROON);
            RLEndMode2D();

            // Draw mouse reference
            //Vector2 mousePos = GetWorldToScreen2D(GetMousePosition(), camera)
            RLDrawCircleV(RLGetMousePosition(), 4, DARKGRAY);
            RLDrawTextEx(RLGetFontDefault(), RLTextFormat("[%i, %i]", RLGetMouseX(), RLGetMouseY()),
                Vector2Add(RLGetMousePosition(), (RLVector2){ -44, -24 }), 20, 2, BLACK);

            RLDrawText("[1][2] Select mouse zoom mode (Wheel or Move)", 20, 20, 20, DARKGRAY);
            if (zoomMode == 0) RLDrawText("Mouse left button drag to move, mouse wheel to zoom", 20, 50, 20, DARKGRAY);
            else RLDrawText("Mouse left button drag to move, mouse press and move to zoom", 20, 50, 20, DARKGRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
