/*******************************************************************************************
*
*   raylib [shaders] example - depth rendering
*
*   Example complexity rating: [★★★☆] 3/4
*
*   Example originally created with raylib 5.6-dev, last time updated with raylib 5.6-dev
*
*   Example contributed by Luís Almeida (@luis605) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Luís Almeida (@luis605)
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
// Load custom render texture with depth texture attached
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

    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - depth rendering");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 4.0f, 1.0f, 5.0f };
    camera.target = (RLVector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Load render texture with a depth texture attached
    RLRenderTexture2D target = LoadRenderTextureDepthTex(screenWidth, screenHeight);

    // Load depth shader and get depth texture shader location
    RLShader depthShader = RLLoadShader(0, RLTextFormat("resources/shaders/glsl%i/depth_render.fs", GLSL_VERSION));
    int depthLoc = RLGetShaderLocation(depthShader, "depthTexture");
    int flipTextureLoc = RLGetShaderLocation(depthShader, "flipY");
    RLSetShaderValue(depthShader, flipTextureLoc, (int[]){ 1 }, SHADER_UNIFORM_INT); // Flip Y texture

    // Load scene models
    RLModel cube = RLLoadModelFromMesh(RLGenMeshCube(1.0f, 1.0f, 1.0f));
    RLModel floor = RLLoadModelFromMesh(RLGenMeshPlane(20.0f, 20.0f, 1, 1));

    RLDisableCursor();  // Limit cursor to relative movement inside the window

    RLSetTargetFPS(60); // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, CAMERA_FREE);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginTextureMode(target);
            RLClearBackground(WHITE);

            RLBeginMode3D(camera);
                RLDrawModel(cube, (RLVector3){ 0.0f, 0.0f, 0.0f }, 3.0f, YELLOW);
                RLDrawModel(floor, (RLVector3){ 10.0f, 0.0f, 2.0f }, 2.0f, RED);
            RLEndMode3D();
        RLEndTextureMode();

        // Draw into screen (main framebuffer)
        RLBeginDrawing();
            RLClearBackground(RAYWHITE);

            RLBeginShaderMode(depthShader);
                RLSetShaderValueTexture(depthShader, depthLoc, target.depth);
                RLDrawTexture(target.depth, 0, 0, WHITE);
            RLEndShaderMode();

            RLDrawRectangle( 10, 10, 320, 93, RLFade(SKYBLUE, 0.5f));
            RLDrawRectangleLines( 10, 10, 320, 93, BLUE);

            RLDrawText("Camera Controls:", 20, 20, 10, BLACK);
            RLDrawText("- WASD to move", 40, 40, 10, DARKGRAY);
            RLDrawText("- Mouse Wheel Pressed to Pan", 40, 60, 10, DARKGRAY);
            RLDrawText("- Z to zoom to (0, 0, 0)", 40, 80, 10, DARKGRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadModel(cube);              // Unload model
    RLUnloadModel(floor);             // Unload model
    UnloadRenderTextureDepthTex(target);
    RLUnloadShader(depthShader);      // Unload shader

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
        target.texture.id = rlLoadTexture(0, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
        target.texture.width = width;
        target.texture.height = height;
        target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
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
        if (rlFramebufferComplete(target.id)) TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

        rlDisableFramebuffer();
    }
    else TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

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