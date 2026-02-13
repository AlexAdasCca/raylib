/*******************************************************************************************
*
*   raylib [models] example - point rendering
*
*   Example complexity rating: [★★★☆] 3/4
*
*   Example originally created with raylib 5.0, last time updated with raylib 5.0
*
*   Example contributed by Reese Gallagher (@satchelfrost) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2024-2025 Reese Gallagher (@satchelfrost)
*
********************************************************************************************/

#include "raylib.h"

#include <stdlib.h>             // Required for: rand()
#include <math.h>               // Required for: cosf(), sinf()

#define MAX_POINTS 10000000     // 10 million
#define MIN_POINTS 1000         // 1 thousand

//------------------------------------------------------------------------------------
// Module Functions Declaration
//------------------------------------------------------------------------------------
// Generate mesh using points
static RLMesh GenMeshPoints(int numPoints);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [models] example - point rendering");

    RLCamera camera = {
        .position   = { 3.0f, 3.0f, 3.0f },
        .target     = { 0.0f, 0.0f, 0.0f },
        .up         = { 0.0f, 1.0f, 0.0f },
        .fovy       = 45.0f,
        .projection = CAMERA_PERSPECTIVE
    };

    RLVector3 position = { 0.0f, 0.0f, 0.0f };
    bool useDrawModelPoints = true;
    bool numPointsChanged = false;
    int numPoints = 1000;

    RLMesh mesh = GenMeshPoints(numPoints);
    RLModel model = RLLoadModelFromMesh(mesh);

    RLSetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, CAMERA_ORBITAL);

        if (RLIsKeyPressed(KEY_SPACE)) useDrawModelPoints = !useDrawModelPoints;
        if (RLIsKeyPressed(KEY_UP))
        {
            numPoints = (numPoints*10 > MAX_POINTS)? MAX_POINTS : numPoints*10;
            numPointsChanged = true;
        }
        if (RLIsKeyPressed(KEY_DOWN))
        {
            numPoints = (numPoints/10 < MIN_POINTS)? MIN_POINTS : numPoints/10;
            numPointsChanged = true;
        }

        // Upload a different point cloud size
        if (numPointsChanged)
        {
            RLUnloadModel(model);
            mesh = GenMeshPoints(numPoints);
            model = RLLoadModelFromMesh(mesh);
            numPointsChanged = false;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(BLACK);

            RLBeginMode3D(camera);
                // The new method only uploads the points once to the GPU
                if (useDrawModelPoints) RLDrawModelPoints(model, position, 1.0f, WHITE);
                else
                {
                    // The old method must continually draw the "points" (lines)
                    for (int i = 0; i < numPoints; i++)
                    {
                        RLVector3 pos = {
                            .x = mesh.vertices[i*3 + 0],
                            .y = mesh.vertices[i*3 + 1],
                            .z = mesh.vertices[i*3 + 2],
                        };
                        RLColor color = {
                            .r = mesh.colors[i*4 + 0],
                            .g = mesh.colors[i*4 + 1],
                            .b = mesh.colors[i*4 + 2],
                            .a = mesh.colors[i*4 + 3],
                        };

                        RLDrawPoint3D(pos, color);
                    }
                }

                // Draw a unit sphere for reference
                RLDrawSphereWires(position, 1.0f, 10, 10, YELLOW);
            RLEndMode3D();

            // Draw UI text
            RLDrawText(RLTextFormat("Point Count: %d", numPoints), 10, screenHeight - 50, 40, WHITE);
            RLDrawText("UP - Increase points", 10, 40, 20, WHITE);
            RLDrawText("DOWN - Decrease points", 10, 70, 20, WHITE);
            RLDrawText("SPACE - Drawing function", 10, 100, 20, WHITE);

            if (useDrawModelPoints) RLDrawText("Using: DrawModelPoints()", 10, 130, 20, GREEN);
            else RLDrawText("Using: DrawPoint3D()", 10, 130, 20, RED);

            RLDrawFPS(10, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadModel(model);

    RLCloseWindow();
    //--------------------------------------------------------------------------------------
    return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definition
//------------------------------------------------------------------------------------
// Generate a spherical point cloud
static RLMesh GenMeshPoints(int numPoints)
{
    RLMesh mesh = {
        .triangleCount = 1,
        .vertexCount = numPoints,
        .vertices = (float *)RLMemAlloc(numPoints*3*sizeof(float)),
        .colors = (unsigned char*)RLMemAlloc(numPoints*4*sizeof(unsigned char)),
    };

    // REF: https://en.wikipedia.org/wiki/Spherical_coordinate_system
    for (int i = 0; i < numPoints; i++)
    {
        float theta = ((float)PI*rand())/((float)RAND_MAX);
        float phi = (2.0f*PI*rand())/((float)RAND_MAX);
        float r = (10.0f*rand())/((float)RAND_MAX);

        mesh.vertices[i*3 + 0] = r*sinf(theta)*cosf(phi);
        mesh.vertices[i*3 + 1] = r*sinf(theta)*sinf(phi);
        mesh.vertices[i*3 + 2] = r*cosf(theta);

        RLColor color = RLColorFromHSV(r*360.0f, 1.0f, 1.0f);

        mesh.colors[i*4 + 0] = color.r;
        mesh.colors[i*4 + 1] = color.g;
        mesh.colors[i*4 + 2] = color.b;
        mesh.colors[i*4 + 3] = color.a;
    }

    // Upload mesh data from CPU (RAM) to GPU (VRAM) memory
    RLUploadMesh(&mesh, false);

    return mesh;
}
