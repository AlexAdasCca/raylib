/*******************************************************************************************
*
*   raylib [textures] example - npatch drawing
*
*   Example complexity rating: [★★★☆] 3/4
*
*   NOTE: Images are loaded in CPU memory (RAM); textures are loaded in GPU memory (VRAM)
*
*   Example originally created with raylib 2.0, last time updated with raylib 2.5
*
*   Example contributed by Jorge A. Gomes (@overdev) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2018-2025 Jorge A. Gomes (@overdev) and Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [textures] example - npatch drawing");

    // NOTE: Textures MUST be loaded after Window initialization (OpenGL context is required)
    RLTexture2D nPatchTexture = RLLoadTexture("resources/ninepatch_button.png");

    RLVector2 mousePosition = { 0 };
    RLVector2 origin = { 0.0f, 0.0f };

    // Position and size of the n-patches
    RLRectangle dstRec1 = { 480.0f, 160.0f, 32.0f, 32.0f };
    RLRectangle dstRec2 = { 160.0f, 160.0f, 32.0f, 32.0f };
    RLRectangle dstRecH = { 160.0f, 93.0f, 32.0f, 32.0f };
    RLRectangle dstRecV = { 92.0f, 160.0f, 32.0f, 32.0f };

    // A 9-patch (NPATCH_NINE_PATCH) changes its sizes in both axis
    RLNPatchInfo ninePatchInfo1 = { (RLRectangle){ 0.0f, 0.0f, 64.0f, 64.0f }, 12, 40, 12, 12, NPATCH_NINE_PATCH };
    RLNPatchInfo ninePatchInfo2 = { (RLRectangle){ 0.0f, 128.0f, 64.0f, 64.0f }, 16, 16, 16, 16, NPATCH_NINE_PATCH };

    // A horizontal 3-patch (NPATCH_THREE_PATCH_HORIZONTAL) changes its sizes along the x axis only
    RLNPatchInfo h3PatchInfo = { (RLRectangle){ 0.0f,  64.0f, 64.0f, 64.0f }, 8, 8, 8, 8, NPATCH_THREE_PATCH_HORIZONTAL };

    // A vertical 3-patch (NPATCH_THREE_PATCH_VERTICAL) changes its sizes along the y axis only
    RLNPatchInfo v3PatchInfo = { (RLRectangle){ 0.0f, 192.0f, 64.0f, 64.0f }, 6, 6, 6, 6, NPATCH_THREE_PATCH_VERTICAL };

    RLSetTargetFPS(60);
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        mousePosition = RLGetMousePosition();

        // Resize the n-patches based on mouse position
        dstRec1.width = mousePosition.x - dstRec1.x;
        dstRec1.height = mousePosition.y - dstRec1.y;
        dstRec2.width = mousePosition.x - dstRec2.x;
        dstRec2.height = mousePosition.y - dstRec2.y;
        dstRecH.width = mousePosition.x - dstRecH.x;
        dstRecV.height = mousePosition.y - dstRecV.y;

        // Set a minimum width and/or height
        if (dstRec1.width < 1.0f) dstRec1.width = 1.0f;
        if (dstRec1.width > 300.0f) dstRec1.width = 300.0f;
        if (dstRec1.height < 1.0f) dstRec1.height = 1.0f;
        if (dstRec2.width < 1.0f) dstRec2.width = 1.0f;
        if (dstRec2.width > 300.0f) dstRec2.width = 300.0f;
        if (dstRec2.height < 1.0f) dstRec2.height = 1.0f;
        if (dstRecH.width < 1.0f) dstRecH.width = 1.0f;
        if (dstRecV.height < 1.0f) dstRecV.height = 1.0f;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            // Draw the n-patches
            RLDrawTextureNPatch(nPatchTexture, ninePatchInfo2, dstRec2, origin, 0.0f, WHITE);
            RLDrawTextureNPatch(nPatchTexture, ninePatchInfo1, dstRec1, origin, 0.0f, WHITE);
            RLDrawTextureNPatch(nPatchTexture, h3PatchInfo, dstRecH, origin, 0.0f, WHITE);
            RLDrawTextureNPatch(nPatchTexture, v3PatchInfo, dstRecV, origin, 0.0f, WHITE);

            // Draw the source texture
            RLDrawRectangleLines(5, 88, 74, 266, BLUE);
            RLDrawTexture(nPatchTexture, 10, 93, WHITE);
            RLDrawText("TEXTURE", 15, 360, 10, DARKGRAY);

            RLDrawText("Move the mouse to stretch or shrink the n-patches", 10, 20, 20, DARKGRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(nPatchTexture);       // Texture unloading

    RLCloseWindow();                // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
