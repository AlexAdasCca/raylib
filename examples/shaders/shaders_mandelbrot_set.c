/*******************************************************************************************
*
*   raylib [shaders] example - mandelbrot set
*
*   Example complexity rating: [★★★☆] 3/4
*
*   NOTE: This example requires raylib OpenGL 3.3 or ES2 versions for shaders support,
*         OpenGL 1.1 does not support shaders, recompile raylib to OpenGL 3.3 version
*
*   NOTE: Shaders used in this example are #version 330 (OpenGL 3.3)
*
*   Example originally created with raylib 5.6, last time updated with raylib 5.6
*
*   Example contributed by Jordi Santonja (@JordSant)
*   Based on previous work by Josh Colclough (@joshcol9232)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Jordi Santonja (@JordSant)
*
********************************************************************************************/

#include "raylib.h"
#include <math.h>

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

// A few good interesting places
const float pointsOfInterest[6][3] =
{
    { -1.76826775f, -0.00422996283f, 28435.9238f },
    { 0.322004497f, -0.0357099883f, 56499.7266f },
    { -0.748880744f, -0.0562955774f, 9237.59082f },
    { -1.78385007f, -0.0156200649f, 14599.5283f },
    { -0.0985441282f, -0.924688697f, 26259.8535f },
    { 0.317785531f, -0.0322612226f, 29297.9258f },
};

const int screenWidth = 800;
const int screenHeight = 450;
const float zoomSpeed = 1.01f;
const float offsetSpeedMul = 2.0f;

const float startingZoom = 0.6f;
const float startingOffset[2] = { -0.5f, 0.0f };

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - mandelbrot set");

    // Load mandelbrot set shader
    // NOTE: Defining 0 (NULL) for vertex shader forces usage of internal default vertex shader
    RLShader shader = RLLoadShader(0, RLTextFormat("resources/shaders/glsl%i/mandelbrot_set.fs", GLSL_VERSION));

    // Create a RenderTexture2D to be used for render to texture
    RLRenderTexture2D target = RLLoadRenderTexture(RLGetScreenWidth(), RLGetScreenHeight());

    // Offset and zoom to draw the mandelbrot set at. (centered on screen and default size)
    float offset[2] = { startingOffset[0], startingOffset[1] };
    float zoom = startingZoom;
    // Depending on the zoom the mximum number of iterations must be adapted to get more detail as we zzoom in
    // The solution is not perfect, so a control has been added to increase/decrease the number of iterations with UP/DOWN keys
#if defined(PLATFORM_DESKTOP)
    int maxIterations = 333;
    float maxIterationsMultiplier = 166.5f;
#else
    int maxIterations = 43;
    float maxIterationsMultiplier = 22.0f;
#endif

    // Get variable (uniform) locations on the shader to connect with the program
    // NOTE: If uniform variable could not be found in the shader, function returns -1
    int zoomLoc = RLGetShaderLocation(shader, "zoom");
    int offsetLoc = RLGetShaderLocation(shader, "offset");
    int maxIterationsLoc = RLGetShaderLocation(shader, "maxIterations");

    // Upload the shader uniform values!
    RLSetShaderValue(shader, zoomLoc, &zoom, SHADER_UNIFORM_FLOAT);
    RLSetShaderValue(shader, offsetLoc, offset, SHADER_UNIFORM_VEC2);
    RLSetShaderValue(shader, maxIterationsLoc, &maxIterations, SHADER_UNIFORM_INT);

    bool showControls = true;           // Show controls

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        bool updateShader = false;

        // Press [1 - 6] to reset c to a point of interest
        if (RLIsKeyPressed(KEY_ONE) ||
            RLIsKeyPressed(KEY_TWO) ||
            RLIsKeyPressed(KEY_THREE) ||
            RLIsKeyPressed(KEY_FOUR) ||
            RLIsKeyPressed(KEY_FIVE) ||
            RLIsKeyPressed(KEY_SIX))
        {
            int interestIndex = 0;
            if (RLIsKeyPressed(KEY_ONE)) interestIndex = 0;
            else if (RLIsKeyPressed(KEY_TWO)) interestIndex = 1;
            else if (RLIsKeyPressed(KEY_THREE)) interestIndex = 2;
            else if (RLIsKeyPressed(KEY_FOUR)) interestIndex = 3;
            else if (RLIsKeyPressed(KEY_FIVE)) interestIndex = 4;
            else if (RLIsKeyPressed(KEY_SIX)) interestIndex = 5;

            offset[0] = pointsOfInterest[interestIndex][0];
            offset[1] = pointsOfInterest[interestIndex][1];
            zoom = pointsOfInterest[interestIndex][2];
            updateShader = true;
        }

        // If "R" is pressed, reset zoom and offset
        if (RLIsKeyPressed(KEY_R))
        {
            offset[0] = startingOffset[0];
            offset[1] = startingOffset[1];
            zoom = startingZoom;
            updateShader = true;
        }

        if (RLIsKeyPressed(KEY_F1)) showControls = !showControls;  // Toggle whether or not to show controls

        // Change number of max iterations with UP and DOWN keys
        // WARNING: Increasing the number of max iterations greatly impacts performance
        if (RLIsKeyPressed(KEY_UP))
        {
            maxIterationsMultiplier *= 1.4f;
            updateShader = true;
        }
        else if (RLIsKeyPressed(KEY_DOWN))
        {
            maxIterationsMultiplier /= 1.4f;
            updateShader = true;
        }

        // If either left or right button is pressed, zoom in/out
        if (RLIsMouseButtonDown(MOUSE_BUTTON_LEFT) || RLIsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            // Change zoom. If Mouse left -> zoom in. Mouse right -> zoom out
            zoom *= RLIsMouseButtonDown(MOUSE_BUTTON_LEFT)? zoomSpeed : (1.0f/zoomSpeed);

            const RLVector2 mousePos = RLGetMousePosition();
            RLVector2 offsetVelocity;
            // Find the velocity at which to change the camera. Take the distance of the mouse
            // From the center of the screen as the direction, and adjust magnitude based on the current zoom
            offsetVelocity.x = (mousePos.x/(float)screenWidth - 0.5f)*offsetSpeedMul/zoom;
            offsetVelocity.y = (mousePos.y/(float)screenHeight - 0.5f)*offsetSpeedMul/zoom;

            // Apply move velocity to camera
            offset[0] += RLGetFrameTime()*offsetVelocity.x;
            offset[1] += RLGetFrameTime()*offsetVelocity.y;

            updateShader = true;
        }

        // In case a parameter has been changed, update the shader values
        if (updateShader)
        {
            // As we zoom in, increase the number of max iterations to get more detail
            // Aproximate formula, but it works-ish
            maxIterations = (int)(sqrtf(2.0f*sqrtf(fabsf(1.0f - sqrtf(37.5f*zoom))))*maxIterationsMultiplier);

            // Update the shader uniform values!
            RLSetShaderValue(shader, zoomLoc, &zoom, SHADER_UNIFORM_FLOAT);
            RLSetShaderValue(shader, offsetLoc, offset, SHADER_UNIFORM_VEC2);
            RLSetShaderValue(shader, maxIterationsLoc, &maxIterations, SHADER_UNIFORM_INT);
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        // Using a render texture to draw Mandelbrot set
        RLBeginTextureMode(target);       // Enable drawing to texture
            RLClearBackground(BLACK);     // Clear the render texture

            // Draw a rectangle in shader mode to be used as shader canvas
            // NOTE: Rectangle uses font white character texture coordinates,
            // So shader can not be applied here directly because input vertexTexCoord
            // Do not represent full screen coordinates (space where want to apply shader)
            RLDrawRectangle(0, 0, RLGetScreenWidth(), RLGetScreenHeight(), BLACK);
        RLEndTextureMode();

        RLBeginDrawing();
            RLClearBackground(BLACK);     // Clear screen background

            // Draw the saved texture and rendered mandelbrot set with shader
            // NOTE: We do not invert texture on Y, already considered inside shader
            RLBeginShaderMode(shader);
                // WARNING: If FLAG_WINDOW_HIGHDPI is enabled, HighDPI monitor scaling should be considered
                // When rendering the RenderTexture2D to fit in the HighDPI scaled Window
                RLDrawTextureEx(target.texture, (RLVector2){ 0.0f, 0.0f }, 0.0f, 1.0f, WHITE);
            RLEndShaderMode();

            if (showControls)
            {
                RLDrawText("Press Mouse buttons right/left to zoom in/out and move", 10, 15, 10, RAYWHITE);
                RLDrawText("Press F1 to toggle these controls", 10, 30, 10, RAYWHITE);
                RLDrawText("Press [1 - 6] to change point of interest", 10, 45, 10, RAYWHITE);
                RLDrawText("Press UP | DOWN to change number of iterations", 10, 60, 10, RAYWHITE);
                RLDrawText("Press R to recenter the camera", 10, 75, 10, RAYWHITE);
            }
        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadShader(shader);               // Unload shader
    RLUnloadRenderTexture(target);        // Unload render texture

    RLCloseWindow();                      // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
