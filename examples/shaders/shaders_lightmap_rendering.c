/*******************************************************************************************
*
*   raylib [shaders] example - lightmap rendering
*
*   Example complexity rating: [★★★☆] 3/4
*
*   NOTE: This example requires raylib OpenGL 3.3 or ES2 versions for shaders support,
*         OpenGL 1.1 does not support shaders, recompile raylib to OpenGL 3.3 version
*
*   NOTE: Shaders used in this example are #version 330 (OpenGL 3.3)
*
*   Example originally created with raylib 4.5, last time updated with raylib 4.5
*
*   Example contributed by Jussi Viitala (@nullstare) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2019-2025 Jussi Viitala (@nullstare) and Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

#define MAP_SIZE 16

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLSetConfigFlags(FLAG_MSAA_4X_HINT);  // Enable Multi Sampling Anti Aliasing 4x (if available)
    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - lightmap rendering");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 4.0f, 6.0f, 8.0f };    // Camera position
    camera.target = (RLVector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    RLMesh mesh = RLGenMeshPlane((float)MAP_SIZE, (float)MAP_SIZE, 1, 1);

    // GenMeshPlane doesn't generate texcoords2 so we will upload them separately
    mesh.texcoords2 = (float *)RL_MALLOC(mesh.vertexCount*2*sizeof(float));

    // X                          // Y
    mesh.texcoords2[0] = 0.0f;    mesh.texcoords2[1] = 0.0f;
    mesh.texcoords2[2] = 1.0f;    mesh.texcoords2[3] = 0.0f;
    mesh.texcoords2[4] = 0.0f;    mesh.texcoords2[5] = 1.0f;
    mesh.texcoords2[6] = 1.0f;    mesh.texcoords2[7] = 1.0f;

    // Load a new texcoords2 attributes buffer
    mesh.vboId[SHADER_LOC_VERTEX_TEXCOORD02] = rlLoadVertexBuffer(mesh.texcoords2, mesh.vertexCount*2*sizeof(float), false);
    rlEnableVertexArray(mesh.vaoId);

    // Index 5 is for texcoords2
    rlSetVertexAttribute(5, 2, RL_FLOAT, 0, 0, 0);
    rlEnableVertexAttribute(5);
    rlDisableVertexArray();

    // Load lightmap shader
    RLShader shader = RLLoadShader(RLTextFormat("resources/shaders/glsl%i/lightmap.vs", GLSL_VERSION),
                               RLTextFormat("resources/shaders/glsl%i/lightmap.fs", GLSL_VERSION));

    RLTexture texture = RLLoadTexture("resources/cubicmap_atlas.png");
    RLTexture light = RLLoadTexture("resources/spark_flame.png");

    RLGenTextureMipmaps(&texture);
    RLSetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);

    RLRenderTexture lightmap = RLLoadRenderTexture(MAP_SIZE, MAP_SIZE);

    RLMaterial material = RLLoadMaterialDefault();
    material.shader = shader;
    material.maps[MATERIAL_MAP_ALBEDO].texture = texture;
    material.maps[MATERIAL_MAP_METALNESS].texture = lightmap.texture;

    // Drawing to lightmap
    RLBeginTextureMode(lightmap);
        RLClearBackground(BLACK);

        RLBeginBlendMode(BLEND_ADDITIVE);
            RLDrawTexturePro(
                light,
                (RLRectangle){ 0, 0, (float)light.width, (float)light.height },
                (RLRectangle){ 0, 0, 2.0f*MAP_SIZE, 2.0f*MAP_SIZE },
                (RLVector2){ (float)MAP_SIZE, (float)MAP_SIZE },
                0.0,
                RED
            );
            RLDrawTexturePro(
                light,
                (RLRectangle){ 0, 0, (float)light.width, (float)light.height },
                (RLRectangle){ (float)MAP_SIZE*0.8f, (float)MAP_SIZE/2.0f, 2.0f*MAP_SIZE, 2.0f*MAP_SIZE },
                (RLVector2){ (float)MAP_SIZE, (float)MAP_SIZE },
                0.0,
                BLUE
            );
            RLDrawTexturePro(
                light,
                (RLRectangle){ 0, 0, (float)light.width, (float)light.height },
                (RLRectangle){ (float)MAP_SIZE*0.8f, (float)MAP_SIZE*0.8f, (float)MAP_SIZE, (float)MAP_SIZE },
                (RLVector2){ (float)MAP_SIZE/2.0f, (float)MAP_SIZE/2.0f },
                0.0,
                GREEN
            );
        RLBeginBlendMode(BLEND_ALPHA);
    RLEndTextureMode();

    // NOTE: To enable trilinear filtering we need mipmaps available for texture
    RLGenTextureMipmaps(&lightmap.texture);
    RLSetTextureFilter(lightmap.texture, TEXTURE_FILTER_TRILINEAR);

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, CAMERA_ORBITAL);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);
                RLDrawMesh(mesh, material, MatrixIdentity());
            RLEndMode3D();

            RLDrawTexturePro(lightmap.texture, (RLRectangle){ 0, 0, -MAP_SIZE, -MAP_SIZE },
                (RLRectangle){ (float)RLGetRenderWidth() - MAP_SIZE*8 - 10, 10, (float)MAP_SIZE*8, (float)MAP_SIZE*8 },
                (RLVector2){ 0.0, 0.0 }, 0.0, WHITE);

            RLDrawText(RLTextFormat("LIGHTMAP: %ix%i pixels", MAP_SIZE, MAP_SIZE), RLGetRenderWidth() - 130, 20 + MAP_SIZE*8, 10, GREEN);

            RLDrawFPS(10, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadMesh(mesh);       // Unload the mesh
    RLUnloadShader(shader);   // Unload shader
    RLUnloadTexture(texture); // Unload texture
    RLUnloadTexture(light);   // Unload texture

    RLCloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
