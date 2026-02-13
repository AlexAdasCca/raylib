/*******************************************************************************************
*
*   raylib [shaders] example - shadowmap rendering
*
*   Example complexity rating: [★★★★] 4/4
*
*   Example originally created with raylib 5.0, last time updated with raylib 5.0
*
*   Example contributed by TheManTheMythTheGameDev (@TheManTheMythTheGameDev) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2023-2025 TheManTheMythTheGameDev (@TheManTheMythTheGameDev)
*
********************************************************************************************/

#include "raylib.h"

#include "raymath.h"
#include "rlgl.h"

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

#define SHADOWMAP_RESOLUTION 1024

static RLRenderTexture2D LoadShadowmapRenderTexture(int width, int height);
static void UnloadShadowmapRenderTexture(RLRenderTexture2D target);
static void DrawScene(RLModel cube, RLModel robot);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    // Shadows are a HUGE topic, and this example shows an extremely simple implementation of the shadowmapping algorithm,
    // which is the industry standard for shadows. This algorithm can be extended in a ridiculous number of ways to improve
    // realism and also adapt it for different scenes. This is pretty much the simplest possible implementation

    RLSetConfigFlags(FLAG_MSAA_4X_HINT);
    RLInitWindow(screenWidth, screenHeight, "raylib [shaders] example - shadowmap rendering");

    RLCamera3D camera = (RLCamera3D){ 0 };
    camera.position = (RLVector3){ 10.0f, 10.0f, 10.0f };
    camera.target = Vector3Zero();
    camera.projection = CAMERA_PERSPECTIVE;
    camera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;

    RLShader shadowShader = RLLoadShader(RLTextFormat("resources/shaders/glsl%i/shadowmap.vs", GLSL_VERSION),
                                     RLTextFormat("resources/shaders/glsl%i/shadowmap.fs", GLSL_VERSION));
    shadowShader.locs[SHADER_LOC_VECTOR_VIEW] = RLGetShaderLocation(shadowShader, "viewPos");

    RLVector3 lightDir = Vector3Normalize((RLVector3){ 0.35f, -1.0f, -0.35f });
    RLColor lightColor = WHITE;
    RLVector4 lightColorNormalized = RLColorNormalize(lightColor);
    int lightDirLoc = RLGetShaderLocation(shadowShader, "lightDir");
    int lightColLoc = RLGetShaderLocation(shadowShader, "lightColor");
    RLSetShaderValue(shadowShader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
    RLSetShaderValue(shadowShader, lightColLoc, &lightColorNormalized, SHADER_UNIFORM_VEC4);
    int ambientLoc = RLGetShaderLocation(shadowShader, "ambient");
    float ambient[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    RLSetShaderValue(shadowShader, ambientLoc, ambient, SHADER_UNIFORM_VEC4);
    int lightVPLoc = RLGetShaderLocation(shadowShader, "lightVP");
    int shadowMapLoc = RLGetShaderLocation(shadowShader, "shadowMap");
    int shadowMapResolution = SHADOWMAP_RESOLUTION;
    RLSetShaderValue(shadowShader, RLGetShaderLocation(shadowShader, "shadowMapResolution"), &shadowMapResolution, SHADER_UNIFORM_INT);

    RLModel cube = RLLoadModelFromMesh(RLGenMeshCube(1.0f, 1.0f, 1.0f));
    cube.materials[0].shader = shadowShader;
    RLModel robot = RLLoadModel("resources/models/robot.glb");
    for (int i = 0; i < robot.materialCount; i++) robot.materials[i].shader = shadowShader;
    int animCount = 0;
    RLModelAnimation *robotAnimations = RLLoadModelAnimations("resources/models/robot.glb", &animCount);

    RLRenderTexture2D shadowMap = LoadShadowmapRenderTexture(SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION);

    // For the shadowmapping algorithm, we will be rendering everything from the light's point of view
    RLCamera3D lightCamera = { 0 };
    lightCamera.position = Vector3Scale(lightDir, -15.0f);
    lightCamera.target = Vector3Zero();
    lightCamera.projection = CAMERA_ORTHOGRAPHIC; // Use an orthographic projection for directional lights
    lightCamera.up = (RLVector3){ 0.0f, 1.0f, 0.0f };
    lightCamera.fovy = 20.0f;

    int frameCounter = 0;

    // Store the light matrices
    RLMatrix lightView = { 0 };
    RLMatrix lightProj = { 0 };
    RLMatrix lightViewProj = { 0 };
    int textureActiveSlot = 10; // Can be anything 0 to 15, but 0 will probably be taken up

    RLSetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        float deltaTime = RLGetFrameTime();

        RLVector3 cameraPos = camera.position;
        RLSetShaderValue(shadowShader, shadowShader.locs[SHADER_LOC_VECTOR_VIEW], &cameraPos, SHADER_UNIFORM_VEC3);
        RLUpdateCamera(&camera, CAMERA_ORBITAL);

        frameCounter++;
        frameCounter %= (robotAnimations[0].frameCount);
        RLUpdateModelAnimation(robot, robotAnimations[0], frameCounter);

        // Move light with arrow keys
        const float cameraSpeed = 0.05f;
        if (RLIsKeyDown(KEY_LEFT))
        {
            if (lightDir.x < 0.6f) lightDir.x += cameraSpeed*60.0f*deltaTime;
        }
        if (RLIsKeyDown(KEY_RIGHT))
        {
            if (lightDir.x > -0.6f) lightDir.x -= cameraSpeed*60.0f*deltaTime;
        }
        if (RLIsKeyDown(KEY_UP))
        {
            if (lightDir.z < 0.6f) lightDir.z += cameraSpeed*60.0f*deltaTime;
        }
        if (RLIsKeyDown(KEY_DOWN))
        {
            if (lightDir.z > -0.6f) lightDir.z -= cameraSpeed*60.0f*deltaTime;
        }

        lightDir = Vector3Normalize(lightDir);
        lightCamera.position = Vector3Scale(lightDir, -15.0f);
        RLSetShaderValue(shadowShader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        // PASS 01: Render all objects into the shadowmap render texture
        // We record all the objects' depths (as rendered from the light source's point of view) in a buffer
        // Anything that is "visible" to the light is in light, anything that isn't is in shadow
        // We can later use the depth buffer when rendering everything from the player's point of view
        // to determine whether a given point is "visible" to the light
        RLBeginTextureMode(shadowMap);
            RLClearBackground(WHITE);

            RLBeginMode3D(lightCamera);
                lightView = rlGetMatrixModelview();
                lightProj = rlGetMatrixProjection();
                DrawScene(cube, robot);
            RLEndMode3D();

        RLEndTextureMode();
        lightViewProj = MatrixMultiply(lightView, lightProj);

        // PASS 02: Draw the scene into main framebuffer, using the generated shadowmap
        RLBeginDrawing();
            RLClearBackground(RAYWHITE);

            RLSetShaderValueMatrix(shadowShader, lightVPLoc, lightViewProj);
            rlEnableShader(shadowShader.id);

            rlActiveTextureSlot(textureActiveSlot);
            rlEnableTexture(shadowMap.depth.id);
            rlSetUniform(shadowMapLoc, &textureActiveSlot, SHADER_UNIFORM_INT, 1);

            RLBeginMode3D(camera);
                DrawScene(cube, robot); // Draw the same exact things as we drew in the shadowmap!
            RLEndMode3D();

            RLDrawText("Use the arrow keys to rotate the light!", 10, 10, 30, RED);
            RLDrawText("Shadows in raylib using the shadowmapping algorithm!", screenWidth - 280, screenHeight - 20, 10, GRAY);

        RLEndDrawing();

        if (RLIsKeyPressed(KEY_F)) RLTakeScreenshot("shaders_shadowmap.png");
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadShader(shadowShader);
    RLUnloadModel(cube);
    RLUnloadModel(robot);
    RLUnloadModelAnimations(robotAnimations, animCount);
    UnloadShadowmapRenderTexture(shadowMap);

    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

// Load render texture for shadowmap projection
// NOTE: Load framebuffer with only a texture depth attachment,
// no color attachment required for shadowmap
static RLRenderTexture2D LoadShadowmapRenderTexture(int width, int height)
{
    RLRenderTexture2D target = { 0 };

    target.id = rlLoadFramebuffer(); // Load an empty framebuffer
    target.texture.width = width;
    target.texture.height = height;

    if (target.id > 0)
    {
        rlEnableFramebuffer(target.id);

        // Create depth texture
        // NOTE: No need a color texture attachment for the shadowmap
        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19; // DEPTH_COMPONENT_24BIT?
        target.depth.mipmaps = 1;

        // Attach depth texture to FBO
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.id)) TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

        rlDisableFramebuffer();
    }
    else TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

// Unload shadowmap render texture from GPU memory (VRAM)
static void UnloadShadowmapRenderTexture(RLRenderTexture2D target)
{
    if (target.id > 0)
    {
        // NOTE: Depth texture/renderbuffer is automatically
        // queried and deleted before deleting framebuffer
        rlUnloadFramebuffer(target.id);
    }
}

// Draw full scene projecting shadows
// NOTE: Required  to be called several time to generate shadowmap
static void DrawScene(RLModel cube, RLModel robot)
{
    RLDrawModelEx(cube, Vector3Zero(), (RLVector3) { 0.0f, 1.0f, 0.0f }, 0.0f, (RLVector3) { 10.0f, 1.0f, 10.0f }, BLUE);
    RLDrawModelEx(cube, (RLVector3) { 1.5f, 1.0f, -1.5f }, (RLVector3) { 0.0f, 1.0f, 0.0f }, 0.0f, Vector3One(), WHITE);
    RLDrawModelEx(robot, (RLVector3) { 0.0f, 0.5f, 0.0f }, (RLVector3) { 0.0f, 1.0f, 0.0f }, 0.0f, (RLVector3) { 1.0f, 1.0f, 1.0f }, RED);
}
