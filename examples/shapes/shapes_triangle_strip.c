/*******************************************************************************************
*
*   raylib [shapes] example - triangle strip
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 5.6-dev, last time updated with raylib 5.6-dev
*
*   Example contributed by Jopestpe (@jopestpe)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Jopestpe (@jopestpe)
*
********************************************************************************************/

#include "raylib.h"
#include <math.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"                 // Required for GUI controls

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [shapes] example - triangle strip");

    RLVector2 points[122] = { 0 };
    RLVector2 center = { (screenWidth/2.0f) - 125.0f, screenHeight/2.0f };
    float segments = 6.0f;
    float insideRadius = 100.0f;
    float outsideRadius = 150.0f;
    bool outline = true;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        int pointCount = (int)(segments);
        float angleStep = (360.0f/pointCount)*DEG2RAD;

        for (int i = 0, i2 = 0; i < pointCount; i++, i2 += 2)
        {
            float angle1 = i*angleStep;
            points[i2] = (RLVector2){ center.x + cosf(angle1)*insideRadius, center.y + sinf(angle1)*insideRadius };
            float angle2 = angle1 + angleStep/2.0f;
            points[i2 + 1] = (RLVector2){ center.x + cosf(angle2)*outsideRadius, center.y + sinf(angle2)*outsideRadius };
        }

        points[pointCount*2] = points[0];
        points[pointCount*2 + 1] = points[1];
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            for (int i = 0; i < pointCount; i++)
            {
                RLVector2 a = points[i*2];
                RLVector2 b = points[i*2 + 1];
                RLVector2 c = points[i*2 + 2];
                RLVector2 d = points[i*2 + 3];

                float angle1 = i*angleStep;
                RLDrawTriangle(c, b, a, RLColorFromHSV(angle1*RAD2DEG, 1.0f, 1.0f));
                RLDrawTriangle(d, b, c, RLColorFromHSV((angle1 + angleStep/2)*RAD2DEG, 1.0f, 1.0f));

                if (outline)
                {
                    RLDrawTriangleLines(a, b, c, BLACK);
                    RLDrawTriangleLines(c, b, d, BLACK);
                }
            }

            RLDrawLine(580, 0, 580, RLGetScreenHeight(), (RLColor){ 218, 218, 218, 255 });
            RLDrawRectangle(580, 0, RLGetScreenWidth(), RLGetScreenHeight(), (RLColor){ 232, 232, 232, 255 });

            // Draw GUI controls
            //------------------------------------------------------------------------------
            GuiSliderBar((RLRectangle){ 640, 40, 120, 20}, "Segments", RLTextFormat("%.0f", segments), &segments, 6.0f, 60.0f);
            GuiCheckBox((RLRectangle){ 640, 70, 20, 20 }, "Outline", &outline);
            //------------------------------------------------------------------------------

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