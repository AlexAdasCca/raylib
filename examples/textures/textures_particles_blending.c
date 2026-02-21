/*******************************************************************************************
*
*   raylib [textures] example - particles blending
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 1.7, last time updated with raylib 3.5
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2017-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#define MAX_PARTICLES 200

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Particle structure
typedef struct {
    RLVector2 position;
    RLColor color;
    float alpha;
    float size;
    float rotation;
    bool active;        // NOTE: Use it to activate/deactive particle
} Particle;

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [textures] example - particles blending");

    // Particles pool, reuse them!
    Particle mouseTail[MAX_PARTICLES] = { 0 };

    // Initialize particles
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        mouseTail[i].position = (RLVector2){ 0, 0 };
        mouseTail[i].color = (RLColor){ RLGetRandomValue(0, 255), RLGetRandomValue(0, 255), RLGetRandomValue(0, 255), 255 };
        mouseTail[i].alpha = 1.0f;
        mouseTail[i].size = (float)RLGetRandomValue(1, 30)/20.0f;
        mouseTail[i].rotation = (float)RLGetRandomValue(0, 360);
        mouseTail[i].active = false;
    }

    float gravity = 3.0f;

    RLTexture2D smoke = RLLoadTexture("resources/spark_flame.png");

    int blending = RL_E_BLEND_ALPHA;

    RLSetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------

        // Activate one particle every frame and Update active particles
        // NOTE: Particles initial position should be mouse position when activated
        // NOTE: Particles fall down with gravity and rotation... and disappear after 2 seconds (alpha = 0)
        // NOTE: When a particle disappears, active = false and it can be reused
        for (int i = 0; i < MAX_PARTICLES; i++)
        {
            if (!mouseTail[i].active)
            {
                mouseTail[i].active = true;
                mouseTail[i].alpha = 1.0f;
                mouseTail[i].position = RLGetMousePosition();
                i = MAX_PARTICLES;
            }
        }

        for (int i = 0; i < MAX_PARTICLES; i++)
        {
            if (mouseTail[i].active)
            {
                mouseTail[i].position.y += gravity/2;
                mouseTail[i].alpha -= 0.005f;

                if (mouseTail[i].alpha <= 0.0f) mouseTail[i].active = false;

                mouseTail[i].rotation += 2.0f;
            }
        }

        if (RLIsKeyPressed(RL_E_KEY_SPACE))
        {
            if (blending == RL_E_BLEND_ALPHA) blending = RL_E_BLEND_ADDITIVE;
            else blending = RL_E_BLEND_ALPHA;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(DARKGRAY);

            RLBeginBlendMode(blending);

                // Draw active particles
                for (int i = 0; i < MAX_PARTICLES; i++)
                {
                    if (mouseTail[i].active) RLDrawTexturePro(smoke, (RLRectangle){ 0.0f, 0.0f, (float)smoke.width, (float)smoke.height },
                                                           (RLRectangle){ mouseTail[i].position.x, mouseTail[i].position.y, smoke.width*mouseTail[i].size, smoke.height*mouseTail[i].size },
                                                           (RLVector2){ (float)(smoke.width*mouseTail[i].size/2.0f), (float)(smoke.height*mouseTail[i].size/2.0f) }, mouseTail[i].rotation,
                                                           RLFade(mouseTail[i].color, mouseTail[i].alpha));
                }

            RLEndBlendMode();

            RLDrawText("PRESS SPACE to CHANGE BLENDING MODE", 180, 20, 20, BLACK);

            if (blending == RL_E_BLEND_ALPHA) RLDrawText("ALPHA BLENDING", 290, screenHeight - 40, 20, BLACK);
            else RLDrawText("ADDITIVE BLENDING", 280, screenHeight - 40, 20, RAYWHITE);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(smoke);

    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}