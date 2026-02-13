/*******************************************************************************************
*
*   raylib [textures] example - sprite button
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 2.5, last time updated with raylib 2.5
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2019-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#define NUM_FRAMES  3       // Number of frames (rectangles) for the button sprite texture

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [textures] example - sprite button");

    RLInitAudioDevice();      // Initialize audio device

    RLSound fxButton = RLLoadSound("resources/buttonfx.wav");   // Load button sound
    RLTexture2D button = RLLoadTexture("resources/button.png"); // Load button texture

    // Define frame rectangle for drawing
    float frameHeight = (float)button.height/NUM_FRAMES;
    RLRectangle sourceRec = { 0, 0, (float)button.width, frameHeight };

    // Define button bounds on screen
    RLRectangle btnBounds = { screenWidth/2.0f - button.width/2.0f, screenHeight/2.0f - (float)button.height/NUM_FRAMES/2.0f, (float)button.width, frameHeight };

    int btnState = 0;               // Button state: 0-NORMAL, 1-MOUSE_HOVER, 2-PRESSED
    bool btnAction = false;         // Button action should be activated

    RLVector2 mousePoint = { 0.0f, 0.0f };

    RLSetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        mousePoint = RLGetMousePosition();
        btnAction = false;

        // Check button state
        if (RLCheckCollisionPointRec(mousePoint, btnBounds))
        {
            if (RLIsMouseButtonDown(MOUSE_BUTTON_LEFT)) btnState = 2;
            else btnState = 1;

            if (RLIsMouseButtonReleased(MOUSE_BUTTON_LEFT)) btnAction = true;
        }
        else btnState = 0;

        if (btnAction)
        {
            RLPlaySound(fxButton);

            // TODO: Any desired action
        }

        // Calculate button frame rectangle to draw depending on button state
        sourceRec.y = btnState*frameHeight;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLDrawTextureRec(button, sourceRec, (RLVector2){ btnBounds.x, btnBounds.y }, WHITE); // Draw button frame

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(button);  // Unload button texture
    RLUnloadSound(fxButton);  // Unload sound

    RLCloseAudioDevice();     // Close audio device

    RLCloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}