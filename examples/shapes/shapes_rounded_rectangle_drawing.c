/*******************************************************************************************
*
*   raylib [shapes] example - rounded rectangle drawing
*
*   Example complexity rating: [★★★☆] 3/4
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

    RLInitWindow(screenWidth, screenHeight, "raylib [shapes] example - rounded rectangle drawing");

    float roundness = 0.2f;
    float width = 200.0f;
    float height = 100.0f;
    float segments = 0.0f;
    float lineThick = 1.0f;

    bool drawRect = false;
    bool drawRoundedRect = true;
    bool drawRoundedLines = false;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLRectangle rec = { ((float)RLGetScreenWidth() - width - 250)/2, (RLGetScreenHeight() - height)/2.0f, (float)width, (float)height };
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawLine(560, 0, 560, RLGetScreenHeight(), RLFade(LIGHTGRAY, 0.6f));
            RLDrawRectangle(560, 0, RLGetScreenWidth() - 500, RLGetScreenHeight(), RLFade(LIGHTGRAY, 0.3f));

            if (drawRect) RLDrawRectangleRec(rec, RLFade(GOLD, 0.6f));
            if (drawRoundedRect) RLDrawRectangleRounded(rec, roundness, (int)segments, RLFade(MAROON, 0.2f));
            if (drawRoundedLines) RLDrawRectangleRoundedLinesEx(rec, roundness, (int)segments, lineThick, RLFade(MAROON, 0.4f));

            // Draw GUI controls
            //------------------------------------------------------------------------------
            GuiSliderBar((RLRectangle){ 640, 40, 105, 20 }, "Width", RLTextFormat("%.2f", width), &width, 0, (float)RLGetScreenWidth() - 300);
            GuiSliderBar((RLRectangle){ 640, 70, 105, 20 }, "Height", RLTextFormat("%.2f", height), &height, 0, (float)RLGetScreenHeight() - 50);
            GuiSliderBar((RLRectangle){ 640, 140, 105, 20 }, "Roundness", RLTextFormat("%.2f", roundness), &roundness, 0.0f, 1.0f);
            GuiSliderBar((RLRectangle){ 640, 170, 105, 20 }, "Thickness", RLTextFormat("%.2f", lineThick), &lineThick, 0, 20);
            GuiSliderBar((RLRectangle){ 640, 240, 105, 20}, "Segments", RLTextFormat("%.2f", segments), &segments, 0, 60);

            GuiCheckBox((RLRectangle){ 640, 320, 20, 20 }, "DrawRoundedRect", &drawRoundedRect);
            GuiCheckBox((RLRectangle){ 640, 350, 20, 20 }, "DrawRoundedLines", &drawRoundedLines);
            GuiCheckBox((RLRectangle){ 640, 380, 20, 20}, "DrawRect", &drawRect);
            //------------------------------------------------------------------------------

            RLDrawText(RLTextFormat("MODE: %s", (segments >= 4)? "MANUAL" : "AUTO"), 640, 280, 10, (segments >= 4)? MAROON : DARKGRAY);

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
