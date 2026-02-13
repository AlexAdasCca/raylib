/*******************************************************************************************
*
*   raylib [textures] example - sprite explosion
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 2.5, last time updated with raylib 3.5
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2019-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#define NUM_FRAMES_PER_LINE     5
#define NUM_LINES               5

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [textures] example - sprite explosion");

    RLInitAudioDevice();

    // Load explosion sound
    RLSound fxBoom = RLLoadSound("resources/boom.wav");

    // Load explosion texture
    RLTexture2D explosion = RLLoadTexture("resources/explosion.png");

    // Init variables for animation
    float frameWidth = (float)explosion.width/NUM_FRAMES_PER_LINE;   // Sprite one frame rectangle width
    float frameHeight = (float)explosion.height/NUM_LINES;           // Sprite one frame rectangle height
    int currentFrame = 0;
    int currentLine = 0;

    RLRectangle frameRec = { 0, 0, frameWidth, frameHeight };
    RLVector2 position = { 0.0f, 0.0f };

    bool active = false;
    int framesCounter = 0;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------

        // Check for mouse button pressed and activate explosion (if not active)
        if (RLIsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !active)
        {
            position = RLGetMousePosition();
            active = true;

            position.x -= frameWidth/2.0f;
            position.y -= frameHeight/2.0f;

            RLPlaySound(fxBoom);
        }

        // Compute explosion animation frames
        if (active)
        {
            framesCounter++;

            if (framesCounter > 2)
            {
                currentFrame++;

                if (currentFrame >= NUM_FRAMES_PER_LINE)
                {
                    currentFrame = 0;
                    currentLine++;

                    if (currentLine >= NUM_LINES)
                    {
                        currentLine = 0;
                        active = false;
                    }
                }

                framesCounter = 0;
            }
        }

        frameRec.x = frameWidth*currentFrame;
        frameRec.y = frameHeight*currentLine;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            // Draw explosion required frame rectangle
            if (active) RLDrawTextureRec(explosion, frameRec, position, WHITE);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(explosion);   // Unload texture
    RLUnloadSound(fxBoom);        // Unload sound

    RLCloseAudioDevice();

    RLCloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}