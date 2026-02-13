/*******************************************************************************************
*
*   raylib [shapes] example - lines drawing
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 5.6-dev, last time updated with raylib 5.6
*
*   Example contributed by Robin (@RobinsAviary) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Robin (@RobinsAviary)
*
********************************************************************************************/

#include "raylib.h"

#include "raymath.h"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [shapes] example - lines drawing");

    // Hint text that shows before you click the screen
    bool startText = true;

    // The mouse's position on the previous frame
    RLVector2 mousePositionPrevious = RLGetMousePosition();

    // The canvas to draw lines on
    RLRenderTexture canvas = RLLoadRenderTexture(screenWidth, screenHeight);

    // The line's thickness
    float lineThickness = 8.0f;
    // The lines hue (in HSV, from 0-360)
    float lineHue = 0.0f;

    // Clear the canvas to the background color
    RLBeginTextureMode(canvas);
        RLClearBackground(RAYWHITE);
    RLEndTextureMode();

    RLSetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // Disable the hint text once the user clicks
        if (RLIsMouseButtonPressed(MOUSE_BUTTON_LEFT) && startText) startText = false;

        // Clear the canvas when the user middle-clicks
        if (RLIsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
        {
            RLBeginTextureMode(canvas);
                RLClearBackground(RAYWHITE);
            RLEndTextureMode();
        }

        // Store whether the left and right buttons are down
        bool leftButtonDown = RLIsMouseButtonDown(MOUSE_BUTTON_LEFT);
        bool rightButtonDown = RLIsMouseButtonDown(MOUSE_BUTTON_RIGHT);

        if (leftButtonDown || rightButtonDown)
        {
            // The color for the line
            RLColor drawColor = WHITE;

            if (leftButtonDown)
            {
                // Increase the hue value by the distance our cursor has moved since the last frame (divided by 3)
                lineHue += Vector2Distance(mousePositionPrevious, RLGetMousePosition())/3.0f;

                // While the hue is >=360, subtract it to bring it down into the range 0-360
                // This is more visually accurate than resetting to zero
                while (lineHue >= 360.0f) lineHue -= 360.0f;

                // Create the final color
                drawColor = RLColorFromHSV(lineHue, 1.0f, 1.0f);
            }
            else if (rightButtonDown) drawColor = RAYWHITE; // Use the background color as an "eraser"

            // Draw the line onto the canvas
            RLBeginTextureMode(canvas);
                // Circles act as "caps", smoothing corners
                RLDrawCircleV(mousePositionPrevious, lineThickness/2.0f, drawColor);
                RLDrawCircleV(RLGetMousePosition(), lineThickness/2.0f, drawColor);
                RLDrawLineEx(mousePositionPrevious, RLGetMousePosition(), lineThickness, drawColor);
            RLEndTextureMode();
        }

        // Update line thickness based on mousewheel
        lineThickness += RLGetMouseWheelMove();
        lineThickness = Clamp(lineThickness, 1.0, 500.0f);

        // Update mouse's previous position
        mousePositionPrevious = RLGetMousePosition();
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            // Draw the render texture to the screen, flipped vertically to make it appear top-side up
            RLDrawTextureRec(canvas.texture, (RLRectangle){ 0.0f, 0.0f, (float)canvas.texture.width,(float)-canvas.texture.height }, Vector2Zero(), WHITE);

            // Draw the preview circle
            if (!leftButtonDown) RLDrawCircleLinesV(RLGetMousePosition(), lineThickness/2.0f, (RLColor){ 127, 127, 127, 127 });

            // Draw the hint text
            if (startText) RLDrawText("try clicking and dragging!", 275, 215, 20, LIGHTGRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadRenderTexture(canvas); // Unload the canvas render texture

    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}