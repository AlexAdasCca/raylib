/*******************************************************************************************
*
*   raylib [core] example - input gestures
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 1.4, last time updated with raylib 4.2
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2016-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#define MAX_GESTURE_STRINGS   20

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - input gestures");

    RLVector2 touchPosition = { 0, 0 };
    RLRectangle touchArea = { 220, 10, screenWidth - 230.0f, screenHeight - 20.0f };

    int gesturesCount = 0;
    char gestureStrings[MAX_GESTURE_STRINGS][32];

    int currentGesture = RL_E_GESTURE_NONE;
    int lastGesture = RL_E_GESTURE_NONE;

    //SetGesturesEnabled(0b0000000000001001);   // Enable only some gestures to be detected

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        lastGesture = currentGesture;
        currentGesture = RLGetGestureDetected();
        touchPosition = RLGetTouchPosition(0);

        if (RLCheckCollisionPointRec(touchPosition, touchArea) && (currentGesture != RL_E_GESTURE_NONE))
        {
            if (currentGesture != lastGesture)
            {
                // Store gesture string
                switch (currentGesture)
                {
                    case RL_E_GESTURE_TAP: RLTextCopy(gestureStrings[gesturesCount], "GESTURE TAP"); break;
                    case RL_E_GESTURE_DOUBLETAP: RLTextCopy(gestureStrings[gesturesCount], "GESTURE DOUBLETAP"); break;
                    case RL_E_GESTURE_HOLD: RLTextCopy(gestureStrings[gesturesCount], "GESTURE HOLD"); break;
                    case RL_E_GESTURE_DRAG: RLTextCopy(gestureStrings[gesturesCount], "GESTURE DRAG"); break;
                    case RL_E_GESTURE_SWIPE_RIGHT: RLTextCopy(gestureStrings[gesturesCount], "GESTURE SWIPE RIGHT"); break;
                    case RL_E_GESTURE_SWIPE_LEFT: RLTextCopy(gestureStrings[gesturesCount], "GESTURE SWIPE LEFT"); break;
                    case RL_E_GESTURE_SWIPE_UP: RLTextCopy(gestureStrings[gesturesCount], "GESTURE SWIPE UP"); break;
                    case RL_E_GESTURE_SWIPE_DOWN: RLTextCopy(gestureStrings[gesturesCount], "GESTURE SWIPE DOWN"); break;
                    case RL_E_GESTURE_PINCH_IN: RLTextCopy(gestureStrings[gesturesCount], "GESTURE PINCH IN"); break;
                    case RL_E_GESTURE_PINCH_OUT: RLTextCopy(gestureStrings[gesturesCount], "GESTURE PINCH OUT"); break;
                    default: break;
                }

                gesturesCount++;

                // Reset gestures strings
                if (gesturesCount >= MAX_GESTURE_STRINGS)
                {
                    for (int i = 0; i < MAX_GESTURE_STRINGS; i++) RLTextCopy(gestureStrings[i], "\0");

                    gesturesCount = 0;
                }
            }
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawRectangleRec(touchArea, GRAY);
            RLDrawRectangle(225, 15, screenWidth - 240, screenHeight - 30, RAYWHITE);

            RLDrawText("GESTURES TEST AREA", screenWidth - 270, screenHeight - 40, 20, RLFade(GRAY, 0.5f));

            for (int i = 0; i < gesturesCount; i++)
            {
                if (i%2 == 0) RLDrawRectangle(10, 30 + 20*i, 200, 20, RLFade(LIGHTGRAY, 0.5f));
                else RLDrawRectangle(10, 30 + 20*i, 200, 20, RLFade(LIGHTGRAY, 0.3f));

                if (i < gesturesCount - 1) RLDrawText(gestureStrings[i], 35, 36 + 20*i, 10, DARKGRAY);
                else RLDrawText(gestureStrings[i], 35, 36 + 20*i, 10, MAROON);
            }

            RLDrawRectangleLines(10, 29, 200, screenHeight - 50, GRAY);
            RLDrawText("DETECTED GESTURES", 50, 15, 10, GRAY);

            if (currentGesture != RL_E_GESTURE_NONE) RLDrawCircleV(touchPosition, 30, MAROON);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}