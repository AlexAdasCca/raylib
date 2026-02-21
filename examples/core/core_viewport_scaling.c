/*******************************************************************************************
*
*   raylib [core] example - viewport scaling
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 5.5, last time updated with raylib 5.5
*
*   Example contributed by Agnis Aldiņš (@nezvers) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Agnis Aldiņš (@nezvers)
*
********************************************************************************************/

#include "raylib.h"

#define RESOLUTION_COUNT    4   // For iteration purposes and teaching example

typedef enum {
    // Only upscale, useful for pixel art
    KEEP_ASPECT_INTEGER,
    KEEP_HEIGHT_INTEGER,
    KEEP_WIDTH_INTEGER,
    // Can also downscale
    KEEP_ASPECT,
    KEEP_HEIGHT,
    KEEP_WIDTH,
    // For itteration purposes and as a teaching example
    VIEWPORT_TYPE_COUNT,
} ViewportType;

// For displaying on GUI
const char *ViewportTypeNames[VIEWPORT_TYPE_COUNT] = {
    "KEEP_ASPECT_INTEGER",
    "KEEP_HEIGHT_INTEGER",
    "KEEP_WIDTH_INTEGER",
    "KEEP_ASPECT",
    "KEEP_HEIGHT",
    "KEEP_WIDTH",
};

//--------------------------------------------------------------------------------------
// Module Functions Declaration
//--------------------------------------------------------------------------------------
static void KeepAspectCenteredInteger(int screenWidth, int screenHeight, int gameWidth, int gameHeight, RLRectangle *sourceRect, RLRectangle *destRect);
static void KeepHeightCenteredInteger(int screenWidth, int screenHeight, int gameWidth, int gameHeight, RLRectangle *sourceRect, RLRectangle *destRect);
static void KeepWidthCenteredInteger(int screenWidth, int screenHeight, int gameWidth, int gameHeight, RLRectangle *sourceRect, RLRectangle *destRect);
static void KeepAspectCentered(int screenWidth, int screenHeight, int gameWidth, int gameHeight, RLRectangle *sourceRect, RLRectangle *destRect);
static void KeepHeightCentered(int screenWidth, int screenHeight, int gameWidth, int gameHeight, RLRectangle *sourceRect, RLRectangle *destRect);
static void KeepWidthCentered(int screenWidth, int screenHeight, int gameWidth, int gameHeight, RLRectangle *sourceRect, RLRectangle *destRect);
static void ResizeRenderSize(ViewportType viewportType, int *screenWidth, int *screenHeight, int gameWidth, int gameHeight, RLRectangle *sourceRect, RLRectangle *destRect, RLRenderTexture2D *target);

// Example how to calculate position on RenderTexture
static RLVector2 Screen2RenderTexturePosition(RLVector2 point, RLRectangle *textureRect, RLRectangle *scaledRect);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //---------------------------------------------------------
    int screenWidth = 800;
    int screenHeight = 450;

    RLSetConfigFlags(RL_E_FLAG_WINDOW_RESIZABLE);
    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - viewport scaling");

    // Preset resolutions that could be created by subdividing screen resolution
    RLVector2 resolutionList[RESOLUTION_COUNT] = {
        (RLVector2){ 64, 64 },
        (RLVector2){ 256, 240 },
        (RLVector2){ 320, 180 },
        // 4K doesn't work with integer scaling but included for example purposes with non-integer scaling
        (RLVector2){ 3840, 2160 },
    };

    int resolutionIndex = 0;
    int gameWidth = 64;
    int gameHeight = 64;

    RLRenderTexture2D target = (RLRenderTexture2D){ 0 };
    RLRectangle sourceRect = (RLRectangle){ 0 };
    RLRectangle destRect = (RLRectangle){ 0 };

    ViewportType viewportType = KEEP_ASPECT_INTEGER;
    ResizeRenderSize(viewportType, &screenWidth, &screenHeight, gameWidth, gameHeight, &sourceRect, &destRect, &target);

    // Button rectangles
    RLRectangle decreaseResolutionButton = (RLRectangle){ 200, 30, 10, 10 };
    RLRectangle increaseResolutionButton = (RLRectangle){ 215, 30, 10, 10 };
    RLRectangle decreaseTypeButton = (RLRectangle){ 200, 45, 10, 10 };
    RLRectangle increaseTypeButton = (RLRectangle){ 215, 45, 10, 10 };

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //----------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (RLIsWindowResized()) ResizeRenderSize(viewportType, &screenWidth, &screenHeight, gameWidth, gameHeight, &sourceRect, &destRect, &target);

        RLVector2 mousePosition = RLGetMousePosition();
        bool mousePressed = RLIsMouseButtonPressed(RL_E_MOUSE_BUTTON_LEFT);

        // Check buttons and rescale
        if (RLCheckCollisionPointRec(mousePosition, decreaseResolutionButton) && mousePressed)
        {
            resolutionIndex = (resolutionIndex + RESOLUTION_COUNT - 1)%RESOLUTION_COUNT;
            gameWidth = (int)resolutionList[resolutionIndex].x;
            gameHeight = (int)resolutionList[resolutionIndex].y;
            ResizeRenderSize(viewportType, &screenWidth, &screenHeight, gameWidth, gameHeight, &sourceRect, &destRect, &target);
        }

        if (RLCheckCollisionPointRec(mousePosition, increaseResolutionButton) && mousePressed)
        {
            resolutionIndex = (resolutionIndex + 1)%RESOLUTION_COUNT;
            gameWidth = (int)resolutionList[resolutionIndex].x;
            gameHeight = (int)resolutionList[resolutionIndex].y;
            ResizeRenderSize(viewportType, &screenWidth, &screenHeight, gameWidth, gameHeight, &sourceRect, &destRect, &target);
        }

        if (RLCheckCollisionPointRec(mousePosition, decreaseTypeButton) && mousePressed)
        {
            viewportType = (viewportType + VIEWPORT_TYPE_COUNT - 1)%VIEWPORT_TYPE_COUNT;
            ResizeRenderSize(viewportType, &screenWidth, &screenHeight, gameWidth, gameHeight, &sourceRect, &destRect, &target);
        }

        if (RLCheckCollisionPointRec(mousePosition, increaseTypeButton) && mousePressed)
        {
            viewportType = (viewportType + 1)%VIEWPORT_TYPE_COUNT;
            ResizeRenderSize(viewportType, &screenWidth, &screenHeight, gameWidth, gameHeight, &sourceRect, &destRect, &target);
        }

        RLVector2 textureMousePosition = Screen2RenderTexturePosition(mousePosition, &sourceRect, &destRect);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        // Draw our scene to the render texture
        RLBeginTextureMode(target);
            RLClearBackground(WHITE);
            RLDrawCircleV(textureMousePosition, 20.0f, LIME);
        RLEndTextureMode();

        // Draw render texture to main framebuffer
        RLBeginDrawing();
            RLClearBackground(BLACK);

            // Draw our render texture with rotation applied
            RLDrawTexturePro(target.texture, sourceRect, destRect, (RLVector2){ 0.0f, 0.0f }, 0.0f, WHITE);

            // Draw Native resolution (GUI or anything)
            // Draw info box
            RLRectangle infoRect = (RLRectangle){5, 5, 330, 105};
            RLDrawRectangleRec(infoRect, RLFade(LIGHTGRAY, 0.7f));
            RLDrawRectangleLinesEx(infoRect, 1, BLUE);

            RLDrawText(RLTextFormat("Window Resolution: %d x %d", screenWidth, screenHeight), 15, 15, 10, BLACK);
            RLDrawText(RLTextFormat("Game Resolution: %d x %d", gameWidth, gameHeight), 15, 30, 10, BLACK);

            RLDrawText(RLTextFormat("Type: %s", ViewportTypeNames[viewportType]), 15, 45, 10, BLACK);
            RLVector2 scaleRatio = (RLVector2){destRect.width/sourceRect.width, -destRect.height/sourceRect.height};
            if (scaleRatio.x < 0.001f || scaleRatio.y < 0.001f) RLDrawText(RLTextFormat("Scale ratio: INVALID"), 15, 60, 10, BLACK);
            else RLDrawText(RLTextFormat("Scale ratio: %.2f x %.2f", scaleRatio.x, scaleRatio.y), 15, 60, 10, BLACK);

            RLDrawText(RLTextFormat("Source size: %.2f x %.2f", sourceRect.width, -sourceRect.height), 15, 75, 10, BLACK);
            RLDrawText(RLTextFormat("Destination size: %.2f x %.2f", destRect.width, destRect.height), 15, 90, 10, BLACK);

            // Draw buttons
            RLDrawRectangleRec(decreaseTypeButton, SKYBLUE);
            RLDrawRectangleRec(increaseTypeButton, SKYBLUE);
            RLDrawRectangleRec(decreaseResolutionButton, SKYBLUE);
            RLDrawRectangleRec(increaseResolutionButton, SKYBLUE);
            RLDrawText("<", decreaseTypeButton.x + 3, decreaseTypeButton.y + 1, 10, BLACK);
            RLDrawText(">", increaseTypeButton.x + 3, increaseTypeButton.y + 1, 10, BLACK);
            RLDrawText("<", decreaseResolutionButton.x + 3, decreaseResolutionButton.y + 1, 10, BLACK);
            RLDrawText(">", increaseResolutionButton.x + 3, increaseResolutionButton.y + 1, 10, BLACK);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //----------------------------------------------------------------------------------
    RLCloseWindow();        // Close window and OpenGL context
    //----------------------------------------------------------------------------------

    return 0;
}

//--------------------------------------------------------------------------------------
// Module Functions Definition
//--------------------------------------------------------------------------------------
static void KeepAspectCenteredInteger(int screenWidth, int screenHeight, int gameWidth, int gameHeight, RLRectangle *sourceRect, RLRectangle *destRect)
{
    sourceRect->x = 0.0f;
    sourceRect->y = (float)gameHeight;
    sourceRect->width = (float)gameWidth;
    sourceRect->height = (float)-gameHeight;

    const int ratio_x = (screenWidth/gameWidth);
    const int ratio_y = (screenHeight/gameHeight);
    const float resizeRatio = (float)((ratio_x < ratio_y)? ratio_x : ratio_y);

    destRect->x = (float)(int)((screenWidth - (gameWidth*resizeRatio))*0.5f);
    destRect->y = (float)(int)((screenHeight - (gameHeight*resizeRatio))*0.5f);
    destRect->width = (float)(int)(gameWidth*resizeRatio);
    destRect->height = (float)(int)(gameHeight*resizeRatio);
}

static void KeepHeightCenteredInteger(int screenWidth, int screenHeight, int gameWidth, int gameHeight, RLRectangle *sourceRect, RLRectangle *destRect)
{
    const float resizeRatio = (float)screenHeight/gameHeight;
    sourceRect->x = 0.0f;
    sourceRect->y = 0.0f;
    sourceRect->width = (float)(int)(screenWidth/resizeRatio);
    sourceRect->height = (float)-gameHeight;

    destRect->x = (float)(int)((screenWidth - (sourceRect->width*resizeRatio))*0.5f);
    destRect->y = (float)(int)((screenHeight - (gameHeight*resizeRatio))*0.5f);
    destRect->width = (float)(int)(sourceRect->width*resizeRatio);
    destRect->height = (float)(int)(gameHeight*resizeRatio);
}

static void KeepWidthCenteredInteger(int screenWidth, int screenHeight, int gameWidth, int gameHeight, RLRectangle *sourceRect, RLRectangle *destRect)
{
    const float resizeRatio = (float)screenWidth/gameWidth;
    sourceRect->x = 0.0f;
    sourceRect->y = 0.0f;
    sourceRect->width = (float)gameWidth;
    sourceRect->height = (float)(int)(screenHeight/resizeRatio);

    destRect->x = (float)(int)((screenWidth - (gameWidth*resizeRatio))*0.5f);
    destRect->y = (float)(int)((screenHeight - (sourceRect->height*resizeRatio))*0.5f);
    destRect->width = (float)(int)(gameWidth*resizeRatio);
    destRect->height = (float)(int)(sourceRect->height*resizeRatio);

    sourceRect->height *= -1.0f;
}

static void KeepAspectCentered(int screenWidth, int screenHeight, int gameWidth, int gameHeight, RLRectangle *sourceRect, RLRectangle *destRect)
{
    sourceRect->x = 0.0f;
    sourceRect->y = (float)gameHeight;
    sourceRect->width = (float)gameWidth;
    sourceRect->height = (float)-gameHeight;

    const float ratio_x = ((float)screenWidth/(float)gameWidth);
    const float ratio_y = ((float)screenHeight/(float)gameHeight);
    const float resizeRatio = (ratio_x < ratio_y ? ratio_x : ratio_y);

    destRect->x = (float)(int)((screenWidth - (gameWidth*resizeRatio))*0.5f);
    destRect->y = (float)(int)((screenHeight - (gameHeight*resizeRatio))*0.5f);
    destRect->width = (float)(int)(gameWidth*resizeRatio);
    destRect->height = (float)(int)(gameHeight*resizeRatio);
}

static void KeepHeightCentered(int screenWidth, int screenHeight, int gameWidth, int gameHeight, RLRectangle *sourceRect, RLRectangle *destRect)
{
    const float resizeRatio = ((float)screenHeight/(float)gameHeight);
    sourceRect->x = 0.0f;
    sourceRect->y = 0.0f;
    sourceRect->width = (float)(int)((float)screenWidth/resizeRatio);
    sourceRect->height = (float)-gameHeight;

    destRect->x = (float)(int)((screenWidth - (sourceRect->width*resizeRatio))*0.5f);
    destRect->y = (float)(int)((screenHeight - (gameHeight*resizeRatio))*0.5f);
    destRect->width = (float)(int)(sourceRect->width*resizeRatio);
    destRect->height = (float)(int)(gameHeight*resizeRatio);
}

static void KeepWidthCentered(int screenWidth, int screenHeight, int gameWidth, int gameHeight, RLRectangle *sourceRect, RLRectangle *destRect)
{
    const float resizeRatio = ((float)screenWidth/(float)gameWidth);
    sourceRect->x = 0.0f;
    sourceRect->y = 0.0f;
    sourceRect->width = (float)gameWidth;
    sourceRect->height = (float)(int)((float)screenHeight/resizeRatio);

    destRect->x = (float)(int)((screenWidth - (gameWidth*resizeRatio))*0.5f);
    destRect->y = (float)(int)((screenHeight - (sourceRect->height*resizeRatio))*0.5f);
    destRect->width = (float)(int)(gameWidth*resizeRatio);
    destRect->height = (float)(int)(sourceRect->height*resizeRatio);

    sourceRect->height *= -1.f;
}

static void ResizeRenderSize(ViewportType viewportType, int *screenWidth, int *screenHeight, int gameWidth, int gameHeight, RLRectangle *sourceRect, RLRectangle *destRect, RLRenderTexture2D *target)
{
    *screenWidth = RLGetScreenWidth();
    *screenHeight = RLGetScreenHeight();

    switch(viewportType)
    {
        case KEEP_ASPECT_INTEGER: KeepAspectCenteredInteger(*screenWidth, *screenHeight, gameWidth, gameHeight, sourceRect, destRect); break;
        case KEEP_HEIGHT_INTEGER: KeepHeightCenteredInteger(*screenWidth, *screenHeight, gameWidth, gameHeight, sourceRect, destRect); break;
        case KEEP_WIDTH_INTEGER: KeepWidthCenteredInteger(*screenWidth, *screenHeight, gameWidth, gameHeight, sourceRect, destRect); break;
        case KEEP_ASPECT: KeepAspectCentered(*screenWidth, *screenHeight, gameWidth, gameHeight, sourceRect, destRect); break;
        case KEEP_HEIGHT: KeepHeightCentered(*screenWidth, *screenHeight, gameWidth, gameHeight, sourceRect, destRect); break;
        case KEEP_WIDTH: KeepWidthCentered(*screenWidth, *screenHeight, gameWidth, gameHeight, sourceRect, destRect); break;
        default: break;
    }

    RLUnloadRenderTexture(*target);
    *target = RLLoadRenderTexture(sourceRect->width, -sourceRect->height);
}

// Example how to calculate position on RenderTexture
static RLVector2 Screen2RenderTexturePosition(RLVector2 point, RLRectangle *textureRect, RLRectangle *scaledRect)
{
    RLVector2 relativePosition = {point.x - scaledRect->x, point.y - scaledRect->y};
    RLVector2 ratio = {textureRect->width/scaledRect->width, -textureRect->height/scaledRect->height};

    return (RLVector2){relativePosition.x*ratio.x, relativePosition.y*ratio.x};
}