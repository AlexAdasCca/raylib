/*******************************************************************************************
*
*   raylib [models] example - billboard rendering
*
*   Example complexity rating: [★★★☆] 3/4
*
*   Example originally created with raylib 1.3, last time updated with raylib 3.5
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2015-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [models] example - billboard rendering");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 5.0f, 4.0f, 5.0f };    // Camera position
    camera.target = (RLVector3){ 0.0f, 2.0f, 0.0f };      // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = RL_E_CAMERA_PERSPECTIVE;             // Camera projection type

    RLTexture2D bill = RLLoadTexture("resources/billboard.png");    // Our billboard texture
    RLVector3 billPositionStatic = { 0.0f, 2.0f, 0.0f };          // Position of static billboard
    RLVector3 billPositionRotating = { 1.0f, 2.0f, 1.0f };        // Position of rotating billboard

    // Entire billboard texture, source is used to take a segment from a larger texture
    RLRectangle source = { 0.0f, 0.0f, (float)bill.width, (float)bill.height };

    // NOTE: Billboard locked on axis-Y
    RLVector3 billUp = { 0.0f, 1.0f, 0.0f };

    // Set the height of the rotating billboard to 1.0 with the aspect ratio fixed
    RLVector2 size = { source.width/source.height, 1.0f };

    // Rotate around origin
    // Here we choose to rotate around the image center
    RLVector2 origin = RLVector2Scale(size, 0.5f);

    // Distance is needed for the correct billboard draw order
    // Larger distance (further away from the camera) should be drawn prior to smaller distance
    float distanceStatic;
    float distanceRotating;
    float rotation = 0.0f;

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, RL_E_CAMERA_ORBITAL);

        rotation += 0.4f;
        distanceStatic = RLVector3Distance(camera.position, billPositionStatic);
        distanceRotating = RLVector3Distance(camera.position, billPositionRotating);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);

                RLDrawGrid(10, 1.0f);        // Draw a grid

                // Draw order matters!
                if (distanceStatic > distanceRotating)
                {
                    RLDrawBillboard(camera, bill, billPositionStatic, 2.0f, WHITE);
                    RLDrawBillboardPro(camera, bill, source, billPositionRotating, billUp, size, origin, rotation, WHITE);
                }
                else
                {
                    RLDrawBillboardPro(camera, bill, source, billPositionRotating, billUp, size, origin, rotation, WHITE);
                    RLDrawBillboard(camera, bill, billPositionStatic, 2.0f, WHITE);
                }

            RLEndMode3D();

            RLDrawFPS(10, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(bill);        // Unload texture

    RLCloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
