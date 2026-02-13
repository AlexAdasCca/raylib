/*******************************************************************************************
*
*   raylib [core] example - highdpi demo
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 5.0, last time updated with raylib 5.5
*
*   Example contributed by Jonathan Marler (@marler8997) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Jonathan Marler (@marler8997)
*
********************************************************************************************/

#include "raylib.h"

//------------------------------------------------------------------------------------
// Module Functions Declaration
//------------------------------------------------------------------------------------
static void DrawTextCenter(const char *text, int x, int y, int fontSize, RLColor color);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLSetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE);
    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - highdpi demo");
    RLSetWindowMinSize(450, 450);

    int logicalGridDescY = 120;
    int logicalGridLabelY = logicalGridDescY + 30;
    int logicalGridTop = logicalGridLabelY + 30;
    int logicalGridBottom = logicalGridTop + 80;
    int pixelGridTop = logicalGridBottom - 20;
    int pixelGridBottom = pixelGridTop + 80;
    int pixelGridLabelY = pixelGridBottom + 30;
    int pixelGridDescY = pixelGridLabelY + 30;
    int cellSize = 50;
    float cellSizePx = (float)cellSize;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        int monitorCount = RLGetMonitorCount();

        if ((monitorCount > 1) && RLIsKeyPressed(KEY_N))
        {
            RLSetWindowMonitor((RLGetCurrentMonitor() + 1)%monitorCount);
        }

        int currentMonitor = RLGetCurrentMonitor();
        RLVector2 dpiScale = RLGetWindowScaleDPI();
        cellSizePx = ((float)cellSize)/dpiScale.x;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            int windowCenter = RLGetScreenWidth()/2;
            DrawTextCenter(RLTextFormat("Dpi Scale: %f", dpiScale.x), windowCenter, 30, 40, DARKGRAY);
            DrawTextCenter(RLTextFormat("Monitor: %d/%d ([N] next monitor)", currentMonitor+1, monitorCount), windowCenter, 70, 20, LIGHTGRAY);
            DrawTextCenter(RLTextFormat("Window is %d \"logical points\" wide", RLGetScreenWidth()), windowCenter, logicalGridDescY, 20, ORANGE);

            bool odd = true;
            for (int i = cellSize; i < RLGetScreenWidth(); i += cellSize, odd = !odd)
            {
                if (odd) RLDrawRectangle(i, logicalGridTop, cellSize, logicalGridBottom-logicalGridTop, ORANGE);

                DrawTextCenter(RLTextFormat("%d", i), i, logicalGridLabelY, 10, LIGHTGRAY);
                RLDrawLine(i, logicalGridLabelY + 10, i, logicalGridBottom, GRAY);
            }

            odd = true;
            const int minTextSpace = 30;
            int lastTextX = -minTextSpace;
            for (int i = cellSize; i < RLGetRenderWidth(); i += cellSize, odd = !odd)
            {
                int x = (int)(((float)i)/dpiScale.x);
                if (odd) RLDrawRectangle(x, pixelGridTop, (int)cellSizePx, pixelGridBottom - pixelGridTop, CLITERAL(RLColor){ 0, 121, 241, 100 });

                RLDrawLine(x, pixelGridTop, (int)(((float)i)/dpiScale.x), pixelGridLabelY - 10, GRAY);

                if ((x - lastTextX) >= minTextSpace)
                {
                    DrawTextCenter(RLTextFormat("%d", i), x, pixelGridLabelY, 10, LIGHTGRAY);
                    lastTextX = x;
                }
            }

            DrawTextCenter(RLTextFormat("Window is %d \"physical pixels\" wide", RLGetRenderWidth()), windowCenter, pixelGridDescY, 20, BLUE);

            const char *text = "Can you see this?";
            RLVector2 size = RLMeasureTextEx(RLGetFontDefault(), text, 20, 3);
            RLVector2 pos = (RLVector2){ RLGetScreenWidth() - size.x - 5, RLGetScreenHeight() - size.y - 5 };
            RLDrawTextEx(RLGetFontDefault(), text, pos, 20, 3, LIGHTGRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definition
//------------------------------------------------------------------------------------
static void DrawTextCenter(const char *text, int x, int y, int fontSize, RLColor color)
{
    RLVector2 size = RLMeasureTextEx(RLGetFontDefault(), text, (float)fontSize, 3);
    RLVector2 pos = (RLVector2){ x - size.x/2, y - size.y/2 };
    RLDrawTextEx(RLGetFontDefault(), text, pos, (float)fontSize, 3, color);
}
