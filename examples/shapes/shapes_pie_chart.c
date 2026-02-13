/*******************************************************************************************
*
*   raylib [shapes] example - pie chart
*
*   Example complexity rating: [★★★☆] 3/4
*
*   Example originally created with raylib 5.5, last time updated with raylib 5.6
*
*   Example contributed by Gideon Serfontein (@GideonSerf) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Gideon Serfontein (@GideonSerf)
*
********************************************************************************************/

#include "raylib.h"
#include <math.h>
#include <stdio.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define MAX_PIE_SLICES  10       // Max pie slices

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [shapes] example - pie chart");

    int sliceCount = 7;
    float donutInnerRadius = 25.0f;
    float values[MAX_PIE_SLICES] = { 300.0f, 100.0f, 450.0f, 350.0f, 600.0f, 380.0f, 750.0f }; // Initial slice values
    char labels[MAX_PIE_SLICES][32] = { 0 };
    bool editingLabel[MAX_PIE_SLICES] = { 0 };

    for (int i = 0; i < MAX_PIE_SLICES; i++)
        snprintf(labels[i], 32, "Slice %02i", i + 1);

    bool showValues = true;
    bool showPercentages = false;
    bool showDonut = false;
    int hoveredSlice = -1;
    RLRectangle scrollPanelBounds = { 0 };
    RLVector2 scrollContentOffset = { 0 };
    RLRectangle view = { 0 };

    // UI layout parameters
    const int panelWidth = 270;
    const int panelMargin = 5;

    // UI Panel top-left anchor
    const RLVector2 panelPos = {
        (float)screenWidth  - panelMargin - panelWidth,
        (float)panelMargin
    };

    // UI Panel rectangle
    const RLRectangle panelRect = {
        panelPos.x, panelPos.y,
        (float)panelWidth,
        (float)screenHeight - 2.0f*panelMargin
    };

    // Pie chart geometry
    const RLRectangle canvas = { 0, 0, panelPos.x, (float)screenHeight };
    const RLVector2 center = { canvas.width/2.0f, canvas.height/2.0f};
    const float radius = 205.0f;

    // Total value for percentage calculations
    float totalValue = 0.0f;

    RLSetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())
    {
        // Update
        //----------------------------------------------------------------------------------
        // Calculate total value for percentage calculations
        totalValue = 0.0f;
        for (int i = 0; i < sliceCount; i++) totalValue += values[i];

        // Check for mouse hover over slices
        hoveredSlice = -1; // Reset hovered slice
        RLVector2 mousePos = RLGetMousePosition();
        if (RLCheckCollisionPointRec(mousePos, canvas)) // Only check if mouse is inside the canvas
        {
            float dx = mousePos.x - center.x;
            float dy = mousePos.y - center.y;
            float distance = sqrtf(dx*dx + dy*dy);

            if (distance <= radius) // Inside the pie radius
            {
                float angle = atan2f(dy, dx)*RAD2DEG;
                if (angle < 0) angle += 360;

                float currentAngle = 0.0f;
                for (int i = 0; i < sliceCount; i++)
                {
                    float sweep = (totalValue > 0)? (values[i]/totalValue)*360.0f : 0.0f;

                    if ((angle >= currentAngle) && (angle < (currentAngle + sweep)))
                    {
                        hoveredSlice = i;
                        break;
                    }

                    currentAngle += sweep;
                }
            }
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();
            RLClearBackground(RAYWHITE);

            // Draw the pie chart on the canvas
            float startAngle = 0.0f;
            for (int i = 0; i < sliceCount; i++)
            {
                float sweepAngle = (totalValue > 0)? (values[i]/totalValue)*360.0f : 0.0f;
                float midAngle = startAngle + sweepAngle/2.0f; // Middle angle for label positioning

                RLColor color = RLColorFromHSV((float)i/sliceCount*360.0f, 0.75f, 0.9f);
                float currentRadius = radius;

                // Make the hovered slice pop out by adding 5 pixels to its radius
                if (i == hoveredSlice) currentRadius += 20.0f;

                // Draw the pie slice using raylib's DrawCircleSector function
                RLDrawCircleSector(center, currentRadius, startAngle, startAngle + sweepAngle, 120, color);

                // Draw the label for the current slice
                if (values[i] > 0)
                {
                    char labelText[64] = { 0 };
                    if (showValues && showPercentages) snprintf(labelText, 64, "%.1f (%.0f%%)", values[i], (values[i]/totalValue)*100.0f);
                    else if (showValues) snprintf(labelText, 64, "%.1f", values[i]);
                    else if (showPercentages) snprintf(labelText, 64, "%.0f%%", (values[i]/totalValue)*100.0f);
                    else labelText[0] = '\0';

                    RLVector2 textSize = RLMeasureTextEx(RLGetFontDefault(), labelText, 20, 1);
                    float labelRadius = radius*0.7f;
                    RLVector2 labelPos = { center.x + cosf(midAngle*DEG2RAD)*labelRadius - textSize.x/2.0f,
                        center.y + sinf(midAngle*DEG2RAD)*labelRadius - textSize.y/2.0f };
                    RLDrawText(labelText, (int)labelPos.x, (int)labelPos.y, 20, WHITE);
                }

                // Draw inner circle to create donut effect
                // TODO: This is a hacky solution, better use DrawRing()
                if (showDonut) RLDrawCircle(center.x, center.y, donutInnerRadius, RAYWHITE);

                startAngle += sweepAngle;
            }

            // UI control panel
            RLDrawRectangleRec(panelRect, RLFade(LIGHTGRAY, 0.5f));
            RLDrawRectangleLinesEx(panelRect, 1.0f, GRAY);

            GuiSpinner((RLRectangle){ panelPos.x + 95, (float)panelPos.y + 12, 125, 25 }, "Slices ", &sliceCount, 1, MAX_PIE_SLICES, false);
            GuiCheckBox((RLRectangle){ panelPos.x + 20, (float)panelPos.y + 12 + 40, 20, 20 }, "Show Values", &showValues);
            GuiCheckBox((RLRectangle){ panelPos.x + 20, (float)panelPos.y + 12 + 70, 20, 20 }, "Show Percentages", &showPercentages);
            GuiCheckBox((RLRectangle){ panelPos.x + 20, (float)panelPos.y + 12 + 100, 20, 20 }, "Make Donut", &showDonut);

            if (showDonut) GuiDisable();
            GuiSliderBar((RLRectangle){ panelPos.x + 80, (float)panelPos.y + 12 + 130, panelRect.width - 100, 30 },
                            "Inner Radius", NULL, &donutInnerRadius, 5.0f, radius - 10.0f);
            GuiEnable();

            GuiLine((RLRectangle){ panelPos.x + 10, (float)panelPos.y + 12 + 170, panelRect.width - 20, 1 }, NULL);

            // Scrollable area for slice editors
            scrollPanelBounds = (RLRectangle){
                panelPos.x + panelMargin,
                (float)panelPos.y + 12 + 190,
                panelRect.width - panelMargin*2,
                panelRect.y + panelRect.height - panelPos.y + 12 + 190 - panelMargin
            };
            int contentHeight = sliceCount*35;

            GuiScrollPanel(scrollPanelBounds, NULL,
                (RLRectangle){ 0, 0, panelRect.width - 25, (float)contentHeight },
                &scrollContentOffset, &view);

            const float contentX = view.x + scrollContentOffset.x; // Left of content
            const float contentY = view.y + scrollContentOffset.y; // Top of content

            RLBeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);

                for (int i = 0; i < sliceCount; i++)
                {
                    const int rowY = (int)(contentY + 5 + i*35);

                    // Color indicator
                    RLColor color = RLColorFromHSV((float)i/sliceCount*360.0f, 0.75f, 0.9f);
                    RLDrawRectangle((int)(contentX + 15), rowY + 5, 20, 20, color);

                    // Label textbox
                    if (GuiTextBox((RLRectangle){ contentX + 45, (float)rowY, 75, 30 }, labels[i], 32, editingLabel[i])) editingLabel[i] = !editingLabel[i];

                    GuiSliderBar((RLRectangle){ contentX + 130, (float)rowY, 110, 30 }, NULL, NULL, &values[i], 0.0f, 1000.0f);
                }

            RLEndScissorMode();

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}