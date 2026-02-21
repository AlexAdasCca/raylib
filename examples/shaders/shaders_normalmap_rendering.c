/*******************************************************************************************
*
*   raylib [shaders] example - normalmap rendering
*
*   Example complexity rating: [★★★★] 4/4
*
*   NOTE: This example requires raylib OpenGL 3.3 or ES2 versions for shaders support,
*        OpenGL 1.1 does not support shaders, recompile raylib to OpenGL 3.3 version
*
*   Example originally created with raylib 5.6, last time updated with raylib 5.6
*
*   Example contributed by Jeremy Montgomery (@Sir_Irk) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Jeremy Montgomery (@Sir_Irk) and Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#include "raymath.h"

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

    RLSetConfigFlags(RL_E_FLAG_MSAA_4X_HINT);
    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - normalmap rendering");

    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 0.0f, 2.0f, -4.0f };
    camera.target = (RLVector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = RL_E_CAMERA_PERSPECTIVE;

    // Load basic normal map lighting shader
    RLShader shader = RLLoadShader(RLTextFormat("resources/shaders/glsl%i/normalmap.vs", GLSL_VERSION),
                               RLTextFormat("resources/shaders/glsl%i/normalmap.fs", GLSL_VERSION));

    // Get some required shader locations
    shader.locs[RL_E_SHADER_LOC_MAP_NORMAL] = RLGetShaderLocation(shader, "normalMap");
    shader.locs[RL_E_SHADER_LOC_VECTOR_VIEW] = RLGetShaderLocation(shader, "viewPos");

    // NOTE: "matModel" location name is automatically assigned on shader loading,
    // no need to get the location again if using that uniform name
    // shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader, "matModel");

    // This example uses just 1 point light
    RLVector3 lightPosition = { 0.0f, 1.0f, 0.0f };
    int lightPosLoc = RLGetShaderLocation(shader, "lightPos");

    // Load a plane model that has proper normals and tangents
    RLModel plane = RLLoadModel("resources/models/plane.glb");

    // Set the plane model's shader and texture maps
    plane.materials[0].shader = shader;
    plane.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = RLLoadTexture("resources/tiles_diffuse.png");
    plane.materials[0].maps[RL_E_MATERIAL_MAP_NORMAL].texture = RLLoadTexture("resources/tiles_normal.png");

    // Generate Mipmaps and use TRILINEAR filtering to help with texture aliasing
    RLGenTextureMipmaps(&plane.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
    RLGenTextureMipmaps(&plane.materials[0].maps[RL_E_MATERIAL_MAP_NORMAL].texture);

    RLSetTextureFilter(plane.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture, RL_E_TEXTURE_FILTER_TRILINEAR);
    RLSetTextureFilter(plane.materials[0].maps[RL_E_MATERIAL_MAP_NORMAL].texture, RL_E_TEXTURE_FILTER_TRILINEAR);

    // Specular exponent AKA shininess of the material
    float specularExponent = 8.0f;
    int specularExponentLoc = RLGetShaderLocation(shader, "specularExponent");

    // Allow toggling the normal map on and off for comparison purposes
    int useNormalMap = 1;
    int useNormalMapLoc = RLGetShaderLocation(shader, "useNormalMap");

    RLSetTargetFPS(60); // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose()) // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // Move the light around on the X and Z axis using WASD keys
        RLVector3 direction = { 0 };
        if (RLIsKeyDown(RL_E_KEY_W)) direction = RLVector3Add(direction, (RLVector3){ 0.0f, 0.0f, 1.0f });
        if (RLIsKeyDown(RL_E_KEY_S)) direction = RLVector3Add(direction, (RLVector3){ 0.0f, 0.0f, -1.0f });
        if (RLIsKeyDown(RL_E_KEY_D)) direction = RLVector3Add(direction, (RLVector3){ -1.0f, 0.0f, 0.0f });
        if (RLIsKeyDown(RL_E_KEY_A)) direction = RLVector3Add(direction, (RLVector3){ 1.0f, 0.0f, 0.0f });

        direction = RLVector3Normalize(direction);
        lightPosition = RLVector3Add(lightPosition, RLVector3Scale(direction, RLGetFrameTime()*3.0f));

        // Increase/Decrease the specular exponent(shininess)
        if (RLIsKeyDown(RL_E_KEY_UP)) specularExponent = RLClamp(specularExponent + 40.0f*RLGetFrameTime(), 2.0f, 128.0f);
        if (RLIsKeyDown(RL_E_KEY_DOWN)) specularExponent = RLClamp(specularExponent - 40.0f*RLGetFrameTime(), 2.0f, 128.0f);

        // Toggle normal map on and off
        if (RLIsKeyPressed(RL_E_KEY_N)) useNormalMap = !useNormalMap;

        // Spin plane model at a constant rate
        plane.transform = RLMatrixRotateY((float)RLGetTime()*0.5f);

        // Update shader values
        float lightPos[3] = {lightPosition.x, lightPosition.y, lightPosition.z};
        RLSetShaderValue(shader, lightPosLoc, lightPos, RL_E_SHADER_UNIFORM_VEC3);

        float camPos[3] = {camera.position.x, camera.position.y, camera.position.z};
        RLSetShaderValue(shader, shader.locs[RL_E_SHADER_LOC_VECTOR_VIEW], camPos, RL_E_SHADER_UNIFORM_VEC3);

        RLSetShaderValue(shader, specularExponentLoc, &specularExponent, RL_E_SHADER_UNIFORM_FLOAT);

        RLSetShaderValue(shader, useNormalMapLoc, &useNormalMap, RL_E_SHADER_UNIFORM_INT);
        //--------------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);

                RLBeginShaderMode(shader);

                    RLDrawModel(plane, RLVector3Zero(), 2.0f, WHITE);

                RLEndShaderMode();

                // Draw sphere to show light position
                RLDrawSphereWires(lightPosition, 0.2f, 8, 8, ORANGE);

            RLEndMode3D();

            RLColor textColor = (useNormalMap) ? DARKGREEN : RED;
            const char *toggleStr = (useNormalMap) ? "On" : "Off";
            RLDrawText(RLTextFormat("Use key [N] to toggle normal map: %s", toggleStr), 10, 10, 10, textColor);

            int yOffset = 24;
            RLDrawText("Use keys [W][A][S][D] to move the light", 10, 10 + yOffset*1, 10, BLACK);
            RLDrawText("Use keys [Up][Down] to change specular exponent", 10, 10 + yOffset*2, 10, BLACK);
            RLDrawText(RLTextFormat("Specular Exponent: %.2f", specularExponent), 10, 10 + yOffset*3, 10, BLUE);

            RLDrawFPS(screenWidth - 90, 10);

        RLEndDrawing();
        //--------------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadShader(shader);
    RLUnloadModel(plane);

    RLCloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
