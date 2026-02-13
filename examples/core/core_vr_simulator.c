/*******************************************************************************************
*
*   raylib [core] example - vr simulator
*
*   Example complexity rating: [★★★☆] 3/4
*
*   Example originally created with raylib 2.5, last time updated with raylib 4.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2017-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION        330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION        100
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

    // NOTE: screenWidth/screenHeight should match VR device aspect ratio
    RLInitWindow(screenWidth, screenHeight, "raylib [core] example - vr simulator");

    // VR device parameters definition
    RLVrDeviceInfo device = {
        // Oculus Rift CV1 parameters for simulator
        .hResolution = 2160,                 // Horizontal resolution in pixels
        .vResolution = 1200,                 // Vertical resolution in pixels
        .hScreenSize = 0.133793f,            // Horizontal size in meters
        .vScreenSize = 0.0669f,              // Vertical size in meters
        .eyeToScreenDistance = 0.041f,       // Distance between eye and display in meters
        .lensSeparationDistance = 0.07f,     // Lens separation distance in meters
        .interpupillaryDistance = 0.07f,     // IPD (distance between pupils) in meters

        // NOTE: CV1 uses fresnel-hybrid-asymmetric lenses with specific compute shaders
        // Following parameters are just an approximation to CV1 distortion stereo rendering
        .lensDistortionValues[0] = 1.0f,     // Lens distortion constant parameter 0
        .lensDistortionValues[1] = 0.22f,    // Lens distortion constant parameter 1
        .lensDistortionValues[2] = 0.24f,    // Lens distortion constant parameter 2
        .lensDistortionValues[3] = 0.0f,     // Lens distortion constant parameter 3
        .chromaAbCorrection[0] = 0.996f,     // Chromatic aberration correction parameter 0
        .chromaAbCorrection[1] = -0.004f,    // Chromatic aberration correction parameter 1
        .chromaAbCorrection[2] = 1.014f,     // Chromatic aberration correction parameter 2
        .chromaAbCorrection[3] = 0.0f,       // Chromatic aberration correction parameter 3
    };

    // Load VR stereo config for VR device parameteres (Oculus Rift CV1 parameters)
    RLVrStereoConfig config = RLLoadVrStereoConfig(device);

    // Distortion shader (uses device lens distortion and chroma)
    RLShader distortion = RLLoadShader(0, RLTextFormat("resources/shaders/glsl%i/distortion.fs", GLSL_VERSION));

    // Update distortion shader with lens and distortion-scale parameters
    RLSetShaderValue(distortion, RLGetShaderLocation(distortion, "leftLensCenter"),
                   config.leftLensCenter, SHADER_UNIFORM_VEC2);
    RLSetShaderValue(distortion, RLGetShaderLocation(distortion, "rightLensCenter"),
                   config.rightLensCenter, SHADER_UNIFORM_VEC2);
    RLSetShaderValue(distortion, RLGetShaderLocation(distortion, "leftScreenCenter"),
                   config.leftScreenCenter, SHADER_UNIFORM_VEC2);
    RLSetShaderValue(distortion, RLGetShaderLocation(distortion, "rightScreenCenter"),
                   config.rightScreenCenter, SHADER_UNIFORM_VEC2);

    RLSetShaderValue(distortion, RLGetShaderLocation(distortion, "scale"),
                   config.scale, SHADER_UNIFORM_VEC2);
    RLSetShaderValue(distortion, RLGetShaderLocation(distortion, "scaleIn"),
                   config.scaleIn, SHADER_UNIFORM_VEC2);
    RLSetShaderValue(distortion, RLGetShaderLocation(distortion, "deviceWarpParam"),
                   device.lensDistortionValues, SHADER_UNIFORM_VEC4);
    RLSetShaderValue(distortion, RLGetShaderLocation(distortion, "chromaAbParam"),
                   device.chromaAbCorrection, SHADER_UNIFORM_VEC4);

    // Initialize framebuffer for stereo rendering
    // NOTE: Screen size should match HMD aspect ratio
    RLRenderTexture2D target = RLLoadRenderTexture(device.hResolution, device.vResolution);

    // The target's height is flipped (in the source Rectangle), due to OpenGL reasons
    RLRectangle sourceRec = { 0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height };
    RLRectangle destRec = { 0.0f, 0.0f, (float)RLGetScreenWidth(), (float)RLGetScreenHeight() };

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 5.0f, 2.0f, 5.0f };    // Camera position
    camera.target = (RLVector3){ 0.0f, 2.0f, 0.0f };      // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector
    camera.fovy = 60.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    RLVector3 cubePosition = { 0.0f, 0.0f, 0.0f };

    RLDisableCursor();                    // Limit cursor to relative movement inside the window

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, CAMERA_FIRST_PERSON);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginTextureMode(target);
            RLClearBackground(RAYWHITE);
            RLBeginVrStereoMode(config);
                RLBeginMode3D(camera);

                    RLDrawCube(cubePosition, 2.0f, 2.0f, 2.0f, RED);
                    RLDrawCubeWires(cubePosition, 2.0f, 2.0f, 2.0f, MAROON);
                    RLDrawGrid(40, 1.0f);

                RLEndMode3D();
            RLEndVrStereoMode();
        RLEndTextureMode();

        RLBeginDrawing();
            RLClearBackground(RAYWHITE);
            RLBeginShaderMode(distortion);
                RLDrawTexturePro(target.texture, sourceRec, destRec, (RLVector2){ 0.0f, 0.0f }, 0.0f, WHITE);
            RLEndShaderMode();
            RLDrawFPS(10, 10);
        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadVrStereoConfig(config);   // Unload stereo config

    RLUnloadRenderTexture(target);    // Unload stereo render fbo
    RLUnloadShader(distortion);       // Unload distortion shader

    RLCloseWindow();                  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}