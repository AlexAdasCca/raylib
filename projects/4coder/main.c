#include <math.h>
#include "raylib.h"

int main()
{
    int screenWidth = 800;
    int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib");

    RLCamera cam;
    cam.position = (RLVector3){ 0.0f, 10.0f, 8.f };
    cam.target = (RLVector3){ 0.0f, 0.0f, 0.0f };
    cam.up = (RLVector3){ 0.0f, 1.f, 0.0f };
    cam.fovy = 60.0f;

    RLVector3 cubePos = { 0.0f, 0.0f, 0.0f };

    RLSetTargetFPS(60);

    while (!RLWindowShouldClose())
    {
        cam.position.x = sin(RLGetTime())*10.0f;
        cam.position.z = cos(RLGetTime())*10.0f;

        RLBeginDrawing();
            RLClearBackground(RAYWHITE);
            RLBeginMode3D(cam);
                RLDrawCube(cubePos, 2.f, 2.f, 2.f, RED);
                RLDrawCubeWires(cubePos, 2.f, 2.f, 2.f, MAROON);
                RLDrawGrid(10, 1.f);
            RLEndMode3D();
            RLDrawText("This is a raylib example", 10, 40, 20, DARKGRAY);
            RLDrawFPS(10, 10);
        RLEndDrawing();
    }
    
    RLCloseWindow();
    return 0;
}
