/*******************************************************************************************
*
*   raylib [shapes] example - basic shapes
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 1.0, last time updated with raylib 4.2
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2014-2025 Ramon Santamaria (@raysan5)
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

    RLInitWindow(screenWidth, screenHeight, "raylib [shapes] example - basic shapes");

    float rotation = 0.0f;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        rotation += 0.2f;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawText("some basic shapes available on raylib", 20, 20, 20, DARKGRAY);

            // Circle shapes and lines
            RLDrawCircle(screenWidth/5, 120, 35, DARKBLUE);
            RLDrawCircleGradient(screenWidth/5, 220, 60, GREEN, SKYBLUE);
            RLDrawCircleLines(screenWidth/5, 340, 80, DARKBLUE);
            RLDrawEllipse(screenWidth/5, 120, 25, 20, YELLOW);
            RLDrawEllipseLines(screenWidth/5, 120, 30, 25, YELLOW);

            // Rectangle shapes and lines
            RLDrawRectangle(screenWidth/4*2 - 60, 100, 120, 60, RED);
            RLDrawRectangleGradientH(screenWidth/4*2 - 90, 170, 180, 130, MAROON, GOLD);
            RLDrawRectangleLines(screenWidth/4*2 - 40, 320, 80, 60, ORANGE);  // NOTE: Uses QUADS internally, not lines

            // Triangle shapes and lines
            RLDrawTriangle((RLVector2){ screenWidth/4.0f *3.0f, 80.0f },
                         (RLVector2){ screenWidth/4.0f *3.0f - 60.0f, 150.0f },
                         (RLVector2){ screenWidth/4.0f *3.0f + 60.0f, 150.0f }, VIOLET);

            RLDrawTriangleLines((RLVector2){ screenWidth/4.0f*3.0f, 160.0f },
                              (RLVector2){ screenWidth/4.0f*3.0f - 20.0f, 230.0f },
                              (RLVector2){ screenWidth/4.0f*3.0f + 20.0f, 230.0f }, DARKBLUE);

            // Polygon shapes and lines
            RLDrawPoly((RLVector2){ screenWidth/4.0f*3, 330 }, 6, 80, rotation, BROWN);
            RLDrawPolyLines((RLVector2){ screenWidth/4.0f*3, 330 }, 6, 90, rotation, BROWN);
            RLDrawPolyLinesEx((RLVector2){ screenWidth/4.0f*3, 330 }, 6, 85, rotation, 6, BEIGE);

            // NOTE: We draw all LINES based shapes together to optimize internal drawing,
            // this way, all LINES are rendered in a single draw pass
            RLDrawLine(18, 42, screenWidth - 18, 42, BLACK);
        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
