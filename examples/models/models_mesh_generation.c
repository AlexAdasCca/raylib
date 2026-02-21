/*******************************************************************************************
*
*   raylib [models] example - mesh generation
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 1.8, last time updated with raylib 4.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2017-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#define NUM_MODELS  9               // Parametric 3d shapes to generate

//------------------------------------------------------------------------------------
// Module Functions Declaration
//------------------------------------------------------------------------------------
static RLMesh GenMeshCustom(void);    // Generate a simple triangle mesh from code

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [models] example - mesh generation");

    // We generate a checked image for texturing
    RLImage checked = RLGenImageChecked(2, 2, 1, 1, RED, GREEN);
    RLTexture2D texture = RLLoadTextureFromImage(checked);
    RLUnloadImage(checked);

    RLModel models[NUM_MODELS] = { 0 };

    models[0] = RLLoadModelFromMesh(RLGenMeshPlane(2, 2, 4, 3));
    models[1] = RLLoadModelFromMesh(RLGenMeshCube(2.0f, 1.0f, 2.0f));
    models[2] = RLLoadModelFromMesh(RLGenMeshSphere(2, 32, 32));
    models[3] = RLLoadModelFromMesh(RLGenMeshHemiSphere(2, 16, 16));
    models[4] = RLLoadModelFromMesh(RLGenMeshCylinder(1, 2, 16));
    models[5] = RLLoadModelFromMesh(RLGenMeshTorus(0.25f, 4.0f, 16, 32));
    models[6] = RLLoadModelFromMesh(RLGenMeshKnot(1.0f, 2.0f, 16, 128));
    models[7] = RLLoadModelFromMesh(RLGenMeshPoly(5, 2.0f));
    models[8] = RLLoadModelFromMesh(GenMeshCustom());

    // NOTE: Generated meshes could be exported using ExportMesh()

    // Set checked texture as default diffuse component for all models material
    for (int i = 0; i < NUM_MODELS; i++) models[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

    // Define the camera to look into our 3d world
    RLCamera camera = { { 5.0f, 5.0f, 5.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };

    // Model drawing position
    RLVector3 position = { 0.0f, 0.0f, 0.0f };

    int currentModel = 0;

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, RL_E_CAMERA_ORBITAL);

        if (RLIsMouseButtonPressed(RL_E_MOUSE_BUTTON_LEFT))
        {
            currentModel = (currentModel + 1)%NUM_MODELS; // Cycle between the textures
        }

        if (RLIsKeyPressed(RL_E_KEY_RIGHT))
        {
            currentModel++;
            if (currentModel >= NUM_MODELS) currentModel = 0;
        }
        else if (RLIsKeyPressed(RL_E_KEY_LEFT))
        {
            currentModel--;
            if (currentModel < 0) currentModel = NUM_MODELS - 1;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);

               RLDrawModel(models[currentModel], position, 1.0f, WHITE);
               RLDrawGrid(10, 1.0);

            RLEndMode3D();

            RLDrawRectangle(30, 400, 310, 30, RLFade(SKYBLUE, 0.5f));
            RLDrawRectangleLines(30, 400, 310, 30, RLFade(DARKBLUE, 0.5f));
            RLDrawText("MOUSE LEFT BUTTON to CYCLE PROCEDURAL MODELS", 40, 410, 10, BLUE);

            switch (currentModel)
            {
                case 0: RLDrawText("PLANE", 680, 10, 20, DARKBLUE); break;
                case 1: RLDrawText("CUBE", 680, 10, 20, DARKBLUE); break;
                case 2: RLDrawText("SPHERE", 680, 10, 20, DARKBLUE); break;
                case 3: RLDrawText("HEMISPHERE", 640, 10, 20, DARKBLUE); break;
                case 4: RLDrawText("CYLINDER", 680, 10, 20, DARKBLUE); break;
                case 5: RLDrawText("TORUS", 680, 10, 20, DARKBLUE); break;
                case 6: RLDrawText("KNOT", 680, 10, 20, DARKBLUE); break;
                case 7: RLDrawText("POLY", 680, 10, 20, DARKBLUE); break;
                case 8: RLDrawText("Custom (triangle)", 580, 10, 20, DARKBLUE); break;
                default: break;
            }

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(texture); // Unload texture

    // Unload models data (GPU VRAM)
    for (int i = 0; i < NUM_MODELS; i++) RLUnloadModel(models[i]);

    RLCloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definition
//------------------------------------------------------------------------------------
// Generate a simple triangle mesh from code
static RLMesh GenMeshCustom(void)
{
    RLMesh mesh = { 0 };
    mesh.triangleCount = 1;
    mesh.vertexCount = mesh.triangleCount*3;
    mesh.vertices = (float *)RLMemAlloc(mesh.vertexCount*3*sizeof(float));    // 3 vertices, 3 coordinates each (x, y, z)
    mesh.texcoords = (float *)RLMemAlloc(mesh.vertexCount*2*sizeof(float));   // 3 vertices, 2 coordinates each (x, y)
    mesh.normals = (float *)RLMemAlloc(mesh.vertexCount*3*sizeof(float));     // 3 vertices, 3 coordinates each (x, y, z)

    // Vertex at (0, 0, 0)
    mesh.vertices[0] = 0;
    mesh.vertices[1] = 0;
    mesh.vertices[2] = 0;
    mesh.normals[0] = 0;
    mesh.normals[1] = 1;
    mesh.normals[2] = 0;
    mesh.texcoords[0] = 0;
    mesh.texcoords[1] = 0;

    // Vertex at (1, 0, 2)
    mesh.vertices[3] = 1;
    mesh.vertices[4] = 0;
    mesh.vertices[5] = 2;
    mesh.normals[3] = 0;
    mesh.normals[4] = 1;
    mesh.normals[5] = 0;
    mesh.texcoords[2] = 0.5f;
    mesh.texcoords[3] = 1.0f;

    // Vertex at (2, 0, 0)
    mesh.vertices[6] = 2;
    mesh.vertices[7] = 0;
    mesh.vertices[8] = 0;
    mesh.normals[6] = 0;
    mesh.normals[7] = 1;
    mesh.normals[8] = 0;
    mesh.texcoords[4] = 1;
    mesh.texcoords[5] =0;

    // Upload mesh data from CPU (RAM) to GPU (VRAM) memory
    RLUploadMesh(&mesh, false);

    return mesh;
}
