/*******************************************************************************************
*
*   raylib [shapes] example - colors palette
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 1.0, last time updated with raylib 2.5
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2014-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#define MAX_COLORS_COUNT    21          // Number of colors available

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [shapes] example - colors palette");

    RLColor colors[MAX_COLORS_COUNT] = {
        DARKGRAY, MAROON, ORANGE, DARKGREEN, DARKBLUE, DARKPURPLE, DARKBROWN,
        GRAY, RED, GOLD, LIME, BLUE, VIOLET, BROWN, LIGHTGRAY, PINK, YELLOW,
        GREEN, SKYBLUE, PURPLE, BEIGE };

    const char *colorNames[MAX_COLORS_COUNT] = {
        "DARKGRAY", "MAROON", "ORANGE", "DARKGREEN", "DARKBLUE", "DARKPURPLE",
        "DARKBROWN", "GRAY", "RED", "GOLD", "LIME", "BLUE", "VIOLET", "BROWN",
        "LIGHTGRAY", "PINK", "YELLOW", "GREEN", "SKYBLUE", "PURPLE", "BEIGE" };

    RLRectangle colorsRecs[MAX_COLORS_COUNT] = { 0 };     // Rectangles array

    // Fills colorsRecs data (for every rectangle)
    for (int i = 0; i < MAX_COLORS_COUNT; i++)
    {
        colorsRecs[i].x = 20.0f + 100.0f *(i%7) + 10.0f *(i%7);
        colorsRecs[i].y = 80.0f + 100.0f *((float)i/7) + 10.0f *((float)i/7);
        colorsRecs[i].width = 100.0f;
        colorsRecs[i].height = 100.0f;
    }

    int colorState[MAX_COLORS_COUNT] = { 0 };           // Color state: 0-DEFAULT, 1-MOUSE_HOVER

    RLVector2 mousePoint = { 0.0f, 0.0f };

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        mousePoint = RLGetMousePosition();

        for (int i = 0; i < MAX_COLORS_COUNT; i++)
        {
            if (RLCheckCollisionPointRec(mousePoint, colorsRecs[i])) colorState[i] = 1;
            else colorState[i] = 0;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawText("raylib colors palette", 28, 42, 20, BLACK);
            RLDrawText("press SPACE to see all colors", RLGetScreenWidth() - 180, RLGetScreenHeight() - 40, 10, GRAY);

            for (int i = 0; i < MAX_COLORS_COUNT; i++)    // Draw all rectangles
            {
                RLDrawRectangleRec(colorsRecs[i], RLFade(colors[i], colorState[i]? 0.6f : 1.0f));

                if (RLIsKeyDown(RL_E_KEY_SPACE) || colorState[i])
                {
                    RLDrawRectangle((int)colorsRecs[i].x, (int)(colorsRecs[i].y + colorsRecs[i].height - 26), (int)colorsRecs[i].width, 20, BLACK);
                    RLDrawRectangleLinesEx(colorsRecs[i], 6, RLFade(BLACK, 0.3f));
                    RLDrawText(colorNames[i], (int)(colorsRecs[i].x + colorsRecs[i].width - RLMeasureText(colorNames[i], 10) - 12),
                        (int)(colorsRecs[i].y + colorsRecs[i].height - 20), 10, colors[i]);
                }
            }

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();                // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}