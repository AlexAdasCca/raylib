/*******************************************************************************************
*
*   raylib [shaders] example - ascii rendering
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 5.5, last time updated with raylib 5.6
*
*   Example contributed by Maicon Santana (@maiconpintoabreu) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Maicon Santana (@maiconpintoabreu)
*
********************************************************************************************/

#include "raylib.h"

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - ascii rendering");

    // Texture to test static drawing
    RLTexture2D fudesumi = RLLoadTexture("resources/fudesumi.png");
    // Texture to test moving drawing
    RLTexture2D raysan = RLLoadTexture("resources/raysan.png");

    // Load shader to be used on postprocessing
    RLShader shader = RLLoadShader(0, RLTextFormat("resources/shaders/glsl%i/ascii.fs", GLSL_VERSION));

    // These locations are used to send data to the GPU
    int resolutionLoc = RLGetShaderLocation(shader, "resolution");
    int fontSizeLoc = RLGetShaderLocation(shader, "fontSize");

    // Set the character size for the ASCII effect
    // Fontsize should be 9 or more
    float fontSize = 9.0f;

    // Send the updated values to the shader
    float resolution[2] = { (float)screenWidth, (float)screenHeight };
    RLSetShaderValue(shader, resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

    RLVector2 circlePos = (RLVector2){40.0f, (float)screenHeight*0.5f};
    float circleSpeed = 1.0f;

    // RenderTexture to apply the postprocessing later
    RLRenderTexture2D target = RLLoadRenderTexture(screenWidth, screenHeight);

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        circlePos.x += circleSpeed;
        if ((circlePos.x > 200.0f) || (circlePos.x < 40.0f)) circleSpeed *= -1; // Revert speed

        if (RLIsKeyPressed(KEY_LEFT) && (fontSize > 9.0)) fontSize -= 1;  // Reduce fontSize
        if (RLIsKeyPressed(KEY_RIGHT) && (fontSize < 15.0)) fontSize += 1;  // Increase fontSize

        // Set fontsize for the shader
        RLSetShaderValue(shader, fontSizeLoc, &fontSize, SHADER_UNIFORM_FLOAT);

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginTextureMode(target);
            RLClearBackground(WHITE);

            // Draw scene in our render texture
            RLDrawTexture(fudesumi, 500, -30, WHITE);
            RLDrawTextureV(raysan, circlePos, WHITE);
        RLEndTextureMode();

        RLBeginDrawing();
            RLClearBackground(RAYWHITE);

            RLBeginShaderMode(shader);
                // Draw the scene texture (that we rendered earlier) to the screen
                // The shader will process every pixel of this texture
                RLDrawTextureRec(target.texture,
                    (RLRectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height },
                    (RLVector2){ 0, 0 }, WHITE);
            RLEndShaderMode();

            RLDrawRectangle(0, 0, screenWidth, 40, BLACK);
            RLDrawText(RLTextFormat("Ascii effect - FontSize:%2.0f - [Left] -1 [Right] +1 ", fontSize), 120, 10, 20, LIGHTGRAY);
            RLDrawFPS(10, 10);
        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadRenderTexture(target);    // Unload render texture

    RLUnloadShader(shader);           // Unload shader
    RLUnloadTexture(fudesumi);        // Unload texture
    RLUnloadTexture(raysan);          // Unload texture

    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
