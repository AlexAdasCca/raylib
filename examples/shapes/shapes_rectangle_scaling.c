/*******************************************************************************************
*
*   raylib [shapes] example - rectangle scaling
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 2.5, last time updated with raylib 2.5
*
*   Example contributed by Vlad Adrian (@demizdor) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2018-2025 Vlad Adrian (@demizdor) and Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#define MOUSE_SCALE_MARK_SIZE   12

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [shapes] example - rectangle scaling");

    RLRectangle rec = { 100, 100, 200, 80 };

    RLVector2 mousePosition = { 0 };

    bool mouseScaleReady = false;
    bool mouseScaleMode = false;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        mousePosition = RLGetMousePosition();

        if (RLCheckCollisionPointRec(mousePosition, (RLRectangle){ rec.x + rec.width - MOUSE_SCALE_MARK_SIZE, rec.y + rec.height - MOUSE_SCALE_MARK_SIZE, MOUSE_SCALE_MARK_SIZE, MOUSE_SCALE_MARK_SIZE }))
        {
            mouseScaleReady = true;
            if (RLIsMouseButtonPressed(RL_E_MOUSE_BUTTON_LEFT)) mouseScaleMode = true;
        }
        else mouseScaleReady = false;

        if (mouseScaleMode)
        {
            mouseScaleReady = true;

            rec.width = (mousePosition.x - rec.x);
            rec.height = (mousePosition.y - rec.y);

            // Check minimum rec size
            if (rec.width < MOUSE_SCALE_MARK_SIZE) rec.width = MOUSE_SCALE_MARK_SIZE;
            if (rec.height < MOUSE_SCALE_MARK_SIZE) rec.height = MOUSE_SCALE_MARK_SIZE;

            // Check maximum rec size
            if (rec.width > (RLGetScreenWidth() - rec.x)) rec.width = RLGetScreenWidth() - rec.x;
            if (rec.height > (RLGetScreenHeight() - rec.y)) rec.height = RLGetScreenHeight() - rec.y;

            if (RLIsMouseButtonReleased(RL_E_MOUSE_BUTTON_LEFT)) mouseScaleMode = false;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawText("Scale rectangle dragging from bottom-right corner!", 10, 10, 20, GRAY);

            RLDrawRectangleRec(rec, RLFade(GREEN, 0.5f));

            if (mouseScaleReady)
            {
                RLDrawRectangleLinesEx(rec, 1, RED);
                RLDrawTriangle((RLVector2){ rec.x + rec.width - MOUSE_SCALE_MARK_SIZE, rec.y + rec.height },
                             (RLVector2){ rec.x + rec.width, rec.y + rec.height },
                             (RLVector2){ rec.x + rec.width, rec.y + rec.height - MOUSE_SCALE_MARK_SIZE }, RED);
            }

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}