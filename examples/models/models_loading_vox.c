/*******************************************************************************************
*
*   raylib [models] example - loading vox
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 4.0, last time updated with raylib 4.0
*
*   Example contributed by Johann Nadalutti (@procfxgen) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2021-2025 Johann Nadalutti (@procfxgen) and Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#include "raymath.h"        // Required for: MatrixTranslate()

#define MAX_VOX_FILES  4

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

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

    const char *voxFileNames[] = {
        "resources/models/vox/chr_knight.vox",
        "resources/models/vox/chr_sword.vox",
        "resources/models/vox/monu9.vox",
        "resources/models/vox/fez.vox"
    };

    RLInitWindow(screenWidth, screenHeight, "raylib [models] example - loading vox");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 10.0f, 10.0f, 10.0f }; // Camera position
    camera.target = (RLVector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    // Load MagicaVoxel files
    RLModel models[MAX_VOX_FILES] = { 0 };

    for (int i = 0; i < MAX_VOX_FILES; i++)
    {
        // Load VOX file and measure time
        double t0 = RLGetTime()*1000.0;
        models[i] = RLLoadModel(voxFileNames[i]);
        double t1 = RLGetTime()*1000.0;

        RLTraceLog(LOG_INFO, RLTextFormat("[%s] Model file loaded in %.3f ms", voxFileNames[i], t1 - t0));

        // Compute model translation matrix to center model on draw position (0, 0 , 0)
        RLBoundingBox bb = RLGetModelBoundingBox(models[i]);
        RLVector3 center = { 0 };
        center.x = bb.min.x + (((bb.max.x - bb.min.x)/2));
        center.z = bb.min.z + (((bb.max.z - bb.min.z)/2));

        RLMatrix matTranslate = MatrixTranslate(-center.x, 0, -center.z);
        models[i].transform = matTranslate;
    }

    int currentModel = 0;
    RLVector3 modelpos = { 0 };
    RLVector3 camerarot = { 0 };

    // Load voxel shader
    RLShader shader = RLLoadShader(RLTextFormat("resources/shaders/glsl%i/voxel_lighting.vs", GLSL_VERSION),
        RLTextFormat("resources/shaders/glsl%i/voxel_lighting.fs", GLSL_VERSION));

    // Get some required shader locations
    shader.locs[SHADER_LOC_VECTOR_VIEW] = RLGetShaderLocation(shader, "viewPos");
    // NOTE: "matModel" location name is automatically assigned on shader loading,
    // no need to get the location again if using that uniform name
    //shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader, "matModel");

    // Ambient light level (some basic lighting)
    int ambientLoc = RLGetShaderLocation(shader, "ambient");
    RLSetShaderValue(shader, ambientLoc, (float[4]) { 0.1f, 0.1f, 0.1f, 1.0f }, SHADER_UNIFORM_VEC4);

    // Assign out lighting shader to model
    for (int i = 0; i < MAX_VOX_FILES; i++)
    {
        for (int j = 0; j < models[i].materialCount; j++) models[i].materials[j].shader = shader;
    }

    // Create lights
    Light lights[MAX_LIGHTS] = { 0 };
    lights[0] = CreateLight(LIGHT_POINT, (RLVector3) { -20, 20, -20 }, Vector3Zero(), GRAY, shader);
    lights[1] = CreateLight(LIGHT_POINT, (RLVector3) { 20, -20, 20 }, Vector3Zero(), GRAY, shader);
    lights[2] = CreateLight(LIGHT_POINT, (RLVector3) { -20, 20, 20 }, Vector3Zero(), GRAY, shader);
    lights[3] = CreateLight(LIGHT_POINT, (RLVector3) { 20, -20, -20 }, Vector3Zero(), GRAY, shader);

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (RLIsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
        {
            const RLVector2 mouseDelta = RLGetMouseDelta();
            camerarot.x = mouseDelta.x*0.05f;
            camerarot.y = mouseDelta.y*0.05f;
        }
        else
        {
            camerarot.x = 0;
            camerarot.y = 0;
        }

        RLUpdateCameraPro(&camera,
            (RLVector3){ (RLIsKeyDown(KEY_W) || RLIsKeyDown(KEY_UP))*0.1f - (RLIsKeyDown(KEY_S) || RLIsKeyDown(KEY_DOWN))*0.1f, // Move forward-backward
                       (RLIsKeyDown(KEY_D) || RLIsKeyDown(KEY_RIGHT))*0.1f - (RLIsKeyDown(KEY_A) || RLIsKeyDown(KEY_LEFT))*0.1f, // Move right-left
                       0.0f }, // Move up-down
            camerarot, // Camera rotation
            RLGetMouseWheelMove()*-2.0f); // Move to target (zoom)

        // Cycle between models on mouse click
        if (RLIsMouseButtonPressed(MOUSE_BUTTON_LEFT)) currentModel = (currentModel + 1)%MAX_VOX_FILES;

        // Update the shader with the camera view vector (points towards { 0.0f, 0.0f, 0.0f })
        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        RLSetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        // Update light values (actually, only enable/disable them)
        for (int i = 0; i < MAX_LIGHTS; i++) UpdateLightValues(shader, lights[i]);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            // Draw 3D model
            RLBeginMode3D(camera);
                RLDrawModel(models[currentModel], modelpos, 1.0f, WHITE);
                RLDrawGrid(10, 1.0);

                // Draw spheres to show where the lights are
                for (int i = 0; i < MAX_LIGHTS; i++)
                {
                    if (lights[i].enabled) RLDrawSphereEx(lights[i].position, 0.2f, 8, 8, lights[i].color);
                    else RLDrawSphereWires(lights[i].position, 0.2f, 8, 8, RLColorAlpha(lights[i].color, 0.3f));
                }
            RLEndMode3D();

            // Display info
            RLDrawRectangle(10, 40, 340, 70, RLFade(SKYBLUE, 0.5f));
            RLDrawRectangleLines(10, 40, 340, 70, RLFade(DARKBLUE, 0.5f));
            RLDrawText("- MOUSE LEFT BUTTON: CYCLE VOX MODELS", 20, 50, 10, BLUE);
            RLDrawText("- MOUSE MIDDLE BUTTON: ZOOM OR ROTATE CAMERA", 20, 70, 10, BLUE);
            RLDrawText("- UP-DOWN-LEFT-RIGHT KEYS: MOVE CAMERA", 20, 90, 10, BLUE);
            RLDrawText(RLTextFormat("Model file: %s", RLGetFileName(voxFileNames[currentModel])), 10, 10, 20, GRAY);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unload models data (GPU VRAM)
    for (int i = 0; i < MAX_VOX_FILES; i++) RLUnloadModel(models[i]);

    RLCloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
