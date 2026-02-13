/*******************************************************************************************
*
*   raylib [textures] example - framebuffer rendering
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 5.6, last time updated with raylib 5.6
*
*   Example contributed by Jack Boakes (@jackboakes) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2026 Jack Boakes (@jackboakes)
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"

//------------------------------------------------------------------------------------
// Module Functions Declaration
//------------------------------------------------------------------------------------
static void DrawCameraPrism(RLCamera3D camera, float aspect, RLColor color);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;
    const int splitWidth = screenWidth/2;

    RLInitWindow(screenWidth, screenHeight, "raylib [textures] example - framebuffer rendering");

    // Camera to look at the 3D world
    RLCamera3D subjectCamera = { 0 };
    subjectCamera.position = (RLVector3){ 5.0f, 5.0f, 5.0f };
    subjectCamera.target = (RLVector3){ 0.0f, 0.0f, 0.0f };
    subjectCamera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };
    subjectCamera.fovy = 45.0f;
    subjectCamera.projection = CAMERA_PERSPECTIVE;

    // Camera to observe the subject camera and 3D world
    RLCamera3D observerCamera = { 0 };
    observerCamera.position = (RLVector3){ 10.0f, 10.0f, 10.0f };
    observerCamera.target = (RLVector3){ 0.0f, 0.0f, 0.0f };
    observerCamera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };
    observerCamera.fovy = 45.0f;
    observerCamera.projection = CAMERA_PERSPECTIVE;

    // Set up render textures
    RLRenderTexture2D observerTarget = RLLoadRenderTexture(splitWidth, screenHeight);
    RLRectangle observerSource = { 0.0f, 0.0f, (float)observerTarget.texture.width, -(float)observerTarget.texture.height };
    RLRectangle observerDest = { 0.0f, 0.0f, (float)splitWidth, (float)screenHeight };

    RLRenderTexture2D subjectTarget = RLLoadRenderTexture(splitWidth, screenHeight);
    RLRectangle subjectSource = { 0.0f, 0.0f, (float)subjectTarget.texture.width, -(float)subjectTarget.texture.height };
    RLRectangle subjectDest = { (float)splitWidth, 0.0f, (float)splitWidth, (float)screenHeight };
    const float textureAspectRatio = (float)subjectTarget.texture.width/(float)subjectTarget.texture.height;

    // Rectangles for cropping render texture
    const float captureSize = 128.0f;
    RLRectangle cropSource = { (subjectTarget.texture.width - captureSize)/2.0f, (subjectTarget.texture.height - captureSize)/2.0f, captureSize, -captureSize };
    RLRectangle cropDest = { splitWidth + 20, 20, captureSize, captureSize};

    RLSetTargetFPS(60);
    RLDisableCursor();
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&observerCamera, CAMERA_FREE);
        RLUpdateCamera(&subjectCamera, CAMERA_ORBITAL);

        if (RLIsKeyPressed(KEY_R)) observerCamera.target = (RLVector3){ 0.0f, 0.0f, 0.0f };

        // Build LHS observer view texture
        RLBeginTextureMode(observerTarget);

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(observerCamera);

                RLDrawGrid(10, 1.0f);
                RLDrawCube((RLVector3){ 0.0f, 0.0f, 0.0f }, 2.0f, 2.0f, 2.0f, GOLD);
                RLDrawCubeWires((RLVector3){ 0.0f, 0.0f, 0.0f }, 2.0f, 2.0f, 2.0f, PINK);
                DrawCameraPrism(subjectCamera, textureAspectRatio, GREEN);

            RLEndMode3D();

            RLDrawText("Observer View", 10, observerTarget.texture.height - 30, 20, BLACK);
            RLDrawText("WASD + Mouse to Move", 10, 10, 20, DARKGRAY);
            RLDrawText("Scroll to Zoom", 10, 30, 20, DARKGRAY);
            RLDrawText("R to Reset Observer Target", 10, 50, 20, DARKGRAY);

        RLEndTextureMode();

        // Build RHS subject view texture
        RLBeginTextureMode(subjectTarget);

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(subjectCamera);

                RLDrawCube((RLVector3){ 0.0f, 0.0f, 0.0f }, 2.0f, 2.0f, 2.0f, GOLD);
                RLDrawCubeWires((RLVector3){ 0.0f, 0.0f, 0.0f }, 2.0f, 2.0f, 2.0f, PINK);
                RLDrawGrid(10, 1.0f);

            RLEndMode3D();

            RLDrawRectangleLines((subjectTarget.texture.width - captureSize)/2, (subjectTarget.texture.height - captureSize)/2, captureSize, captureSize, GREEN);
            RLDrawText("Subject View", 10, subjectTarget.texture.height - 30, 20, BLACK);

        RLEndTextureMode();
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(BLACK);

            // Draw observer texture LHS
            RLDrawTexturePro(observerTarget.texture, observerSource, observerDest, (RLVector2){0.0f, 0.0f }, 0.0f, WHITE);

            // Draw subject texture RHS
            RLDrawTexturePro(subjectTarget.texture, subjectSource, subjectDest, (RLVector2){ 0.0f, 0.0f }, 0.0f, WHITE);

            // Draw the small crop overlay on top
            RLDrawTexturePro(subjectTarget.texture, cropSource, cropDest, (RLVector2){ 0.0f, 0.0f }, 0.0f, WHITE);
            RLDrawRectangleLinesEx(cropDest, 2, BLACK);

            // Draw split screen divider line
            RLDrawLine(splitWidth, 0, splitWidth, screenHeight, BLACK);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadRenderTexture(observerTarget);
    RLUnloadRenderTexture(subjectTarget);

    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
static void DrawCameraPrism(RLCamera3D camera, float aspect, RLColor color)
{
    float length = Vector3Distance(camera.position, camera.target);
    // Define the 4 corners of the camera's prism plane sliced at the target in Normalized Device Coordinates
    RLVector3 planeNDC[4] = {
        { -1.0f, -1.0f, 1.0f }, // Bottom Left
        {  1.0f, -1.0f, 1.0f }, // Bottom Right
        {  1.0f,  1.0f, 1.0f }, // Top Right
        { -1.0f,  1.0f, 1.0f }  // Top Left
    };

    // Build the matrices
    RLMatrix view = RLGetCameraMatrix(camera);
    RLMatrix proj = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.05f, length);
    // Combine view and projection so we can reverse the full camera transform
    RLMatrix viewProj = MatrixMultiply(view, proj);
    // Invert the view-projection matrix to unproject points from NDC space back into world space
    RLMatrix inverseViewProj = MatrixInvert(viewProj);

    // Transform the 4 plane corners from NDC into world space
    RLVector3 corners[4];
    for (int i = 0; i < 4; i++)
    {
        float x = planeNDC[i].x;
        float y = planeNDC[i].y;
        float z = planeNDC[i].z;

        // Multiply NDC position by the inverse view-projection matrix
        // This produces a homogeneous (x, y, z, w) position in world space
        float vx = inverseViewProj.m0*x + inverseViewProj.m4*y + inverseViewProj.m8*z + inverseViewProj.m12;
        float vy = inverseViewProj.m1*x + inverseViewProj.m5*y + inverseViewProj.m9*z + inverseViewProj.m13;
        float vz = inverseViewProj.m2*x + inverseViewProj.m6*y + inverseViewProj.m10*z + inverseViewProj.m14;
        float vw = inverseViewProj.m3*x + inverseViewProj.m7*y + inverseViewProj.m11*z + inverseViewProj.m15;

        corners[i] = (RLVector3){ vx/vw, vy/vw, vz/vw };
    }

    // Draw the far plane sliced at the target
    RLDrawLine3D(corners[0], corners[1], color);
    RLDrawLine3D(corners[1], corners[2], color);
    RLDrawLine3D(corners[2], corners[3], color);
    RLDrawLine3D(corners[3], corners[0], color);

    // Draw the prism lines from the far plane to the camera position
    for (int i = 0; i < 4; i++)
    {
        RLDrawLine3D(camera.position, corners[i], color);
    }
}