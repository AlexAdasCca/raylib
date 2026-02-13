/*******************************************************************************************
*
*   raylib [core] example - 2d camera split screen
*
*   Example complexity rating: [★★★★] 4/4
*
*   Addapted from the core_3d_camera_split_screen example:
*       https://github.com/raysan5/raylib/blob/master/examples/core/core_3d_camera_split_screen.c
*
*   Example originally created with raylib 4.5, last time updated with raylib 4.5
*
*   Example contributed by Gabriel dos Santos Sanches (@gabrielssanches) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2023-2025 Gabriel dos Santos Sanches (@gabrielssanches)
*
********************************************************************************************/

#include "raylib.h"

#define PLAYER_SIZE 40

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 440;

    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - 2d camera split screen");

    RLRectangle player1 = { 200, 200, PLAYER_SIZE, PLAYER_SIZE };
    RLRectangle player2 = { 250, 200, PLAYER_SIZE, PLAYER_SIZE };

    RLCamera2D camera1 = { 0 };
    camera1.target = (RLVector2){ player1.x, player1.y };
    camera1.offset = (RLVector2){ 200.0f, 200.0f };
    camera1.rotation = 0.0f;
    camera1.zoom = 1.0f;

    RLCamera2D camera2 = { 0 };
    camera2.target = (RLVector2){ player2.x, player2.y };
    camera2.offset = (RLVector2){ 200.0f, 200.0f };
    camera2.rotation = 0.0f;
    camera2.zoom = 1.0f;

    RLRenderTexture screenCamera1 = RLLoadRenderTexture(screenWidth/2, screenHeight);
    RLRenderTexture screenCamera2 = RLLoadRenderTexture(screenWidth/2, screenHeight);

    // Build a flipped rectangle the size of the split view to use for drawing later
    RLRectangle splitScreenRect = { 0.0f, 0.0f, (float)screenCamera1.texture.width, (float)-screenCamera1.texture.height };

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (RLIsKeyDown(KEY_S)) player1.y += 3.0f;
        else if (RLIsKeyDown(KEY_W)) player1.y -= 3.0f;
        if (RLIsKeyDown(KEY_D)) player1.x += 3.0f;
        else if (RLIsKeyDown(KEY_A)) player1.x -= 3.0f;

        if (RLIsKeyDown(KEY_UP)) player2.y -= 3.0f;
        else if (RLIsKeyDown(KEY_DOWN)) player2.y += 3.0f;
        if (RLIsKeyDown(KEY_RIGHT)) player2.x += 3.0f;
        else if (RLIsKeyDown(KEY_LEFT)) player2.x -= 3.0f;

        camera1.target = (RLVector2){ player1.x, player1.y };
        camera2.target = (RLVector2){ player2.x, player2.y };
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginTextureMode(screenCamera1);
            RLClearBackground(RAYWHITE);

            RLBeginMode2D(camera1);

                // Draw full scene with first camera
                for (int i = 0; i < screenWidth/PLAYER_SIZE + 1; i++)
                {
                    RLDrawLineV((RLVector2){(float)PLAYER_SIZE*i, 0}, (RLVector2){ (float)PLAYER_SIZE*i, (float)screenHeight}, LIGHTGRAY);
                }

                for (int i = 0; i < screenHeight/PLAYER_SIZE + 1; i++)
                {
                    RLDrawLineV((RLVector2){0, (float)PLAYER_SIZE*i}, (RLVector2){ (float)screenWidth, (float)PLAYER_SIZE*i}, LIGHTGRAY);
                }

                for (int i = 0; i < screenWidth/PLAYER_SIZE; i++)
                {
                    for (int j = 0; j < screenHeight/PLAYER_SIZE; j++)
                    {
                        RLDrawText(RLTextFormat("[%i,%i]", i, j), 10 + PLAYER_SIZE*i, 15 + PLAYER_SIZE*j, 10, LIGHTGRAY);
                    }
                }

                RLDrawRectangleRec(player1, RED);
                RLDrawRectangleRec(player2, BLUE);
            RLEndMode2D();

            RLDrawRectangle(0, 0, RLGetScreenWidth()/2, 30, RLFade(RAYWHITE, 0.6f));
            RLDrawText("PLAYER1: W/S/A/D to move", 10, 10, 10, MAROON);

        RLEndTextureMode();

        RLBeginTextureMode(screenCamera2);
            RLClearBackground(RAYWHITE);

            RLBeginMode2D(camera2);

                // Draw full scene with second camera
                for (int i = 0; i < screenWidth/PLAYER_SIZE + 1; i++)
                {
                    RLDrawLineV((RLVector2){ (float)PLAYER_SIZE*i, 0}, (RLVector2){ (float)PLAYER_SIZE*i, (float)screenHeight}, LIGHTGRAY);
                }

                for (int i = 0; i < screenHeight/PLAYER_SIZE + 1; i++)
                {
                    RLDrawLineV((RLVector2){0, (float)PLAYER_SIZE*i}, (RLVector2){ (float)screenWidth, (float)PLAYER_SIZE*i}, LIGHTGRAY);
                }

                for (int i = 0; i < screenWidth/PLAYER_SIZE; i++)
                {
                    for (int j = 0; j < screenHeight/PLAYER_SIZE; j++)
                    {
                        RLDrawText(RLTextFormat("[%i,%i]", i, j), 10 + PLAYER_SIZE*i, 15 + PLAYER_SIZE*j, 10, LIGHTGRAY);
                    }
                }

                RLDrawRectangleRec(player1, RED);
                RLDrawRectangleRec(player2, BLUE);

            RLEndMode2D();

            RLDrawRectangle(0, 0, RLGetScreenWidth()/2, 30, RLFade(RAYWHITE, 0.6f));
            RLDrawText("PLAYER2: UP/DOWN/LEFT/RIGHT to move", 10, 10, 10, DARKBLUE);

        RLEndTextureMode();

        // Draw both views render textures to the screen side by side
        RLBeginDrawing();
            RLClearBackground(BLACK);

            RLDrawTextureRec(screenCamera1.texture, splitScreenRect, (RLVector2){ 0, 0 }, WHITE);
            RLDrawTextureRec(screenCamera2.texture, splitScreenRect, (RLVector2){ screenWidth/2.0f, 0 }, WHITE);

            RLDrawRectangle(RLGetScreenWidth()/2 - 2, 0, 4, RLGetScreenHeight(), LIGHTGRAY);
        RLEndDrawing();
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadRenderTexture(screenCamera1); // Unload render texture
    RLUnloadRenderTexture(screenCamera2); // Unload render texture

    RLCloseWindow();                      // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
