/*******************************************************************************************
*
*   raylib [core] example - 2d camera
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 1.5, last time updated with raylib 3.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2016-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include <math.h>

#define MAX_BUILDINGS   100

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - 2d camera");

    RLRectangle player = { 400, 280, 40, 40 };
    RLRectangle buildings[MAX_BUILDINGS] = { 0 };
    RLColor buildColors[MAX_BUILDINGS] = { 0 };

    int spacing = 0;

    for (int i = 0; i < MAX_BUILDINGS; i++)
    {
        buildings[i].width = (float)RLGetRandomValue(50, 200);
        buildings[i].height = (float)RLGetRandomValue(100, 800);
        buildings[i].y = screenHeight - 130.0f - buildings[i].height;
        buildings[i].x = -6000.0f + spacing;

        spacing += (int)buildings[i].width;

        buildColors[i] = (RLColor){
            (unsigned char)RLGetRandomValue(200, 240),
            (unsigned char)RLGetRandomValue(200, 240),
            (unsigned char)RLGetRandomValue(200, 250),
            255};
    }

    RLCamera2D camera = { 0 };
    camera.target = (RLVector2){ player.x + 20.0f, player.y + 20.0f };
    camera.offset = (RLVector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // Player movement
        if (RLIsKeyDown(KEY_RIGHT)) player.x += 2;
        else if (RLIsKeyDown(KEY_LEFT)) player.x -= 2;

        // Camera target follows player
        camera.target = (RLVector2){ player.x + 20, player.y + 20 };

        // Camera rotation controls
        if (RLIsKeyDown(KEY_A)) camera.rotation--;
        else if (RLIsKeyDown(KEY_S)) camera.rotation++;

        // Limit camera rotation to 80 degrees (-40 to 40)
        if (camera.rotation > 40) camera.rotation = 40;
        else if (camera.rotation < -40) camera.rotation = -40;

        // Camera zoom controls
        // Uses log scaling to provide consistent zoom speed
        camera.zoom = expf(logf(camera.zoom) + ((float)RLGetMouseWheelMove()*0.1f));

        if (camera.zoom > 3.0f) camera.zoom = 3.0f;
        else if (camera.zoom < 0.1f) camera.zoom = 0.1f;

        // Camera reset (zoom and rotation)
        if (RLIsKeyPressed(KEY_R))
        {
            camera.zoom = 1.0f;
            camera.rotation = 0.0f;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode2D(camera);

                RLDrawRectangle(-6000, 320, 13000, 8000, DARKGRAY);

                for (int i = 0; i < MAX_BUILDINGS; i++) RLDrawRectangleRec(buildings[i], buildColors[i]);

                RLDrawRectangleRec(player, RED);

                RLDrawLine((int)camera.target.x, -screenHeight*10, (int)camera.target.x, screenHeight*10, GREEN);
                RLDrawLine(-screenWidth*10, (int)camera.target.y, screenWidth*10, (int)camera.target.y, GREEN);

            RLEndMode2D();

            RLDrawText("SCREEN AREA", 640, 10, 20, RED);

            RLDrawRectangle(0, 0, screenWidth, 5, RED);
            RLDrawRectangle(0, 5, 5, screenHeight - 10, RED);
            RLDrawRectangle(screenWidth - 5, 5, 5, screenHeight - 10, RED);
            RLDrawRectangle(0, screenHeight - 5, screenWidth, 5, RED);

            RLDrawRectangle( 10, 10, 250, 113, RLFade(SKYBLUE, 0.5f));
            RLDrawRectangleLines( 10, 10, 250, 113, BLUE);

            RLDrawText("Free 2D camera controls:", 20, 20, 10, BLACK);
            RLDrawText("- Right/Left to move player", 40, 40, 10, DARKGRAY);
            RLDrawText("- Mouse Wheel to Zoom in-out", 40, 60, 10, DARKGRAY);
            RLDrawText("- A / S to Rotate", 40, 80, 10, DARKGRAY);
            RLDrawText("- R to reset Zoom and Rotation", 40, 100, 10, DARKGRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
