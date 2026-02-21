/*******************************************************************************************
*
*   raylib [others] example - compute shader
*
*   NOTE: This example requires raylib OpenGL 4.3 versions for compute shaders support,
*         shaders used in this example are #version 430 (OpenGL 4.3)
*
*   Example complexity rating: [★★★★] 4/4
*
*   Example originally created with raylib 4.0, last time updated with raylib 4.0
*
*   Example contributed by Teddy Astie (@tsnake41) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2021-2025 Teddy Astie (@tsnake41)
*
********************************************************************************************/

#include "raylib.h"
#include "rlgl.h"

#include <stdlib.h>

// IMPORTANT: This must match gol*.glsl GOL_WIDTH constant
// This must be a multiple of 16 (check golLogic compute dispatch)
#define GOL_WIDTH 768

// Maximum amount of queued draw commands (squares draw from mouse down events)
#define MAX_BUFFERED_TRANSFERTS 48

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Game Of Life Update Command
typedef struct GolUpdateCmd {
    unsigned int x;         // x coordinate of the gol command
    unsigned int y;         // y coordinate of the gol command
    unsigned int w;         // width of the filled zone
    unsigned int enabled;   // whether to enable or disable zone
} GolUpdateCmd;

// Game Of Life Update Commands SSBO
typedef struct GolUpdateSSBO {
    unsigned int count;
    GolUpdateCmd commands[MAX_BUFFERED_TRANSFERTS];
} GolUpdateSSBO;

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = GOL_WIDTH;
    const int screenHeight = GOL_WIDTH;

    RLInitWindow(screenWidth, screenHeight, "raylib [others] example - compute shader");

    const RLVector2 resolution = { (float)screenWidth, (float)screenHeight };
    unsigned int brushSize = 8;

    // Game of Life logic compute shader
    char *golLogicCode = RLLoadFileText("resources/shaders/glsl430/gol.glsl");
    unsigned int golLogicShader = rlCompileShader(golLogicCode, RL_COMPUTE_SHADER);
    unsigned int golLogicProgram = rlLoadComputeShaderProgram(golLogicShader);
    RLUnloadFileText(golLogicCode);

    // Game of Life logic render shader
    RLShader golRenderShader = RLLoadShader(NULL, "resources/shaders/glsl430/gol_render.glsl");
    int resUniformLoc = RLGetShaderLocation(golRenderShader, "resolution");

    // Game of Life transfert shader (CPU<->GPU download and upload)
    char *golTransfertCode = RLLoadFileText("resources/shaders/glsl430/gol_transfert.glsl");
    unsigned int golTransfertShader = rlCompileShader(golTransfertCode, RL_COMPUTE_SHADER);
    unsigned int golTransfertProgram = rlLoadComputeShaderProgram(golTransfertShader);
    RLUnloadFileText(golTransfertCode);

    // Load shader storage buffer object (SSBO), id returned
    unsigned int ssboA = rlLoadShaderBuffer(GOL_WIDTH*GOL_WIDTH*sizeof(unsigned int), NULL, RL_DYNAMIC_COPY);
    unsigned int ssboB = rlLoadShaderBuffer(GOL_WIDTH*GOL_WIDTH*sizeof(unsigned int), NULL, RL_DYNAMIC_COPY);
    unsigned int ssboTransfert = rlLoadShaderBuffer(sizeof(GolUpdateSSBO), NULL, RL_DYNAMIC_COPY);

    GolUpdateSSBO transfertBuffer = { 0 };

    // Create a white texture of the size of the window to update
    // each pixel of the window using the fragment shader: golRenderShader
    RLImage whiteImage = RLGenImageColor(GOL_WIDTH, GOL_WIDTH, WHITE);
    RLTexture whiteTex = RLLoadTextureFromImage(whiteImage);
    RLUnloadImage(whiteImage);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())
    {
        // Update
        //----------------------------------------------------------------------------------
        brushSize += (int)RLGetMouseWheelMove();

        if ((RLIsMouseButtonDown(RL_E_MOUSE_BUTTON_LEFT) || RLIsMouseButtonDown(RL_E_MOUSE_BUTTON_RIGHT))
            && (transfertBuffer.count < MAX_BUFFERED_TRANSFERTS))
        {
            // Buffer a new command
            transfertBuffer.commands[transfertBuffer.count].x = RLGetMouseX() - brushSize/2;
            transfertBuffer.commands[transfertBuffer.count].y = RLGetMouseY() - brushSize/2;
            transfertBuffer.commands[transfertBuffer.count].w = brushSize;
            transfertBuffer.commands[transfertBuffer.count].enabled = RLIsMouseButtonDown(RL_E_MOUSE_BUTTON_LEFT);
            transfertBuffer.count++;
        }
        else if (transfertBuffer.count > 0)  // Process transfert buffer
        {
            // Send SSBO buffer to GPU
            rlUpdateShaderBuffer(ssboTransfert, &transfertBuffer, sizeof(GolUpdateSSBO), 0);

            // Process SSBO commands on GPU
            rlEnableShader(golTransfertProgram);
            rlBindShaderBuffer(ssboA, 1);
            rlBindShaderBuffer(ssboTransfert, 3);
            rlComputeShaderDispatch(transfertBuffer.count, 1, 1); // Each GPU unit will process a command!
            rlDisableShader();

            transfertBuffer.count = 0;
        }
        else
        {
            // Process game of life logic
            rlEnableShader(golLogicProgram);
            rlBindShaderBuffer(ssboA, 1);
            rlBindShaderBuffer(ssboB, 2);
            rlComputeShaderDispatch(GOL_WIDTH/16, GOL_WIDTH/16, 1);
            rlDisableShader();

            // ssboA <-> ssboB
            int temp = ssboA;
            ssboA = ssboB;
            ssboB = temp;
        }

        rlBindShaderBuffer(ssboA, 1);
        RLSetShaderValue(golRenderShader, resUniformLoc, &resolution, RL_E_SHADER_UNIFORM_VEC2);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(BLANK);

            RLBeginShaderMode(golRenderShader);
                RLDrawTexture(whiteTex, 0, 0, WHITE);
            RLEndShaderMode();

            RLDrawRectangleLines(RLGetMouseX() - brushSize/2, RLGetMouseY() - brushSize/2, brushSize, brushSize, RED);

            RLDrawText("Use Mouse wheel to increase/decrease brush size", 10, 10, 20, WHITE);
            RLDrawFPS(RLGetScreenWidth() - 100, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unload shader buffers objects
    rlUnloadShaderBuffer(ssboA);
    rlUnloadShaderBuffer(ssboB);
    rlUnloadShaderBuffer(ssboTransfert);

    // Unload compute shader programs
    rlUnloadShaderProgram(golTransfertProgram);
    rlUnloadShaderProgram(golLogicProgram);

    RLUnloadTexture(whiteTex);            // Unload white texture
    RLUnloadShader(golRenderShader);      // Unload rendering fragment shader

    RLCloseWindow();                      // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
