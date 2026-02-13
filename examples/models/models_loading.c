/*******************************************************************************************
*
*   raylib [models] example - loading
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   NOTE: raylib supports multiple models file formats:
*
*     - OBJ  > Text file format. Must include vertex position-texcoords-normals information,
*              if files references some .mtl materials file, it will be loaded (or try to)
*     - GLTF > Text/binary file format. Includes lot of information and it could
*              also reference external files, raylib will try loading mesh and materials data
*     - IQM  > Binary file format. Includes mesh vertex data but also animation data,
*              raylib can load .iqm animations
*     - VOX  > Binary file format. MagikaVoxel mesh format:
*              https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt
*     - M3D  > Binary file format. Model 3D format:
*              https://bztsrc.gitlab.io/model3d
*
*   Example originally created with raylib 2.0, last time updated with raylib 4.2
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2014-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [models] example - loading");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 50.0f, 50.0f, 50.0f }; // Camera position
    camera.target = (RLVector3){ 0.0f, 10.0f, 0.0f };     // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;                   // Camera mode type

    RLModel model = RLLoadModel("resources/models/obj/castle.obj");                 // Load model
    RLTexture2D texture = RLLoadTexture("resources/models/obj/castle_diffuse.png"); // Load model texture
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;            // Set map diffuse texture

    RLVector3 position = { 0.0f, 0.0f, 0.0f };                    // Set model position

    RLBoundingBox bounds = RLGetMeshBoundingBox(model.meshes[0]);   // Set model bounds

    // NOTE: bounds are calculated from the original size of the model,
    // if model is scaled on drawing, bounds must be also scaled

    bool selected = false;          // Selected object flag

    RLDisableCursor();                // Limit cursor to relative movement inside the window

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, CAMERA_FIRST_PERSON);

        // Load new models/textures on drag&drop
        if (RLIsFileDropped())
        {
            RLFilePathList droppedFiles = RLLoadDroppedFiles();

            if (droppedFiles.count == 1) // Only support one file dropped
            {
                if (RLIsFileExtension(droppedFiles.paths[0], ".obj") ||
                    RLIsFileExtension(droppedFiles.paths[0], ".gltf") ||
                    RLIsFileExtension(droppedFiles.paths[0], ".glb") ||
                    RLIsFileExtension(droppedFiles.paths[0], ".vox") ||
                    RLIsFileExtension(droppedFiles.paths[0], ".iqm") ||
                    RLIsFileExtension(droppedFiles.paths[0], ".m3d"))       // Model file formats supported
                {
                    RLUnloadModel(model);                         // Unload previous model
                    model = RLLoadModel(droppedFiles.paths[0]);   // Load new model
                    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture; // Set current map diffuse texture

                    bounds = RLGetMeshBoundingBox(model.meshes[0]);

                    // TODO: Move camera position from target enough distance to visualize model properly
                }
                else if (RLIsFileExtension(droppedFiles.paths[0], ".png"))  // Texture file formats supported
                {
                    // Unload current model texture and load new one
                    RLUnloadTexture(texture);
                    texture = RLLoadTexture(droppedFiles.paths[0]);
                    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
                }
            }

            RLUnloadDroppedFiles(droppedFiles);    // Unload filepaths from memory
        }

        // Select model on mouse click
        if (RLIsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            // Check collision between ray and box
            if (RLGetRayCollisionBox(RLGetScreenToWorldRay(RLGetMousePosition(), camera), bounds).hit) selected = !selected;
            else selected = false;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);

                RLDrawModel(model, position, 1.0f, WHITE);        // Draw 3d model with texture

                RLDrawGrid(20, 10.0f);         // Draw a grid

                if (selected) RLDrawBoundingBox(bounds, GREEN);   // Draw selection box

            RLEndMode3D();

            RLDrawText("Drag & drop model to load mesh/texture.", 10, RLGetScreenHeight() - 20, 10, DARKGRAY);
            if (selected) RLDrawText("MODEL SELECTED", RLGetScreenWidth() - 110, 10, 10, GREEN);

            RLDrawText("(c) Castle 3D model by Alberto Cano", screenWidth - 200, screenHeight - 20, 10, GRAY);

            RLDrawFPS(10, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadTexture(texture);     // Unload texture
    RLUnloadModel(model);         // Unload model

    RLCloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}