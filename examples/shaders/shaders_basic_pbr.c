/*******************************************************************************************
*
*   raylib [shaders] example - basic pbr
*
*   Example complexity rating: [★★★★] 4/4
*
*   Example originally created with raylib 5.0, last time updated with raylib 5.5
*
*   Example contributed by Afan OLOVCIC (@_DevDad) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2023-2025 Afan OLOVCIC (@_DevDad)
*
*   Model: "Old Rusty Car" (https://skfb.ly/LxRy) by Renafox,
*   licensed under Creative Commons Attribution-NonCommercial
*   (http://creativecommons.org/licenses/by-nc/4.0/)
*
********************************************************************************************/

#include "raylib.h"

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

#include <stdlib.h>             // Required for: NULL

#define MAX_LIGHTS  4           // Max dynamic lights supported by shader

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Light type
typedef enum {
    LIGHT_DIRECTIONAL = 0,
    LIGHT_POINT,
    LIGHT_SPOT
} LightType;

// Light data
typedef struct {
    int type;
    int enabled;
    RLVector3 position;
    RLVector3 target;
    float color[4];
    float intensity;

    // Shader light parameters locations
    int typeLoc;
    int enabledLoc;
    int positionLoc;
    int targetLoc;
    int colorLoc;
    int intensityLoc;
} Light;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static int lightCount = 0;     // Current number of dynamic lights that have been created

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
// Create a light and get shader locations
static Light CreateLight(int type, RLVector3 position, RLVector3 target, RLColor color, float intensity, RLShader shader);

// Update light properties on shader
// NOTE: Light shader locations should be available
static void UpdateLight(RLShader shader, Light light);

//----------------------------------------------------------------------------------
// Program main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLSetConfigFlags(RL_E_FLAG_MSAA_4X_HINT);
    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - basic pbr");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 2.0f, 2.0f, 6.0f };    // Camera position
    camera.target = (RLVector3){ 0.0f, 0.5f, 0.0f };      // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = RL_E_CAMERA_PERSPECTIVE;             // Camera projection type

    // Load PBR shader and setup all required locations
    RLShader shader = RLLoadShader(RLTextFormat("resources/shaders/glsl%i/pbr.vs", GLSL_VERSION),
                               RLTextFormat("resources/shaders/glsl%i/pbr.fs", GLSL_VERSION));
    shader.locs[RL_E_SHADER_LOC_MAP_ALBEDO] = RLGetShaderLocation(shader, "albedoMap");
    // WARNING: Metalness, roughness, and ambient occlusion are all packed into a MRA texture
    // They are passed as to the SHADER_LOC_MAP_METALNESS location for convenience,
    // shader already takes care of it accordingly
    shader.locs[RL_E_SHADER_LOC_MAP_METALNESS] = RLGetShaderLocation(shader, "mraMap");
    shader.locs[RL_E_SHADER_LOC_MAP_NORMAL] = RLGetShaderLocation(shader, "normalMap");
    // WARNING: Similar to the MRA map, the emissive map packs different information
    // into a single texture: it stores height and emission data
    // It is binded to SHADER_LOC_MAP_EMISSION location an properly processed on shader
    shader.locs[RL_E_SHADER_LOC_MAP_EMISSION] = RLGetShaderLocation(shader, "emissiveMap");
    shader.locs[RL_E_SHADER_LOC_COLOR_DIFFUSE] = RLGetShaderLocation(shader, "albedoColor");

    // Setup additional required shader locations, including lights data
    shader.locs[RL_E_SHADER_LOC_VECTOR_VIEW] = RLGetShaderLocation(shader, "viewPos");
    int lightCountLoc = RLGetShaderLocation(shader, "numOfLights");
    int maxLightCount = MAX_LIGHTS;
    RLSetShaderValue(shader, lightCountLoc, &maxLightCount, RL_E_SHADER_UNIFORM_INT);

    // Setup ambient color and intensity parameters
    float ambientIntensity = 0.02f;
    RLColor ambientColor = (RLColor){ 26, 32, 135, 255 };
    RLVector3 ambientColorNormalized = (RLVector3){ ambientColor.r/255.0f, ambientColor.g/255.0f, ambientColor.b/255.0f };
    RLSetShaderValue(shader, RLGetShaderLocation(shader, "ambientColor"), &ambientColorNormalized, RL_E_SHADER_UNIFORM_VEC3);
    RLSetShaderValue(shader, RLGetShaderLocation(shader, "ambient"), &ambientIntensity, RL_E_SHADER_UNIFORM_FLOAT);

    // Get location for shader parameters that can be modified in real time
    int metallicValueLoc = RLGetShaderLocation(shader, "metallicValue");
    int roughnessValueLoc = RLGetShaderLocation(shader, "roughnessValue");
    int emissiveIntensityLoc = RLGetShaderLocation(shader, "emissivePower");
    int emissiveColorLoc = RLGetShaderLocation(shader, "emissiveColor");
    int textureTilingLoc = RLGetShaderLocation(shader, "tiling");

    // Load old car model using PBR maps and shader
    // WARNING: We know this model consists of a single model.meshes[0] and
    // that model.materials[0] is by default assigned to that mesh
    // There could be more complex models consisting of multiple meshes and
    // multiple materials defined for those meshes... but always 1 mesh = 1 material
    RLModel car = RLLoadModel("resources/models/old_car_new.glb");

    // Assign already setup PBR shader to model.materials[0], used by models.meshes[0]
    car.materials[0].shader = shader;

    // Setup materials[0].maps default parameters
    car.materials[0].maps[RL_E_MATERIAL_MAP_ALBEDO].color = WHITE;
    car.materials[0].maps[RL_E_MATERIAL_MAP_METALNESS].value = 1.0f;
    car.materials[0].maps[RL_E_MATERIAL_MAP_ROUGHNESS].value = 0.0f;
    car.materials[0].maps[RL_E_MATERIAL_MAP_OCCLUSION].value = 1.0f;
    car.materials[0].maps[RL_E_MATERIAL_MAP_EMISSION].color = (RLColor){ 255, 162, 0, 255 };

    // Setup materials[0].maps default textures
    car.materials[0].maps[RL_E_MATERIAL_MAP_ALBEDO].texture = RLLoadTexture("resources/old_car_d.png");
    car.materials[0].maps[RL_E_MATERIAL_MAP_METALNESS].texture = RLLoadTexture("resources/old_car_mra.png");
    car.materials[0].maps[RL_E_MATERIAL_MAP_NORMAL].texture = RLLoadTexture("resources/old_car_n.png");
    car.materials[0].maps[RL_E_MATERIAL_MAP_EMISSION].texture = RLLoadTexture("resources/old_car_e.png");

    // Load floor model mesh and assign material parameters
    // NOTE: A basic plane shape can be generated instead of being loaded from a model file
    RLModel floor = RLLoadModel("resources/models/plane.glb");
    //Mesh floorMesh = GenMeshPlane(10, 10, 10, 10);
    //GenMeshTangents(&floorMesh);      // TODO: Review tangents generation
    //Model floor = LoadModelFromMesh(floorMesh);

    // Assign material shader for our floor model, same PBR shader
    floor.materials[0].shader = shader;

    floor.materials[0].maps[RL_E_MATERIAL_MAP_ALBEDO].color = WHITE;
    floor.materials[0].maps[RL_E_MATERIAL_MAP_METALNESS].value = 0.8f;
    floor.materials[0].maps[RL_E_MATERIAL_MAP_ROUGHNESS].value = 0.1f;
    floor.materials[0].maps[RL_E_MATERIAL_MAP_OCCLUSION].value = 1.0f;
    floor.materials[0].maps[RL_E_MATERIAL_MAP_EMISSION].color = BLACK;

    floor.materials[0].maps[RL_E_MATERIAL_MAP_ALBEDO].texture = RLLoadTexture("resources/road_a.png");
    floor.materials[0].maps[RL_E_MATERIAL_MAP_METALNESS].texture = RLLoadTexture("resources/road_mra.png");
    floor.materials[0].maps[RL_E_MATERIAL_MAP_NORMAL].texture = RLLoadTexture("resources/road_n.png");

    // Models texture tiling parameter can be stored in the Material struct if required (CURRENTLY NOT USED)
    // NOTE: Material.params[4] are available for generic parameters storage (float)
    RLVector2 carTextureTiling = (RLVector2){ 0.5f, 0.5f };
    RLVector2 floorTextureTiling = (RLVector2){ 0.5f, 0.5f };

    // Create some lights
    Light lights[MAX_LIGHTS] = { 0 };
    lights[0] = CreateLight(LIGHT_POINT, (RLVector3){ -1.0f, 1.0f, -2.0f }, (RLVector3){ 0.0f, 0.0f, 0.0f }, YELLOW, 4.0f, shader);
    lights[1] = CreateLight(LIGHT_POINT, (RLVector3){ 2.0f, 1.0f, 1.0f }, (RLVector3){ 0.0f, 0.0f, 0.0f }, GREEN, 3.3f, shader);
    lights[2] = CreateLight(LIGHT_POINT, (RLVector3){ -2.0f, 1.0f, 1.0f }, (RLVector3){ 0.0f, 0.0f, 0.0f }, RED, 8.3f, shader);
    lights[3] = CreateLight(LIGHT_POINT, (RLVector3){ 1.0f, 1.0f, -2.0f }, (RLVector3){ 0.0f, 0.0f, 0.0f }, BLUE, 2.0f, shader);

    // Setup material texture maps usage in shader
    // NOTE: By default, the texture maps are always used
    int usage = 1;
    RLSetShaderValue(shader, RLGetShaderLocation(shader, "useTexAlbedo"), &usage, RL_E_SHADER_UNIFORM_INT);
    RLSetShaderValue(shader, RLGetShaderLocation(shader, "useTexNormal"), &usage, RL_E_SHADER_UNIFORM_INT);
    RLSetShaderValue(shader, RLGetShaderLocation(shader, "useTexMRA"), &usage, RL_E_SHADER_UNIFORM_INT);
    RLSetShaderValue(shader, RLGetShaderLocation(shader, "useTexEmissive"), &usage, RL_E_SHADER_UNIFORM_INT);

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        RLUpdateCamera(&camera, RL_E_CAMERA_ORBITAL);

        // Update the shader with the camera view vector (points towards { 0.0f, 0.0f, 0.0f })
        float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
        RLSetShaderValue(shader, shader.locs[RL_E_SHADER_LOC_VECTOR_VIEW], cameraPos, RL_E_SHADER_UNIFORM_VEC3);

        // Check key inputs to enable/disable lights
        if (RLIsKeyPressed(RL_E_KEY_ONE)) { lights[2].enabled = !lights[2].enabled; }
        if (RLIsKeyPressed(RL_E_KEY_TWO)) { lights[1].enabled = !lights[1].enabled; }
        if (RLIsKeyPressed(RL_E_KEY_THREE)) { lights[3].enabled = !lights[3].enabled; }
        if (RLIsKeyPressed(RL_E_KEY_FOUR)) { lights[0].enabled = !lights[0].enabled; }

        // Update light values on shader (actually, only enable/disable them)
        for (int i = 0; i < MAX_LIGHTS; i++) UpdateLight(shader, lights[i]);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(BLACK);

            RLBeginMode3D(camera);

                // Set floor model texture tiling and emissive color parameters on shader
                RLSetShaderValue(shader, textureTilingLoc, &floorTextureTiling, RL_E_SHADER_UNIFORM_VEC2);
                RLVector4 floorEmissiveColor = RLColorNormalize(floor.materials[0].maps[RL_E_MATERIAL_MAP_EMISSION].color);
                RLSetShaderValue(shader, emissiveColorLoc, &floorEmissiveColor, RL_E_SHADER_UNIFORM_VEC4);

                // Set floor metallic and roughness values
                RLSetShaderValue(shader, metallicValueLoc, &floor.materials[0].maps[RL_E_MATERIAL_MAP_METALNESS].value, RL_E_SHADER_UNIFORM_FLOAT);
                RLSetShaderValue(shader, roughnessValueLoc, &floor.materials[0].maps[RL_E_MATERIAL_MAP_ROUGHNESS].value, RL_E_SHADER_UNIFORM_FLOAT);

                RLDrawModel(floor, (RLVector3){ 0.0f, 0.0f, 0.0f }, 5.0f, WHITE);   // Draw floor model

                // Set old car model texture tiling, emissive color and emissive intensity parameters on shader
                RLSetShaderValue(shader, textureTilingLoc, &carTextureTiling, RL_E_SHADER_UNIFORM_VEC2);
                RLVector4 carEmissiveColor = RLColorNormalize(car.materials[0].maps[RL_E_MATERIAL_MAP_EMISSION].color);
                RLSetShaderValue(shader, emissiveColorLoc, &carEmissiveColor, RL_E_SHADER_UNIFORM_VEC4);
                float emissiveIntensity = 0.01f;
                RLSetShaderValue(shader, emissiveIntensityLoc, &emissiveIntensity, RL_E_SHADER_UNIFORM_FLOAT);

                // Set old car metallic and roughness values
                RLSetShaderValue(shader, metallicValueLoc, &car.materials[0].maps[RL_E_MATERIAL_MAP_METALNESS].value, RL_E_SHADER_UNIFORM_FLOAT);
                RLSetShaderValue(shader, roughnessValueLoc, &car.materials[0].maps[RL_E_MATERIAL_MAP_ROUGHNESS].value, RL_E_SHADER_UNIFORM_FLOAT);

                RLDrawModel(car, (RLVector3){ 0.0f, 0.0f, 0.0f }, 0.25f, WHITE);   // Draw car model

                // Draw spheres to show the lights positions
                for (int i = 0; i < MAX_LIGHTS; i++)
                {
                    RLColor lightColor = (RLColor){
                        (unsigned char)(lights[i].color[0]*255),
                        (unsigned char)(lights[i].color[1]*255),
                        (unsigned char)(lights[i].color[2]*255),
                        (unsigned char)(lights[i].color[3]*255) };

                    if (lights[i].enabled) RLDrawSphereEx(lights[i].position, 0.2f, 8, 8, lightColor);
                    else RLDrawSphereWires(lights[i].position, 0.2f, 8, 8, RLColorAlpha(lightColor, 0.3f));
                }

            RLEndMode3D();

            RLDrawText("Toggle lights: [1][2][3][4]", 10, 40, 20, LIGHTGRAY);

            RLDrawText("(c) Old Rusty Car model by Renafox (https://skfb.ly/LxRy)", screenWidth - 320, screenHeight - 20, 10, LIGHTGRAY);

            RLDrawFPS(10, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unbind (disconnect) shader from car.material[0]
    // to avoid UnloadMaterial() trying to unload it automatically
    car.materials[0].shader = (RLShader){ 0 };
    RLUnloadMaterial(car.materials[0]);
    car.materials[0].maps = NULL;
    RLUnloadModel(car);

    floor.materials[0].shader = (RLShader){ 0 };
    RLUnloadMaterial(floor.materials[0]);
    floor.materials[0].maps = NULL;
    RLUnloadModel(floor);

    RLUnloadShader(shader);       // Unload Shader

    RLCloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
// Create light with provided data
// NOTE: It updated the global lightCount and it's limited to MAX_LIGHTS
static Light CreateLight(int type, RLVector3 position, RLVector3 target, RLColor color, float intensity, RLShader shader)
{
    Light light = { 0 };

    if (lightCount < MAX_LIGHTS)
    {
        light.enabled = 1;
        light.type = type;
        light.position = position;
        light.target = target;
        light.color[0] = (float)color.r/255.0f;
        light.color[1] = (float)color.g/255.0f;
        light.color[2] = (float)color.b/255.0f;
        light.color[3] = (float)color.a/255.0f;
        light.intensity = intensity;

        // NOTE: Shader parameters names for lights must match the requested ones
        light.enabledLoc = RLGetShaderLocation(shader, RLTextFormat("lights[%i].enabled", lightCount));
        light.typeLoc = RLGetShaderLocation(shader, RLTextFormat("lights[%i].type", lightCount));
        light.positionLoc = RLGetShaderLocation(shader, RLTextFormat("lights[%i].position", lightCount));
        light.targetLoc = RLGetShaderLocation(shader, RLTextFormat("lights[%i].target", lightCount));
        light.colorLoc = RLGetShaderLocation(shader, RLTextFormat("lights[%i].color", lightCount));
        light.intensityLoc = RLGetShaderLocation(shader, RLTextFormat("lights[%i].intensity", lightCount));

        UpdateLight(shader, light);

        lightCount++;
    }

    return light;
}

// Send light properties to shader
// NOTE: Light shader locations should be available
static void UpdateLight(RLShader shader, Light light)
{
    RLSetShaderValue(shader, light.enabledLoc, &light.enabled, RL_E_SHADER_UNIFORM_INT);
    RLSetShaderValue(shader, light.typeLoc, &light.type, RL_E_SHADER_UNIFORM_INT);

    // Send to shader light position values
    float position[3] = { light.position.x, light.position.y, light.position.z };
    RLSetShaderValue(shader, light.positionLoc, position, RL_E_SHADER_UNIFORM_VEC3);

    // Send to shader light target position values
    float target[3] = { light.target.x, light.target.y, light.target.z };
    RLSetShaderValue(shader, light.targetLoc, target, RL_E_SHADER_UNIFORM_VEC3);
    RLSetShaderValue(shader, light.colorLoc, light.color, RL_E_SHADER_UNIFORM_VEC4);
    RLSetShaderValue(shader, light.intensityLoc, &light.intensity, RL_E_SHADER_UNIFORM_FLOAT);
}
