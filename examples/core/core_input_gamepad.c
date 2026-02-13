/*******************************************************************************************
*
*   raylib [core] example - input gamepad
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   NOTE: This example requires a Gamepad connected to the system
*         raylib is configured to work with the following gamepads:
*                - Xbox 360 Controller (Xbox 360, Xbox One)
*                - PLAYSTATION(R)3 Controller
*         Check raylib.h for buttons configuration
*
*   Example originally created with raylib 1.1, last time updated with raylib 4.2
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2013-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

// NOTE: Gamepad name ID depends on drivers and OS
#define XBOX_ALIAS_1 "xbox"
#define XBOX_ALIAS_2 "x-box"
#define PS_ALIAS     "playstation"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLSetConfigFlags(FLAG_MSAA_4X_HINT);  // Set MSAA 4X hint before windows creation

    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - input gamepad");

    RLTexture2D texPs3Pad = RLLoadTexture("resources/ps3.png");
    RLTexture2D texXboxPad = RLLoadTexture("resources/xbox.png");

    // Set axis deadzones
    const float leftStickDeadzoneX = 0.1f;
    const float leftStickDeadzoneY = 0.1f;
    const float rightStickDeadzoneX = 0.1f;
    const float rightStickDeadzoneY = 0.1f;
    const float leftTriggerDeadzone = -0.9f;
    const float rightTriggerDeadzone = -0.9f;

    RLRectangle vibrateButton = { 0 };

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    int gamepad = 0; // which gamepad to display

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (RLIsKeyPressed(KEY_LEFT) && gamepad > 0) gamepad--;
        if (RLIsKeyPressed(KEY_RIGHT)) gamepad++;
        RLVector2 mousePosition = RLGetMousePosition();

        vibrateButton = (RLRectangle){ 10, 70.0f + 20*RLGetGamepadAxisCount(gamepad) + 20, 75, 24 };
        if (RLIsMouseButtonPressed(MOUSE_BUTTON_LEFT) && RLCheckCollisionPointRec(mousePosition, vibrateButton)) RLSetGamepadVibration(gamepad, 1.0, 1.0, 1.0);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            if (RLIsGamepadAvailable(gamepad))
            {
                RLDrawText(RLTextFormat("GP%d: %s", gamepad, RLGetGamepadName(gamepad)), 10, 10, 10, BLACK);

                // Get axis values
                float leftStickX = RLGetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X);
                float leftStickY = RLGetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y);
                float rightStickX = RLGetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_X);
                float rightStickY = RLGetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_Y);
                float leftTrigger = RLGetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_TRIGGER);
                float rightTrigger = RLGetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_TRIGGER);

                // Calculate deadzones
                if (leftStickX > -leftStickDeadzoneX && leftStickX < leftStickDeadzoneX) leftStickX = 0.0f;
                if (leftStickY > -leftStickDeadzoneY && leftStickY < leftStickDeadzoneY) leftStickY = 0.0f;
                if (rightStickX > -rightStickDeadzoneX && rightStickX < rightStickDeadzoneX) rightStickX = 0.0f;
                if (rightStickY > -rightStickDeadzoneY && rightStickY < rightStickDeadzoneY) rightStickY = 0.0f;
                if (leftTrigger < leftTriggerDeadzone) leftTrigger = -1.0f;
                if (rightTrigger < rightTriggerDeadzone) rightTrigger = -1.0f;

                if ((RLTextFindIndex(RLTextToLower(RLGetGamepadName(gamepad)), XBOX_ALIAS_1) > -1) ||
                    (RLTextFindIndex(RLTextToLower(RLGetGamepadName(gamepad)), XBOX_ALIAS_2) > -1))
                {
                    RLDrawTexture(texXboxPad, 0, 0, DARKGRAY);

                    // Draw buttons: xbox home
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE)) RLDrawCircle(394, 89, 19, RED);

                    // Draw buttons: basic
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT)) RLDrawCircle(436, 150, 9, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE_LEFT)) RLDrawCircle(352, 150, 9, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) RLDrawCircle(501, 151, 15, BLUE);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) RLDrawCircle(536, 187, 15, LIME);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) RLDrawCircle(572, 151, 15, MAROON);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP)) RLDrawCircle(536, 115, 15, GOLD);

                    // Draw buttons: d-pad
                    RLDrawRectangle(317, 202, 19, 71, BLACK);
                    RLDrawRectangle(293, 228, 69, 19, BLACK);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_UP)) RLDrawRectangle(317, 202, 19, 26, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) RLDrawRectangle(317, 202 + 45, 19, 26, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) RLDrawRectangle(292, 228, 25, 19, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) RLDrawRectangle(292 + 44, 228, 26, 19, RED);

                    // Draw buttons: left-right back
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) RLDrawCircle(259, 61, 20, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) RLDrawCircle(536, 61, 20, RED);

                    // Draw axis: left joystick
                    RLColor leftGamepadColor = BLACK;
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_THUMB)) leftGamepadColor = RED;
                    RLDrawCircle(259, 152, 39, BLACK);
                    RLDrawCircle(259, 152, 34, LIGHTGRAY);
                    RLDrawCircle(259 + (int)(leftStickX*20), 152 + (int)(leftStickY*20), 25, leftGamepadColor);

                    // Draw axis: right joystick
                    RLColor rightGamepadColor = BLACK;
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_THUMB)) rightGamepadColor = RED;
                    RLDrawCircle(461, 237, 38, BLACK);
                    RLDrawCircle(461, 237, 33, LIGHTGRAY);
                    RLDrawCircle(461 + (int)(rightStickX*20), 237 + (int)(rightStickY*20), 25, rightGamepadColor);

                    // Draw axis: left-right triggers
                    RLDrawRectangle(170, 30, 15, 70, GRAY);
                    RLDrawRectangle(604, 30, 15, 70, GRAY);
                    RLDrawRectangle(170, 30, 15, (int)(((1 + leftTrigger)/2)*70), RED);
                    RLDrawRectangle(604, 30, 15, (int)(((1 + rightTrigger)/2)*70), RED);

                    //DrawText(TextFormat("Xbox axis LT: %02.02f", GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_TRIGGER)), 10, 40, 10, BLACK);
                    //DrawText(TextFormat("Xbox axis RT: %02.02f", GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_TRIGGER)), 10, 60, 10, BLACK);
                }
                else if (RLTextFindIndex(RLTextToLower(RLGetGamepadName(gamepad)), PS_ALIAS) > -1)
                {
                    RLDrawTexture(texPs3Pad, 0, 0, DARKGRAY);

                    // Draw buttons: ps
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE)) RLDrawCircle(396, 222, 13, RED);

                    // Draw buttons: basic
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE_LEFT)) RLDrawRectangle(328, 170, 32, 13, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT)) RLDrawTriangle((RLVector2){ 436, 168 }, (RLVector2){ 436, 185 }, (RLVector2){ 464, 177 }, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP)) RLDrawCircle(557, 144, 13, LIME);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) RLDrawCircle(586, 173, 13, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) RLDrawCircle(557, 203, 13, VIOLET);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) RLDrawCircle(527, 173, 13, PINK);

                    // Draw buttons: d-pad
                    RLDrawRectangle(225, 132, 24, 84, BLACK);
                    RLDrawRectangle(195, 161, 84, 25, BLACK);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_UP)) RLDrawRectangle(225, 132, 24, 29, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) RLDrawRectangle(225, 132 + 54, 24, 30, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) RLDrawRectangle(195, 161, 30, 25, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) RLDrawRectangle(195 + 54, 161, 30, 25, RED);

                    // Draw buttons: left-right back buttons
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) RLDrawCircle(239, 82, 20, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) RLDrawCircle(557, 82, 20, RED);

                    // Draw axis: left joystick
                    RLColor leftGamepadColor = BLACK;
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_THUMB)) leftGamepadColor = RED;
                    RLDrawCircle(319, 255, 35, BLACK);
                    RLDrawCircle(319, 255, 31, LIGHTGRAY);
                    RLDrawCircle(319 + (int)(leftStickX*20), 255 + (int)(leftStickY*20), 25, leftGamepadColor);

                    // Draw axis: right joystick
                    RLColor rightGamepadColor = BLACK;
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_THUMB)) rightGamepadColor = RED;
                    RLDrawCircle(475, 255, 35, BLACK);
                    RLDrawCircle(475, 255, 31, LIGHTGRAY);
                    RLDrawCircle(475 + (int)(rightStickX*20), 255 + (int)(rightStickY*20), 25, rightGamepadColor);

                    // Draw axis: left-right triggers
                    RLDrawRectangle(169, 48, 15, 70, GRAY);
                    RLDrawRectangle(611, 48, 15, 70, GRAY);
                    RLDrawRectangle(169, 48, 15, (int)(((1 + leftTrigger)/2)*70), RED);
                    RLDrawRectangle(611, 48, 15, (int)(((1 + rightTrigger)/2)*70), RED);
                }
                else
                {
                    // Draw background: generic
                    RLDrawRectangleRounded((RLRectangle){ 175, 110, 460, 220}, 0.3f, 16, DARKGRAY);

                    // Draw buttons: basic
                    RLDrawCircle(365, 170, 12, RAYWHITE);
                    RLDrawCircle(405, 170, 12, RAYWHITE);
                    RLDrawCircle(445, 170, 12, RAYWHITE);
                    RLDrawCircle(516, 191, 17, RAYWHITE);
                    RLDrawCircle(551, 227, 17, RAYWHITE);
                    RLDrawCircle(587, 191, 17, RAYWHITE);
                    RLDrawCircle(551, 155, 17, RAYWHITE);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE_LEFT)) RLDrawCircle(365, 170, 10, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE)) RLDrawCircle(405, 170, 10, GREEN);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT)) RLDrawCircle(445, 170, 10, BLUE);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) RLDrawCircle(516, 191, 15, GOLD);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) RLDrawCircle(551, 227, 15, BLUE);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) RLDrawCircle(587, 191, 15, GREEN);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP)) RLDrawCircle(551, 155, 15, RED);

                    // Draw buttons: d-pad
                    RLDrawRectangle(245, 145, 28, 88, RAYWHITE);
                    RLDrawRectangle(215, 174, 88, 29, RAYWHITE);
                    RLDrawRectangle(247, 147, 24, 84, BLACK);
                    RLDrawRectangle(217, 176, 84, 25, BLACK);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_UP)) RLDrawRectangle(247, 147, 24, 29, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) RLDrawRectangle(247, 147 + 54, 24, 30, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) RLDrawRectangle(217, 176, 30, 25, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) RLDrawRectangle(217 + 54, 176, 30, 25, RED);

                    // Draw buttons: left-right back
                    RLDrawRectangleRounded((RLRectangle){ 215, 98, 100, 10}, 0.5f, 16, DARKGRAY);
                    RLDrawRectangleRounded((RLRectangle){ 495, 98, 100, 10}, 0.5f, 16, DARKGRAY);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) RLDrawRectangleRounded((RLRectangle){ 215, 98, 100, 10}, 0.5f, 16, RED);
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) RLDrawRectangleRounded((RLRectangle){ 495, 98, 100, 10}, 0.5f, 16, RED);

                    // Draw axis: left joystick
                    RLColor leftGamepadColor = BLACK;
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_THUMB)) leftGamepadColor = RED;
                    RLDrawCircle(345, 260, 40, BLACK);
                    RLDrawCircle(345, 260, 35, LIGHTGRAY);
                    RLDrawCircle(345 + (int)(leftStickX*20), 260 + (int)(leftStickY*20), 25, leftGamepadColor);

                    // Draw axis: right joystick
                    RLColor rightGamepadColor = BLACK;
                    if (RLIsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_THUMB)) rightGamepadColor = RED;
                    RLDrawCircle(465, 260, 40, BLACK);
                    RLDrawCircle(465, 260, 35, LIGHTGRAY);
                    RLDrawCircle(465 + (int)(rightStickX*20), 260 + (int)(rightStickY*20), 25, rightGamepadColor);

                    // Draw axis: left-right triggers
                    RLDrawRectangle(151, 110, 15, 70, GRAY);
                    RLDrawRectangle(644, 110, 15, 70, GRAY);
                    RLDrawRectangle(151, 110, 15, (int)(((1 + leftTrigger)/2)*70), RED);
                    RLDrawRectangle(644, 110, 15, (int)(((1 + rightTrigger)/2)*70), RED);
                }

                RLDrawText(RLTextFormat("DETECTED AXIS [%i]:", RLGetGamepadAxisCount(gamepad)), 10, 50, 10, MAROON);

                for (int i = 0; i < RLGetGamepadAxisCount(gamepad); i++)
                {
                    RLDrawText(RLTextFormat("AXIS %i: %.02f", i, RLGetGamepadAxisMovement(gamepad, i)), 20, 70 + 20*i, 10, DARKGRAY);
                }

                // Draw vibrate button
                RLDrawRectangleRec(vibrateButton, SKYBLUE);
                RLDrawText("VIBRATE", (int)(vibrateButton.x + 14), (int)(vibrateButton.y + 1), 10, DARKGRAY);

                if (RLGetGamepadButtonPressed() != GAMEPAD_BUTTON_UNKNOWN) RLDrawText(RLTextFormat("DETECTED BUTTON: %i", RLGetGamepadButtonPressed()), 10, 430, 10, RED);
                else RLDrawText("DETECTED BUTTON: NONE", 10, 430, 10, GRAY);
            }
            else
            {
                RLDrawText(RLTextFormat("GP%d: NOT DETECTED", gamepad), 10, 10, 10, GRAY);
                RLDrawTexture(texXboxPad, 0, 0, LIGHTGRAY);
            }

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(texPs3Pad);
    RLUnloadTexture(texXboxPad);

    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
