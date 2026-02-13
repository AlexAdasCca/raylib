/*******************************************************************************************
*
*   raylib [shapes] example - math angle rotation
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 5.6-dev, last time updated with raylib 5.6
*
*   Example contributed by Kris (@krispy-snacc) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Kris (@krispy-snacc)
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h" // for DEG2RAD

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 720;
    const int screenHeight = 400;

    RLInitWindow(screenWidth, screenHeight, "raylib [shapes] example - math angle rotation");
    RLSetTargetFPS(60);

    RLVector2 center = { screenWidth/2.0f, screenHeight/2.0f };
    const float lineLength = 150.0f;

    // Predefined angles for fixed lines
    int angles[] = { 0, 30, 60, 90 };
    int numAngles = sizeof(angles)/sizeof(angles[0]);

    float totalAngle = 0.0f; // Animated rotation angle
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())
    {
        // Update
        //----------------------------------------------------------------------------------
        totalAngle += 1.0f; // degrees per frame
        if (totalAngle >= 360.0f) totalAngle -= 360.0f;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();
            RLClearBackground(WHITE);

            RLDrawText("Fixed angles + rotating line", 10, 10, 20, LIGHTGRAY);

            // Draw fixed-angle lines with colorful gradient
            for (int i = 0; i < numAngles; i++)
            {
                float rad = angles[i]*DEG2RAD;
                RLVector2 end = { center.x + cosf(rad)*lineLength,
                                center.y + sinf(rad)*lineLength };

                // Gradient color from green → cyan → blue → magenta
                RLColor col;
                switch(i)
                {
                    case 0: col = GREEN; break;
                    case 1: col = ORANGE; break;
                    case 2: col = BLUE; break;
                    case 3: col = MAGENTA; break;
                    default: col = WHITE; break;
                }

                RLDrawLineEx(center, end, 5.0f, col);

                // Draw angle label slightly offset along the line
                RLVector2 textPos = { center.x + cosf(rad)*(lineLength + 20),
                                    center.y + sinf(rad)*(lineLength + 20) };
                RLDrawText(RLTextFormat("%d°", angles[i]), (int)textPos.x, (int)textPos.y, 20, col);
            }

            // Draw animated rotating line with changing color
            float animRad = totalAngle*DEG2RAD;
            RLVector2 animEnd = { center.x + cosf(animRad)*lineLength,
                                center.y + sinf(animRad)*lineLength };

            // Cycle through HSV colors for animated line
            RLColor animCol = RLColorFromHSV(fmodf(totalAngle, 360.0f), 0.8f, 0.9f);
            RLDrawLineEx(center, animEnd, 5.0f, animCol);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();
    //--------------------------------------------------------------------------------------

    return 0;
}
