/*******************************************************************************************
*
*   raylib [core] example - input gestures testbed
*
*   Example complexity rating: [★★★☆] 3/4
*
*   Example originally created with raylib 5.0, last time updated with raylib 5.6-dev
*
*   Example contributed by ubkp (@ubkp) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2023-2025 ubkp (@ubkp)
*
********************************************************************************************/

#include "raylib.h"

#include <math.h>       // Required for the protractor angle graphic drawing

#define GESTURE_LOG_SIZE    20
#define MAX_TOUCH_COUNT     32

//------------------------------------------------------------------------------------
// Module Functions Declaration
//------------------------------------------------------------------------------------
static char const *GetGestureName(int gesture); // Get text string for gesture value
static RLColor GetGestureColor(int gesture); // Get color for gesture value

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - input gestures testbed");

    RLVector2 messagePosition = { 160, 7 };

    // Last gesture variables definitions
    int lastGesture = 0;
    RLVector2 lastGesturePosition = { 165, 130 };

    // Gesture log variables definitions
    // NOTE: The gesture log uses an array (as an inverted circular queue) to store the performed gestures
    char gestureLog[GESTURE_LOG_SIZE][12] = { "" };
    // NOTE: The index for the inverted circular queue (moving from last to first direction, then looping around)
    int gestureLogIndex = GESTURE_LOG_SIZE;
    int previousGesture = 0;

    // Log mode values:
    // - 0 shows repeated events
    // - 1 hides repeated events
    // - 2 shows repeated events but hide hold events
    // - 3 hides repeated events and hide hold events
    int logMode = 1;

    RLColor gestureColor = { 0, 0, 0, 255 };
    RLRectangle logButton1 = { 53, 7, 48, 26 };
    RLRectangle logButton2 = { 108, 7, 36, 26 };
    RLVector2 gestureLogPosition = { 10, 10 };

    // Protractor variables definitions
    float angleLength = 90.0f;
    float currentAngleDegrees = 0.0f;
    RLVector2 finalVector = { 0.0f, 0.0f };
    RLVector2 protractorPosition = { 266.0f, 315.0f };

    RLSetTargetFPS(60);   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //--------------------------------------------------------------------------------------
        // Handle common gestures data
        int i, ii; // Iterators that will be reused by all for loops
        const int currentGesture = RLGetGestureDetected();
        const float currentDragDegrees = RLGetGestureDragAngle();
        const float currentPitchDegrees = RLGetGesturePinchAngle();
        const int touchCount = RLGetTouchPointCount();

        // Handle last gesture
        if ((currentGesture != 0) && (currentGesture != 4) && (currentGesture != previousGesture))
            lastGesture = currentGesture; // Filter the meaningful gestures (1, 2, 8 to 512) for the display

        // Handle gesture log
        if (RLIsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            if (RLCheckCollisionPointRec(RLGetMousePosition(), logButton1))
            {
                switch (logMode)
                {
                    case 3: logMode = 2; break;
                    case 2: logMode = 3; break;
                    case 1: logMode = 0; break;
                    default: logMode = 1; break;
                }
            }
            else if (RLCheckCollisionPointRec(RLGetMousePosition(), logButton2))
            {
                switch (logMode)
                {
                    case 3: logMode = 1; break;
                    case 2: logMode = 0; break;
                    case 1: logMode = 3; break;
                    default: logMode = 2; break;
                }
            }
        }

        int fillLog = 0; // Gate variable to be used to allow or not the gesture log to be filled
        if (currentGesture !=0)
        {
            if (logMode == 3) // 3 hides repeated events and hide hold events
            {
                if (((currentGesture != 4) && (currentGesture != previousGesture)) || (currentGesture < 3)) fillLog = 1;
            }
            else if (logMode == 2) // 2 shows repeated events but hide hold events
            {
                if (currentGesture != 4) fillLog = 1;
            }
            else if (logMode == 1) // 1 hides repeated events
            {
                if (currentGesture != previousGesture) fillLog = 1;
            }
            else  // 0 shows repeated events
            {
                fillLog = 1;
            }
        }

        if (fillLog) // If one of the conditions from logMode was met, fill the gesture log
        {
            previousGesture = currentGesture;
            gestureColor = GetGestureColor(currentGesture);
            if (gestureLogIndex <= 0) gestureLogIndex = GESTURE_LOG_SIZE;
            gestureLogIndex--;

            // Copy the gesture respective name to the gesture log array
            RLTextCopy(gestureLog[gestureLogIndex], GetGestureName(currentGesture));
        }

        // Handle protractor
        if (currentGesture > 255) currentAngleDegrees = currentPitchDegrees; // Pinch In and Pinch Out
        else if (currentGesture > 15) currentAngleDegrees = currentDragDegrees; // Swipe Right, Swipe Left, Swipe Up and Swipe Down
        else if (currentGesture > 0) currentAngleDegrees = 0.0f; // Tap, Doubletap, Hold and Grab

        float currentAngleRadians = ((currentAngleDegrees + 90.0f)*PI/180); // Convert the current angle to Radians
        // Calculate the final vector for display
        finalVector = (RLVector2){ (angleLength*sinf(currentAngleRadians)) + protractorPosition.x,
            (angleLength*cosf(currentAngleRadians)) + protractorPosition.y };

        // Handle touch and mouse pointer points
        RLVector2 touchPosition[MAX_TOUCH_COUNT] = { 0 };
        RLVector2 mousePosition = { 0 };
        if (currentGesture != GESTURE_NONE)
        {
            if (touchCount != 0)
            {
                for (i = 0; i < touchCount; i++) touchPosition[i] = RLGetTouchPosition(i); // Fill the touch positions
            }
            else mousePosition = RLGetMousePosition();
        }
        //--------------------------------------------------------------------------------------

        // Draw
        //--------------------------------------------------------------------------------------
        RLBeginDrawing();
            RLClearBackground(RAYWHITE);

            // Draw common elements
            RLDrawText("*", (int)messagePosition.x + 5, (int)messagePosition.y + 5, 10, BLACK);
            RLDrawText("Example optimized for Web/HTML5\non Smartphones with Touch Screen.", (int)messagePosition.x + 15, (int)messagePosition.y + 5, 10, BLACK);
            RLDrawText("*", (int)messagePosition.x + 5, (int)messagePosition.y + 35, 10, BLACK);
            RLDrawText("While running on Desktop Web Browsers,\ninspect and turn on Touch Emulation.", (int)messagePosition.x + 15, (int)messagePosition.y + 35, 10, BLACK);

            // Draw last gesture
            RLDrawText("Last gesture", (int)lastGesturePosition.x + 33, (int)lastGesturePosition.y - 47, 20, BLACK);
            RLDrawText("Swipe         Tap       Pinch  Touch", (int)lastGesturePosition.x + 17, (int)lastGesturePosition.y - 18, 10, BLACK);
            RLDrawRectangle((int)lastGesturePosition.x + 20, (int)lastGesturePosition.y, 20, 20, lastGesture == GESTURE_SWIPE_UP ? RED : LIGHTGRAY);
            RLDrawRectangle((int)lastGesturePosition.x, (int)lastGesturePosition.y + 20, 20, 20, lastGesture == GESTURE_SWIPE_LEFT ? RED : LIGHTGRAY);
            RLDrawRectangle((int)lastGesturePosition.x + 40, (int)lastGesturePosition.y + 20, 20, 20, lastGesture == GESTURE_SWIPE_RIGHT ? RED : LIGHTGRAY);
            RLDrawRectangle((int)lastGesturePosition.x + 20, (int)lastGesturePosition.y + 40, 20, 20, lastGesture == GESTURE_SWIPE_DOWN ? RED : LIGHTGRAY);
            RLDrawCircle((int)lastGesturePosition.x + 80, (int)lastGesturePosition.y + 16, 10, lastGesture == GESTURE_TAP ? BLUE : LIGHTGRAY);
            RLDrawRing( (RLVector2){lastGesturePosition.x + 103, lastGesturePosition.y + 16}, 6.0f, 11.0f, 0.0f, 360.0f, 0, lastGesture == GESTURE_DRAG ? LIME : LIGHTGRAY);
            RLDrawCircle((int)lastGesturePosition.x + 80, (int)lastGesturePosition.y + 43, 10, lastGesture == GESTURE_DOUBLETAP ? SKYBLUE : LIGHTGRAY);
            RLDrawCircle((int)lastGesturePosition.x + 103, (int)lastGesturePosition.y + 43, 10, lastGesture == GESTURE_DOUBLETAP ? SKYBLUE : LIGHTGRAY);
            RLDrawTriangle((RLVector2){ lastGesturePosition.x + 122, lastGesturePosition.y + 16 }, (RLVector2){ lastGesturePosition.x + 137, lastGesturePosition.y + 26 }, (RLVector2){lastGesturePosition.x + 137, lastGesturePosition.y + 6 }, lastGesture == GESTURE_PINCH_OUT? ORANGE : LIGHTGRAY);
            RLDrawTriangle((RLVector2){ lastGesturePosition.x + 147, lastGesturePosition.y + 6 }, (RLVector2){ lastGesturePosition.x + 147, lastGesturePosition.y + 26 }, (RLVector2){ lastGesturePosition.x + 162, lastGesturePosition.y + 16 }, lastGesture == GESTURE_PINCH_OUT? ORANGE : LIGHTGRAY);
            RLDrawTriangle((RLVector2){ lastGesturePosition.x + 125, lastGesturePosition.y + 33 }, (RLVector2){ lastGesturePosition.x + 125, lastGesturePosition.y + 53 }, (RLVector2){ lastGesturePosition.x + 140, lastGesturePosition.y + 43 }, lastGesture == GESTURE_PINCH_IN? VIOLET : LIGHTGRAY);
            RLDrawTriangle((RLVector2){ lastGesturePosition.x + 144, lastGesturePosition.y + 43 }, (RLVector2){ lastGesturePosition.x + 159, lastGesturePosition.y + 53 }, (RLVector2){ lastGesturePosition.x + 159, lastGesturePosition.y + 33 }, lastGesture == GESTURE_PINCH_IN? VIOLET : LIGHTGRAY);
            for (i = 0; i < 4; i++) RLDrawCircle((int)lastGesturePosition.x + 180, (int)lastGesturePosition.y + 7 + i*15, 5, touchCount <= i? LIGHTGRAY : gestureColor);

            // Draw gesture log
            RLDrawText("Log", (int)gestureLogPosition.x, (int)gestureLogPosition.y, 20, BLACK);

            // Loop in both directions to print the gesture log array in the inverted order (and looping around if the index started somewhere in the middle)
            for (i = 0, ii = gestureLogIndex; i < GESTURE_LOG_SIZE; i++, ii = (ii + 1)%GESTURE_LOG_SIZE) RLDrawText(gestureLog[ii], (int)gestureLogPosition.x, (int)gestureLogPosition.y + 410 - i*20, 20, (i == 0 ? gestureColor : LIGHTGRAY));
            RLColor logButton1Color, logButton2Color;
            switch (logMode)
            {
                case 3:  logButton1Color=MAROON; logButton2Color=MAROON; break;
                case 2:  logButton1Color=GRAY;   logButton2Color=MAROON; break;
                case 1:  logButton1Color=MAROON; logButton2Color=GRAY;   break;
                default: logButton1Color=GRAY;   logButton2Color=GRAY;   break;
            }
            RLDrawRectangleRec(logButton1, logButton1Color);
            RLDrawText("Hide", (int)logButton1.x + 7, (int)logButton1.y + 3, 10, WHITE);
            RLDrawText("Repeat", (int)logButton1.x + 7, (int)logButton1.y + 13, 10, WHITE);
            RLDrawRectangleRec(logButton2, logButton2Color);
            RLDrawText("Hide", (int)logButton1.x + 62, (int)logButton1.y + 3, 10, WHITE);
            RLDrawText("Hold", (int)logButton1.x + 62, (int)logButton1.y + 13, 10, WHITE);

            // Draw protractor
            RLDrawText("Angle", (int)protractorPosition.x + 55, (int)protractorPosition.y + 76, 10, BLACK);
            const char *angleString = RLTextFormat("%f", currentAngleDegrees);
            const int angleStringDot = RLTextFindIndex(angleString, ".");
            const char *angleStringTrim = RLTextSubtext(angleString, 0, angleStringDot + 3);
            RLDrawText( angleStringTrim, (int)protractorPosition.x + 55, (int)protractorPosition.y + 92, 20, gestureColor);
            RLDrawCircleV(protractorPosition, 80.0f, WHITE);
            RLDrawLineEx((RLVector2){ protractorPosition.x - 90, protractorPosition.y }, (RLVector2){ protractorPosition.x + 90, protractorPosition.y }, 3.0f, LIGHTGRAY);
            RLDrawLineEx((RLVector2){ protractorPosition.x, protractorPosition.y - 90 }, (RLVector2){ protractorPosition.x, protractorPosition.y + 90 }, 3.0f, LIGHTGRAY);
            RLDrawLineEx((RLVector2){ protractorPosition.x - 80, protractorPosition.y - 45 }, (RLVector2){ protractorPosition.x + 80, protractorPosition.y + 45 }, 3.0f, GREEN);
            RLDrawLineEx((RLVector2){ protractorPosition.x - 80, protractorPosition.y + 45 }, (RLVector2){ protractorPosition.x + 80, protractorPosition.y - 45 }, 3.0f, GREEN);
            RLDrawText("0", (int)protractorPosition.x + 96, (int)protractorPosition.y - 9, 20, BLACK);
            RLDrawText("30", (int)protractorPosition.x + 74, (int)protractorPosition.y - 68, 20, BLACK);
            RLDrawText("90", (int)protractorPosition.x - 11, (int)protractorPosition.y - 110, 20, BLACK);
            RLDrawText("150", (int)protractorPosition.x - 100, (int)protractorPosition.y - 68, 20, BLACK);
            RLDrawText("180", (int)protractorPosition.x - 124, (int)protractorPosition.y - 9, 20, BLACK);
            RLDrawText("210", (int)protractorPosition.x - 100, (int)protractorPosition.y + 50, 20, BLACK);
            RLDrawText("270", (int)protractorPosition.x - 18, (int)protractorPosition.y + 92, 20, BLACK);
            RLDrawText("330", (int)protractorPosition.x + 72, (int)protractorPosition.y + 50, 20, BLACK);
            if (currentAngleDegrees != 0.0f) RLDrawLineEx(protractorPosition, finalVector, 3.0f, gestureColor);

            // Draw touch and mouse pointer points
            if (currentGesture != GESTURE_NONE)
            {
                if ( touchCount != 0 )
                {
                    for (i = 0; i < touchCount; i++)
                    {
                        RLDrawCircleV(touchPosition[i], 50.0f, RLFade(gestureColor, 0.5f));
                        RLDrawCircleV(touchPosition[i], 5.0f, gestureColor);
                    }

                    if (touchCount == 2) RLDrawLineEx(touchPosition[0], touchPosition[1], ((currentGesture == 512)? 8.0f : 12.0f), gestureColor);
                }
                else
                {
                    RLDrawCircleV(mousePosition, 35.0f, RLFade(gestureColor, 0.5f));
                    RLDrawCircleV(mousePosition, 5.0f, gestureColor);
                }
            }

        RLEndDrawing();
        //--------------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
// Get text string for gesture value
static char const *GetGestureName(int gesture)
{
    switch (gesture)
    {
        case 0: return "None"; break;
        case 1: return "Tap"; break;
        case 2: return "Double Tap"; break;
        case 4: return "Hold"; break;
        case 8: return "Drag"; break;
        case 16: return "Swipe Right"; break;
        case 32: return "Swipe Left"; break;
        case 64: return "Swipe Up"; break;
        case 128: return "Swipe Down"; break;
        case 256: return "Pinch In"; break;
        case 512: return "Pinch Out"; break;
        default:  return "Unknown"; break;
    }
}

// Get color for gesture value
static RLColor GetGestureColor(int gesture)
{
    switch (gesture)
    {
        case 0: return BLACK; break;
        case 1: return BLUE; break;
        case 2: return SKYBLUE; break;
        case 4: return BLACK; break;
        case 8: return LIME; break;
        case 16: return RED; break;
        case 32: return RED; break;
        case 64: return RED; break;
        case 128: return RED; break;
        case 256: return VIOLET; break;
        case 512: return ORANGE; break;
        default: return BLACK; break;
    }
}
