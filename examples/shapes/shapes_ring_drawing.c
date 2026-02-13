/*******************************************************************************************
*
*   raylib [shapes] example - ring drawing
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

    RLInitWindow(screenWidth, screenHeight, "raylib [shapes] example - ring drawing");

    RLVector2 center = {(RLGetScreenWidth() - 300)/2.0f, RLGetScreenHeight()/2.0f };

    float innerRadius = 80.0f;
    float outerRadius = 190.0f;

    float startAngle = 0.0f;
    float endAngle = 360.0f;
    float segments = 0.0f;

    bool drawRing = true;
    bool drawRingLines = false;
    bool drawCircleLines = false;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // NOTE: All variables update happens inside GUI control functions
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawLine(500, 0, 500, RLGetScreenHeight(), RLFade(LIGHTGRAY, 0.6f));
            RLDrawRectangle(500, 0, RLGetScreenWidth() - 500, RLGetScreenHeight(), RLFade(LIGHTGRAY, 0.3f));

            if (drawRing) RLDrawRing(center, innerRadius, outerRadius, startAngle, endAngle, (int)segments, RLFade(MAROON, 0.3f));
            if (drawRingLines) RLDrawRingLines(center, innerRadius, outerRadius, startAngle, endAngle, (int)segments, RLFade(BLACK, 0.4f));
            if (drawCircleLines) RLDrawCircleSectorLines(center, outerRadius, startAngle, endAngle, (int)segments, RLFade(BLACK, 0.4f));

            // Draw GUI controls
            //------------------------------------------------------------------------------
            GuiSliderBar((RLRectangle){ 600, 40, 120, 20 }, "StartAngle", RLTextFormat("%.2f", startAngle), &startAngle, -450, 450);
            GuiSliderBar((RLRectangle){ 600, 70, 120, 20 }, "EndAngle", RLTextFormat("%.2f", endAngle), &endAngle, -450, 450);

            GuiSliderBar((RLRectangle){ 600, 140, 120, 20 }, "InnerRadius", RLTextFormat("%.2f", innerRadius), &innerRadius, 0, 100);
            GuiSliderBar((RLRectangle){ 600, 170, 120, 20 }, "OuterRadius", RLTextFormat("%.2f", outerRadius), &outerRadius, 0, 200);

            GuiSliderBar((RLRectangle){ 600, 240, 120, 20 }, "Segments", RLTextFormat("%.2f", segments), &segments, 0, 100);

            GuiCheckBox((RLRectangle){ 600, 320, 20, 20 }, "Draw Ring", &drawRing);
            GuiCheckBox((RLRectangle){ 600, 350, 20, 20 }, "Draw RingLines", &drawRingLines);
            GuiCheckBox((RLRectangle){ 600, 380, 20, 20 }, "Draw CircleLines", &drawCircleLines);
            //------------------------------------------------------------------------------

            int minSegments = (int)ceilf((endAngle - startAngle)/90);
            RLDrawText(RLTextFormat("MODE: %s", (segments >= minSegments)? "MANUAL" : "AUTO"), 600, 270, 10, (segments >= minSegments)? MAROON : DARKGRAY);

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