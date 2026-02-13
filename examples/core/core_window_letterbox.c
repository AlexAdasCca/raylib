/*******************************************************************************************
*
*   raylib [core] example - window letterbox
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 2.5, last time updated with raylib 4.0
*
*   Example contributed by Anata (@anatagawa) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2019-2025 Anata (@anatagawa) and Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#include "raymath.h"        // Required for: Vector2Clamp()

#define MAX(a, b) ((a)>(b)? (a) : (b))
#define MIN(a, b) ((a)<(b)? (a) : (b))

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    // Enable config flags for resizable window and vertical synchro
    RLSetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - window letterbox");
    RLSetWindowMinSize(320, 240);

    int gameScreenWidth = 640;
    int gameScreenHeight = 480;

    // Render texture initialization, used to hold the rendering result so we can easily resize it
    RLRenderTexture2D target = RLLoadRenderTexture(gameScreenWidth, gameScreenHeight);
    RLSetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);  // Texture scale filter to use

    RLColor colors[10] = { 0 };
    for (int i = 0; i < 10; i++) colors[i] = (RLColor){ RLGetRandomValue(100, 250), RLGetRandomValue(50, 150), RLGetRandomValue(10, 100), 255 };

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // Compute required framebuffer scaling
        float scale = MIN((float)RLGetScreenWidth()/gameScreenWidth, (float)RLGetScreenHeight()/gameScreenHeight);

        if (RLIsKeyPressed(KEY_SPACE))
        {
            // Recalculate random colors for the bars
            for (int i = 0; i < 10; i++) colors[i] = (RLColor){ RLGetRandomValue(100, 250), RLGetRandomValue(50, 150), RLGetRandomValue(10, 100), 255 };
        }

        // Update virtual mouse (clamped mouse value behind game screen)
        RLVector2 mouse = RLGetMousePosition();
        RLVector2 virtualMouse = { 0 };
        virtualMouse.x = (mouse.x - (RLGetScreenWidth() - (gameScreenWidth*scale))*0.5f)/scale;
        virtualMouse.y = (mouse.y - (RLGetScreenHeight() - (gameScreenHeight*scale))*0.5f)/scale;
        virtualMouse = Vector2Clamp(virtualMouse, (RLVector2){ 0, 0 }, (RLVector2){ (float)gameScreenWidth, (float)gameScreenHeight });

        // Apply the same transformation as the virtual mouse to the real mouse (i.e. to work with raygui)
        //SetMouseOffset(-(GetScreenWidth() - (gameScreenWidth*scale))*0.5f, -(GetScreenHeight() - (gameScreenHeight*scale))*0.5f);
        //SetMouseScale(1/scale, 1/scale);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        // Draw everything in the render texture, note this will not be rendered on screen, yet
        RLBeginTextureMode(target);
            RLClearBackground(RAYWHITE);  // Clear render texture background color

            for (int i = 0; i < 10; i++) RLDrawRectangle(0, (gameScreenHeight/10)*i, gameScreenWidth, gameScreenHeight/10, colors[i]);

            RLDrawText("If executed inside a window,\nyou can resize the window,\nand see the screen scaling!", 10, 25, 20, WHITE);
            RLDrawText(RLTextFormat("Default Mouse: [%i , %i]", (int)mouse.x, (int)mouse.y), 350, 25, 20, GREEN);
            RLDrawText(RLTextFormat("Virtual Mouse: [%i , %i]", (int)virtualMouse.x, (int)virtualMouse.y), 350, 55, 20, YELLOW);
        RLEndTextureMode();

        RLBeginDrawing();
            RLClearBackground(BLACK);     // Clear screen background

            // Draw render texture to screen, properly scaled
            RLDrawTexturePro(target.texture, (RLRectangle){ 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height },
                           (RLRectangle){ (RLGetScreenWidth() - ((float)gameScreenWidth*scale))*0.5f, (RLGetScreenHeight() - ((float)gameScreenHeight*scale))*0.5f,
                           (float)gameScreenWidth*scale, (float)gameScreenHeight*scale }, (RLVector2){ 0, 0 }, 0.0f, WHITE);
        RLEndDrawing();
        //--------------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadRenderTexture(target);        // Unload render texture

    RLCloseWindow();                      // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
