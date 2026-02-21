/*******************************************************************************************
*
*   raylib [shaders] example - custom uniform
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   NOTE: This example requires raylib OpenGL 3.3 or ES2 versions for shaders support,
*         OpenGL 1.1 does not support shaders, recompile raylib to OpenGL 3.3 version
*
*   NOTE: Shaders used in this example are #version 330 (OpenGL 3.3), to test this example
*         on OpenGL ES 2.0 platforms (Android, Raspberry Pi, HTML5), use #version 100 shaders
*         raylib comes with shaders ready for both versions, check raylib/shaders install folder
*
*   Example originally created with raylib 1.3, last time updated with raylib 4.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2015-2025 Ramon Santamaria (@raysan5)
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

    RLSetConfigFlags(RL_E_FLAG_MSAA_4X_HINT);      // Enable Multi Sampling Anti Aliasing 4x (if available)

    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - custom uniform");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 8.0f, 8.0f, 8.0f };    // Camera position
    camera.target = (RLVector3){ 0.0f, 1.5f, 0.0f };      // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = RL_E_CAMERA_PERSPECTIVE;             // Camera projection type

    RLModel model = RLLoadModel("resources/models/barracks.obj");                   // Load OBJ model
    RLTexture2D texture = RLLoadTexture("resources/models/barracks_diffuse.png");   // Load model texture (diffuse map)
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;                     // Set model diffuse texture

    RLVector3 position = { 0.0f, 0.0f, 0.0f };                                    // Set model position

    // Load postprocessing shader
    // NOTE: Defining 0 (NULL) for vertex shader forces usage of internal default vertex shader
    RLShader shader = RLLoadShader(0, RLTextFormat("resources/shaders/glsl%i/swirl.fs", GLSL_VERSION));

    // Get variable (uniform) location on the shader to connect with the program
    // NOTE: If uniform variable could not be found in the shader, function returns -1
    int swirlCenterLoc = RLGetShaderLocation(shader, "center");

    float swirlCenter[2] = { (float)screenWidth/2, (float)screenHeight/2 };

    // Create a RenderTexture2D to be used for render to texture
    RLRenderTexture2D target = RLLoadRenderTexture(screenWidth, screenHeight);

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, RL_E_CAMERA_ORBITAL);

        RLVector2 mousePosition = RLGetMousePosition();

        swirlCenter[0] = mousePosition.x;
        swirlCenter[1] = screenHeight - mousePosition.y;

        // Send new value to the shader to be used on drawing
        RLSetShaderValue(shader, swirlCenterLoc, swirlCenter, RL_E_SHADER_UNIFORM_VEC2);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginTextureMode(target);       // Enable drawing to texture
            RLClearBackground(RAYWHITE);  // Clear texture background

            RLBeginMode3D(camera);        // Begin 3d mode drawing
                RLDrawModel(model, position, 0.5f, WHITE);   // Draw 3d model with texture
                RLDrawGrid(10, 1.0f);     // Draw a grid
            RLEndMode3D();                // End 3d mode drawing, returns to orthographic 2d mode

            RLDrawText("TEXT DRAWN IN RENDER TEXTURE", 200, 10, 30, RED);
        RLEndTextureMode();               // End drawing to texture (now we have a texture available for next passes)

        RLBeginDrawing();
            RLClearBackground(RAYWHITE);  // Clear screen background

            // Enable shader using the custom uniform
            RLBeginShaderMode(shader);
                // NOTE: Render texture must be y-flipped due to default OpenGL coordinates (left-bottom)
                RLDrawTextureRec(target.texture, (RLRectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height }, (RLVector2){ 0, 0 }, WHITE);
            RLEndShaderMode();

            // Draw some 2d text over drawn texture
            RLDrawText("(c) Barracks 3D model by Alberto Cano", screenWidth - 220, screenHeight - 20, 10, GRAY);
            RLDrawFPS(10, 10);
        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadShader(shader);               // Unload shader
    RLUnloadTexture(texture);             // Unload texture
    RLUnloadModel(model);                 // Unload model
    RLUnloadRenderTexture(target);        // Unload render texture

    RLCloseWindow();                      // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}