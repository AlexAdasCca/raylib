/*******************************************************************************************
*
*   raylib [models] example - mesh picking
*
*   Example complexity rating: [★★★☆] 3/4
*
*   Example originally created with raylib 1.7, last time updated with raylib 4.0
*
*   Example contributed by Joel Davis (@joeld42) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2017-2025 Joel Davis (@joeld42) and Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"

#undef FLT_MAX
#define FLT_MAX     340282346638528859811704183484516925440.0f     // Maximum value of a float, from bit pattern 01111111011111111111111111111111

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [models] example - mesh picking");

    // Define the camera to look into our 3d world
    RLCamera camera = { 0 };
    camera.position = (RLVector3){ 20.0f, 20.0f, 20.0f }; // Camera position
    camera.target = (RLVector3){ 0.0f, 8.0f, 0.0f };      // Camera looking at point
    camera.up = (RLVector3){ 0.0f, 1.6f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    RLRay ray = { 0 };        // Picking ray

    RLModel tower = RLLoadModel("resources/models/obj/turret.obj");                 // Load OBJ model
    RLTexture2D texture = RLLoadTexture("resources/models/obj/turret_diffuse.png"); // Load model texture
    tower.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;            // Set model diffuse texture

    RLVector3 towerPos = { 0.0f, 0.0f, 0.0f };                        // Set model position
    RLBoundingBox towerBBox = RLGetMeshBoundingBox(tower.meshes[0]);    // Get mesh bounding box

    // Ground quad
    RLVector3 g0 = (RLVector3){ -50.0f, 0.0f, -50.0f };
    RLVector3 g1 = (RLVector3){ -50.0f, 0.0f,  50.0f };
    RLVector3 g2 = (RLVector3){  50.0f, 0.0f,  50.0f };
    RLVector3 g3 = (RLVector3){  50.0f, 0.0f, -50.0f };

    // Test triangle
    RLVector3 ta = (RLVector3){ -25.0f, 0.5f, 0.0f };
    RLVector3 tb = (RLVector3){ -4.0f, 2.5f, 1.0f };
    RLVector3 tc = (RLVector3){ -8.0f, 6.5f, 0.0f };

    RLVector3 bary = { 0.0f, 0.0f, 0.0f };

    // Test sphere
    RLVector3 sp = (RLVector3){ -30.0f, 5.0f, 5.0f };
    float sr = 4.0f;

    RLSetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------
    // Main game loop
    while (!RLWindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if (RLIsCursorHidden()) RLUpdateCamera(&camera, CAMERA_FIRST_PERSON);          // Update camera

        // Toggle camera controls
        if (RLIsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
        {
            if (RLIsCursorHidden()) RLEnableCursor();
            else RLDisableCursor();
        }

        // Display information about closest hit
        RLRayCollision collision = { 0 };
        const char *hitObjectName = "None";
        collision.distance = FLT_MAX;
        collision.hit = false;
        RLColor cursorColor = WHITE;

        // Get ray and test against objects
        ray = RLGetScreenToWorldRay(RLGetMousePosition(), camera);

        // Check ray collision against ground quad
        RLRayCollision groundHitInfo = RLGetRayCollisionQuad(ray, g0, g1, g2, g3);

        if ((groundHitInfo.hit) && (groundHitInfo.distance < collision.distance))
        {
            collision = groundHitInfo;
            cursorColor = GREEN;
            hitObjectName = "Ground";
        }

        // Check ray collision against test triangle
        RLRayCollision triHitInfo = RLGetRayCollisionTriangle(ray, ta, tb, tc);

        if ((triHitInfo.hit) && (triHitInfo.distance < collision.distance))
        {
            collision = triHitInfo;
            cursorColor = PURPLE;
            hitObjectName = "Triangle";

            bary = Vector3Barycenter(collision.point, ta, tb, tc);
        }

        // Check ray collision against test sphere
        RLRayCollision sphereHitInfo = RLGetRayCollisionSphere(ray, sp, sr);

        if ((sphereHitInfo.hit) && (sphereHitInfo.distance < collision.distance))
        {
            collision = sphereHitInfo;
            cursorColor = ORANGE;
            hitObjectName = "Sphere";
        }

        // Check ray collision against bounding box first, before trying the full ray-mesh test
        RLRayCollision boxHitInfo = RLGetRayCollisionBox(ray, towerBBox);

        if ((boxHitInfo.hit) && (boxHitInfo.distance < collision.distance))
        {
            collision = boxHitInfo;
            cursorColor = ORANGE;
            hitObjectName = "Box";

            // Check ray collision against model meshes
            RLRayCollision meshHitInfo = { 0 };
            for (int m = 0; m < tower.meshCount; m++)
            {
                // NOTE: We consider the model.transform for the collision check but
                // it can be checked against any transform Matrix, used when checking against same
                // model drawn multiple times with multiple transforms
                meshHitInfo = RLGetRayCollisionMesh(ray, tower.meshes[m], tower.transform);
                if (meshHitInfo.hit)
                {
                    // Save the closest hit mesh
                    if ((!collision.hit) || (collision.distance > meshHitInfo.distance)) collision = meshHitInfo;

                    break;  // Stop once one mesh collision is detected, the colliding mesh is m
                }
            }

            if (meshHitInfo.hit)
            {
                collision = meshHitInfo;
                cursorColor = ORANGE;
                hitObjectName = "Mesh";
            }
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            RLBeginMode3D(camera);

                // Draw the tower
                // WARNING: If scale is different than 1.0f,
                // not considered by GetRayCollisionModel()
                RLDrawModel(tower, towerPos, 1.0f, WHITE);

                // Draw the test triangle
                RLDrawLine3D(ta, tb, PURPLE);
                RLDrawLine3D(tb, tc, PURPLE);
                RLDrawLine3D(tc, ta, PURPLE);

                // Draw the test sphere
                RLDrawSphereWires(sp, sr, 8, 8, PURPLE);

                // Draw the mesh bbox if we hit it
                if (boxHitInfo.hit) RLDrawBoundingBox(towerBBox, LIME);

                // If we hit something, draw the cursor at the hit point
                if (collision.hit)
                {
                    RLDrawCube(collision.point, 0.3f, 0.3f, 0.3f, cursorColor);
                    RLDrawCubeWires(collision.point, 0.3f, 0.3f, 0.3f, RED);

                    RLVector3 normalEnd;
                    normalEnd.x = collision.point.x + collision.normal.x;
                    normalEnd.y = collision.point.y + collision.normal.y;
                    normalEnd.z = collision.point.z + collision.normal.z;

                    RLDrawLine3D(collision.point, normalEnd, RED);
                }

                RLDrawRay(ray, MAROON);

                RLDrawGrid(10, 10.0f);

            RLEndMode3D();

            // Draw some debug GUI text
            RLDrawText(RLTextFormat("Hit Object: %s", hitObjectName), 10, 50, 10, BLACK);

            if (collision.hit)
            {
                int ypos = 70;

                RLDrawText(RLTextFormat("Distance: %3.2f", collision.distance), 10, ypos, 10, BLACK);

                RLDrawText(RLTextFormat("Hit Pos: %3.2f %3.2f %3.2f",
                                    collision.point.x,
                                    collision.point.y,
                                    collision.point.z), 10, ypos + 15, 10, BLACK);

                RLDrawText(RLTextFormat("Hit Norm: %3.2f %3.2f %3.2f",
                                    collision.normal.x,
                                    collision.normal.y,
                                    collision.normal.z), 10, ypos + 30, 10, BLACK);

                if (triHitInfo.hit && RLTextIsEqual(hitObjectName, "Triangle"))
                    RLDrawText(RLTextFormat("Barycenter: %3.2f %3.2f %3.2f",  bary.x, bary.y, bary.z), 10, ypos + 45, 10, BLACK);
            }

            RLDrawText("Right click mouse to toggle camera controls", 10, 430, 10, GRAY);

            RLDrawText("(c) Turret 3D model by Alberto Cano", screenWidth - 200, screenHeight - 20, 10, GRAY);

            RLDrawFPS(10, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadModel(tower);         // Unload model
    RLUnloadTexture(texture);     // Unload texture

    RLCloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
