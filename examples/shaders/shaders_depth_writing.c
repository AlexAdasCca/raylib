/*******************************************************************************************
*
*   raylib [shaders] example - depth writing
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 4.2, last time updated with raylib 4.2
*
*   Example contributed by Buğra Alptekin Sarı (@BugraAlptekinSari) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2022-2025 Buğra Alptekin Sarı (@BugraAlptekinSari)
*
********************************************************************************************/

#include "raylib.h"

#include "rlgl.h"

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

//--------------------------------------------------------------------------------------
// Module Functions Declaration
//--------------------------------------------------------------------------------------
// Load custom render texture, create a writable depth texture buffer
static RLRenderTexture2D LoadRenderTextureDepthTex(int width, int height);

// Unload render texture from GPU memory (VRAM)
static void UnloadRenderTextureDepthTex(RLRenderTexture2D target);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - depth writing");

    // Define the camera to look into our 3d world
    RLCamera camera = {
        .position = (RLVector3){ 2.0f, 2.0f, 3.0f },    // Camera position
        .target = (RLVector3){ 0.0f, 0.5f, 0.0f },      // Camera looking at point
        .up = (RLVector3){ 0.0f, 1.0f, 0.0f },          // Camera up vector (rotation towards target)
        .fovy = 45.0f,                                // Camera field-of-view Y
        .projection = RL_E_CAMERA_PERSPECTIVE              // Camera projection type
    };

    // Load custom render texture with writable depth texture buffer
    RLRenderTexture2D target = LoadRenderTextureDepthTex(screenWidth, screenHeight);

    // Load depth writing shader
    // NOTE: The shader inverts the depth buffer by writing into it by `gl_FragDepth = 1 - gl_FragCoord.z;`
    RLShader shader = RLLoadShader(0, RLTextFormat("resources/shaders/glsl%i/depth_write.fs", GLSL_VERSION));
    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, RL_E_CAMERA_ORBITAL);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        // Draw into our custom render texture
        RLBeginTextureMode(target);
            RLClearBackground(WHITE);

            RLBeginMode3D(camera);
                RLBeginShaderMode(shader);
                    RLDrawCubeWiresV((RLVector3){ 0.0f, 0.5f, 1.0f }, (RLVector3){ 1.0f, 1.0f, 1.0f }, RED);
                    RLDrawCubeV((RLVector3){ 0.0f, 0.5f, 1.0f }, (RLVector3){ 1.0f, 1.0f, 1.0f }, PURPLE);
                    RLDrawCubeWiresV((RLVector3){ 0.0f, 0.5f, -1.0f }, (RLVector3){ 1.0f, 1.0f, 1.0f }, DARKGREEN);
                    RLDrawCubeV((RLVector3) { 0.0f, 0.5f, -1.0f }, (RLVector3){ 1.0f, 1.0f, 1.0f }, YELLOW);
                    RLDrawGrid(10, 1.0f);
                RLEndShaderMode();
            RLEndMode3D();
        RLEndTextureMode();

        // Draw into screen our custom render texture
        RLBeginDrawing();
            RLClearBackground(RAYWHITE);

            RLDrawTextureRec(target.texture, (RLRectangle) { 0, 0, (float)screenWidth, (float)-screenHeight }, (RLVector2) { 0, 0 }, WHITE);

            RLDrawFPS(10, 10);
        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadRenderTextureDepthTex(target);
    RLUnloadShader(shader);

    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//--------------------------------------------------------------------------------------
// Module Functions Definition
//--------------------------------------------------------------------------------------
// Load custom render texture, create a writable depth texture buffer
static RLRenderTexture2D LoadRenderTextureDepthTex(int width, int height)
{
    RLRenderTexture2D target = { 0 };

    target.id = rlLoadFramebuffer(); // Load an empty framebuffer

    if (target.id > 0)
    {
        rlEnableFramebuffer(target.id);

        // Create color texture (default to RGBA)
        target.texture.id = rlLoadTexture(0, width, height, RL_E_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
        target.texture.width = width;
        target.texture.height = height;
        target.texture.format = RL_E_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        target.texture.mipmaps = 1;

        // Create depth texture buffer (instead of raylib default renderbuffer)
        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19;       // DEPTH_COMPONENT_24BIT: Not defined in raylib
        target.depth.mipmaps = 1;

        // Attach color texture and depth texture to FBO
        rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.id)) TRACELOG(RL_E_LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

        rlDisableFramebuffer();
    }
    else TRACELOG(RL_E_LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

// Unload render texture from GPU memory (VRAM)
void UnloadRenderTextureDepthTex(RLRenderTexture2D target)
{
    if (target.id > 0)
    {
        // Color texture attached to FBO is deleted
        rlUnloadTexture(target.texture.id);
        rlUnloadTexture(target.depth.id);

        // NOTE: Depth texture is automatically
        // queried and deleted before deleting framebuffer
        rlUnloadFramebuffer(target.id);
    }
}