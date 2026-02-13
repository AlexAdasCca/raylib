/*******************************************************************************************
*
*   raylib [textures] example - mouse painting
*
*   Example complexity rating: [★★★☆] 3/4
*
*   Example originally created with raylib 3.0, last time updated with raylib 3.0
*
*   Example contributed by Chris Dill (@MysteriousSpace) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2019-2025 Chris Dill (@MysteriousSpace) and Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#define MAX_COLORS_COUNT    23          // Number of colors available

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [textures] example - mouse painting");

    // Colors to choose from
    RLColor colors[MAX_COLORS_COUNT] = {
        RAYWHITE, YELLOW, GOLD, ORANGE, PINK, RED, MAROON, GREEN, LIME, DARKGREEN,
        SKYBLUE, BLUE, DARKBLUE, PURPLE, VIOLET, DARKPURPLE, BEIGE, BROWN, DARKBROWN,
        LIGHTGRAY, GRAY, DARKGRAY, BLACK };

    // Define colorsRecs data (for every rectangle)
    RLRectangle colorsRecs[MAX_COLORS_COUNT] = { 0 };

    for (int i = 0; i < MAX_COLORS_COUNT; i++)
    {
        colorsRecs[i].x = 10 + 30.0f*i + 2*i;
        colorsRecs[i].y = 10;
        colorsRecs[i].width = 30;
        colorsRecs[i].height = 30;
    }

    int colorSelected = 0;
    int colorSelectedPrev = colorSelected;
    int colorMouseHover = 0;
    float brushSize = 20.0f;
    bool mouseWasPressed = false;

    RLRectangle btnSaveRec = { 750, 10, 40, 30 };
    bool btnSaveMouseHover = false;
    bool showSaveMessage = false;
    int saveMessageCounter = 0;

    // Create a RenderTexture2D to use as a canvas
    RLRenderTexture2D target = RLLoadRenderTexture(screenWidth, screenHeight);

    // Clear render texture before entering the game loop
    RLBeginTextureMode(target);
    RLClearBackground(colors[0]);
    RLEndTextureMode();

    RLSetTargetFPS(120);              // Set our game to run at 120 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLVector2 mousePos = RLGetMousePosition();

        // Move between colors with keys
        if (RLIsKeyPressed(KEY_RIGHT)) colorSelected++;
        else if (RLIsKeyPressed(KEY_LEFT)) colorSelected--;

        if (colorSelected >= MAX_COLORS_COUNT) colorSelected = MAX_COLORS_COUNT - 1;
        else if (colorSelected < 0) colorSelected = 0;

        // Choose color with mouse
        for (int i = 0; i < MAX_COLORS_COUNT; i++)
        {
            if (RLCheckCollisionPointRec(mousePos, colorsRecs[i]))
            {
                colorMouseHover = i;
                break;
            }
            else colorMouseHover = -1;
        }

        if ((colorMouseHover >= 0) && RLIsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            colorSelected = colorMouseHover;
            colorSelectedPrev = colorSelected;
        }

        // Change brush size
        brushSize += RLGetMouseWheelMove()*5;
        if (brushSize < 2) brushSize = 2;
        if (brushSize > 50) brushSize = 50;

        if (RLIsKeyPressed(KEY_C))
        {
            // Clear render texture to clear color
            RLBeginTextureMode(target);
            RLClearBackground(colors[0]);
            RLEndTextureMode();
        }

        if (RLIsMouseButtonDown(MOUSE_BUTTON_LEFT) || (RLGetGestureDetected() == GESTURE_DRAG))
        {
            // Paint circle into render texture
            // NOTE: To avoid discontinuous circles, we could store
            // previous-next mouse points and just draw a line using brush size
            RLBeginTextureMode(target);
            if (mousePos.y > 50) RLDrawCircle((int)mousePos.x, (int)mousePos.y, brushSize, colors[colorSelected]);
            RLEndTextureMode();
        }

        if (RLIsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            if (!mouseWasPressed)
            {
                colorSelectedPrev = colorSelected;
                colorSelected = 0;
            }

            mouseWasPressed = true;

            // Erase circle from render texture
            RLBeginTextureMode(target);
            if (mousePos.y > 50) RLDrawCircle((int)mousePos.x, (int)mousePos.y, brushSize, colors[0]);
            RLEndTextureMode();
        }
        else if (RLIsMouseButtonReleased(MOUSE_BUTTON_RIGHT) && mouseWasPressed)
        {
            colorSelected = colorSelectedPrev;
            mouseWasPressed = false;
        }

        // Check mouse hover save button
        if (RLCheckCollisionPointRec(mousePos, btnSaveRec)) btnSaveMouseHover = true;
        else btnSaveMouseHover = false;

        // Image saving logic
        // NOTE: Saving painted texture to a default named image
        if ((btnSaveMouseHover && RLIsMouseButtonReleased(MOUSE_BUTTON_LEFT)) || RLIsKeyPressed(KEY_S))
        {
            RLImage image = RLLoadImageFromTexture(target.texture);
            RLImageFlipVertical(&image);
            RLExportImage(image, "my_amazing_texture_painting.png");
            RLUnloadImage(image);
            showSaveMessage = true;
        }

        if (showSaveMessage)
        {
            // On saving, show a full screen message for 2 seconds
            saveMessageCounter++;
            if (saveMessageCounter > 240)
            {
                showSaveMessage = false;
                saveMessageCounter = 0;
            }
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

        RLClearBackground(RAYWHITE);

        // NOTE: Render texture must be y-flipped due to default OpenGL coordinates (left-bottom)
        RLDrawTextureRec(target.texture, (RLRectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height }, (RLVector2) { 0, 0 }, WHITE);

        // Draw drawing circle for reference
        if (mousePos.y > 50)
        {
            if (RLIsMouseButtonDown(MOUSE_BUTTON_RIGHT)) RLDrawCircleLines((int)mousePos.x, (int)mousePos.y, brushSize, GRAY);
            else RLDrawCircle(RLGetMouseX(), RLGetMouseY(), brushSize, colors[colorSelected]);
        }

        // Draw top panel
        RLDrawRectangle(0, 0, RLGetScreenWidth(), 50, RAYWHITE);
        RLDrawLine(0, 50, RLGetScreenWidth(), 50, LIGHTGRAY);

        // Draw color selection rectangles
        for (int i = 0; i < MAX_COLORS_COUNT; i++) RLDrawRectangleRec(colorsRecs[i], colors[i]);
        RLDrawRectangleLines(10, 10, 30, 30, LIGHTGRAY);

        if (colorMouseHover >= 0) RLDrawRectangleRec(colorsRecs[colorMouseHover], RLFade(WHITE, 0.6f));

        RLDrawRectangleLinesEx((RLRectangle){ colorsRecs[colorSelected].x - 2, colorsRecs[colorSelected].y - 2,
                             colorsRecs[colorSelected].width + 4, colorsRecs[colorSelected].height + 4 }, 2, BLACK);

        // Draw save image button
        RLDrawRectangleLinesEx(btnSaveRec, 2, btnSaveMouseHover ? RED : BLACK);
        RLDrawText("SAVE!", 755, 20, 10, btnSaveMouseHover ? RED : BLACK);

        // Draw save image message
        if (showSaveMessage)
        {
            RLDrawRectangle(0, 0, RLGetScreenWidth(), RLGetScreenHeight(), RLFade(RAYWHITE, 0.8f));
            RLDrawRectangle(0, 150, RLGetScreenWidth(), 80, BLACK);
            RLDrawText("IMAGE SAVED!", 150, 180, 20, RAYWHITE);
        }

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadRenderTexture(target);    // Unload render texture

    RLCloseWindow();                  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
