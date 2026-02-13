/*******************************************************************************************
*
*   raylib [shapes] example - bullet hell
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Example originally created with raylib 5.6, last time updated with raylib 5.6
*
*   Example contributed by Zero (@zerohorsepower) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Zero (@zerohorsepower)
*
********************************************************************************************/

#include "raylib.h"

#include <stdlib.h>         // Required for: calloc(), free()
#include <math.h>           // Required for: cosf(), sinf()

#define MAX_BULLETS 500000      // Max bullets to be processed

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct Bullet {
    RLVector2 position;       // Bullet position on screen
    RLVector2 acceleration;   // Amount of pixels to be incremented to position every frame
    bool disabled;          // Skip processing and draw case out of screen
    RLColor color;            // Bullet color
} Bullet;

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [shapes] example - bullet hell");

    // Bullets definition
    Bullet *bullets = (Bullet *)RL_CALLOC(MAX_BULLETS, sizeof(Bullet)); // Bullets array
    int bulletCount = 0;
    int bulletDisabledCount = 0; // Used to calculate how many bullets are on screen
    int bulletRadius = 10;
    float bulletSpeed = 3.0f;
    int bulletRows = 6;
    RLColor bulletColor[2] = { RED, BLUE };

    // Spawner variables
    float baseDirection = 0;
    int angleIncrement = 5; // After spawn all bullet rows, increment this value on the baseDirection for next the frame
    float spawnCooldown = 2;
    float spawnCooldownTimer = spawnCooldown;

    // Magic circle
    float magicCircleRotation = 0;

    // Used on performance drawing
    RLRenderTexture bulletTexture = RLLoadRenderTexture(24, 24);

    // Draw circle to bullet texture, then draw bullet using DrawTexture()
    // NOTE: This is done to improve the performance, since DrawCircle() is very slow
    RLBeginTextureMode(bulletTexture);
        RLDrawCircle(12, 12, (float)bulletRadius, WHITE);
        RLDrawCircleLines(12, 12, (float)bulletRadius, BLACK);
    RLEndTextureMode();

    bool drawInPerformanceMode = true; // Switch between DrawCircle() and DrawTexture()

    RLSetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // Reset the bullet index
        // New bullets will replace the old ones that are already disabled due to out-of-screen
        if (bulletCount >= MAX_BULLETS)
        {
            bulletCount = 0;
            bulletDisabledCount = 0;
        }

        spawnCooldownTimer--;
        if (spawnCooldownTimer < 0)
        {
            spawnCooldownTimer = spawnCooldown;

            // Spawn bullets
            float degreesPerRow = 360.0f/bulletRows;
            for (int row = 0; row < bulletRows; row++)
            {
                if (bulletCount < MAX_BULLETS)
                {
                    bullets[bulletCount].position = (RLVector2){(float) screenWidth/2, (float) screenHeight/2};
                    bullets[bulletCount].disabled = false;
                    bullets[bulletCount].color = bulletColor[row%2];

                    float bulletDirection = baseDirection + (degreesPerRow*row);

                    // Bullet speed*bullet direction, this will determine how much pixels will be incremented/decremented
                    // from the bullet position every frame. Since the bullets doesn't change its direction and speed,
                    // only need to calculate it at the spawning time
                    // 0 degrees = right, 90 degrees = down, 180 degrees = left and 270 degrees = up, basically clockwise
                    // Case you want it to be anti-clockwise, add "* -1" at the y acceleration
                    bullets[bulletCount].acceleration = (RLVector2){
                        bulletSpeed*cosf(bulletDirection*DEG2RAD),
                        bulletSpeed*sinf(bulletDirection*DEG2RAD)
                    };

                    bulletCount++;
                }
            }

            baseDirection += angleIncrement;
        }

        // Update bullets position based on its acceleration
        for (int i = 0; i < bulletCount; i++)
        {
            // Only update bullet if inside the screen
            if (!bullets[i].disabled)
            {
                bullets[i].position.x += bullets[i].acceleration.x;
                bullets[i].position.y += bullets[i].acceleration.y;

                // Disable bullet if out of screen
                if ((bullets[i].position.x < -bulletRadius*2) ||
                    (bullets[i].position.x > screenWidth + bulletRadius*2) ||
                    (bullets[i].position.y < -bulletRadius*2) ||
                    (bullets[i].position.y > screenHeight + bulletRadius*2))
                {
                    bullets[i].disabled = true;
                    bulletDisabledCount++;
                }
            }
        }

        // Input logic
        if ((RLIsKeyPressed(KEY_RIGHT) || RLIsKeyPressed(KEY_D)) && (bulletRows < 359)) bulletRows++;
        if ((RLIsKeyPressed(KEY_LEFT) || RLIsKeyPressed(KEY_A)) && (bulletRows > 1)) bulletRows--;
        if (RLIsKeyPressed(KEY_UP) || RLIsKeyPressed(KEY_W)) bulletSpeed += 0.25f;
        if ((RLIsKeyPressed(KEY_DOWN) || RLIsKeyPressed(KEY_S)) && (bulletSpeed > 0.50f)) bulletSpeed -= 0.25f;
        if (RLIsKeyPressed(KEY_Z) && (spawnCooldown > 1)) spawnCooldown--;
        if (RLIsKeyPressed(KEY_X)) spawnCooldown++;
        if (RLIsKeyPressed(KEY_ENTER)) drawInPerformanceMode = !drawInPerformanceMode;

        if (RLIsKeyDown(KEY_SPACE))
        {
            angleIncrement += 1;
            angleIncrement %= 360;
        }

        if (RLIsKeyPressed(KEY_C))
        {
            bulletCount = 0;
            bulletDisabledCount = 0;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();
            RLClearBackground(RAYWHITE);

            // Draw magic circle
            magicCircleRotation++;
            RLDrawRectanglePro((RLRectangle){ (float)screenWidth/2, (float)screenHeight/2, 120, 120 },
                (RLVector2){ 60.0f, 60.0f }, magicCircleRotation, PURPLE);
            RLDrawRectanglePro((RLRectangle){ (float)screenWidth/2, (float)screenHeight/2, 120, 120 },
                (RLVector2){ 60.0f, 60.0f }, magicCircleRotation + 45, PURPLE);
            RLDrawCircleLines(screenWidth/2, screenHeight/2, 70, BLACK);
            RLDrawCircleLines(screenWidth/2, screenHeight/2, 50, BLACK);
            RLDrawCircleLines(screenWidth/2, screenHeight/2, 30, BLACK);

            // Draw bullets
            if (drawInPerformanceMode)
            {
                // Draw bullets using pre-rendered texture containing circle
                for (int i = 0; i < bulletCount; i++)
                {
                    // Do not draw disabled bullets (out of screen)
                    if (!bullets[i].disabled)
                    {
                        RLDrawTexture(bulletTexture.texture,
                            (int)(bullets[i].position.x - bulletTexture.texture.width*0.5f),
                            (int)(bullets[i].position.y - bulletTexture.texture.height*0.5f),
                            bullets[i].color);
                    }
                }
            }
            else
            {
                // Draw bullets using DrawCircle(), less performant
                for (int i = 0; i < bulletCount; i++)
                {
                    // Do not draw disabled bullets (out of screen)
                    if (!bullets[i].disabled)
                    {
                        RLDrawCircleV(bullets[i].position, (float)bulletRadius, bullets[i].color);
                        RLDrawCircleLinesV(bullets[i].position, (float)bulletRadius, BLACK);
                    }
                }
            }

            // Draw UI
            RLDrawRectangle(10, 10, 280, 150, (RLColor){0,0, 0, 200 });
            RLDrawText("Controls:", 20, 20, 10, LIGHTGRAY);
            RLDrawText("- Right/Left or A/D: Change rows number", 40, 40, 10, LIGHTGRAY);
            RLDrawText("- Up/Down or W/S: Change bullet speed", 40, 60, 10, LIGHTGRAY);
            RLDrawText("- Z or X: Change spawn cooldown", 40, 80, 10, LIGHTGRAY);
            RLDrawText("- Space (Hold): Change the angle increment", 40, 100, 10, LIGHTGRAY);
            RLDrawText("- Enter: Switch draw method (Performance)", 40, 120, 10, LIGHTGRAY);
            RLDrawText("- C: Clear bullets", 40, 140, 10, LIGHTGRAY);

            RLDrawRectangle(610, 10, 170, 30, (RLColor){0,0, 0, 200 });
            if (drawInPerformanceMode) RLDrawText("Draw method: DrawTexture(*)", 620, 20, 10, GREEN);
            else RLDrawText("Draw method: DrawCircle(*)", 620, 20, 10, RED);

            RLDrawRectangle(135, 410, 530, 30, (RLColor){0,0, 0, 200 });
            RLDrawText(RLTextFormat("[ FPS: %d, Bullets: %d, Rows: %d, Bullet speed: %.2f, Angle increment per frame: %d, Cooldown: %.0f ]",
                    RLGetFPS(), bulletCount - bulletDisabledCount, bulletRows, bulletSpeed,  angleIncrement, spawnCooldown),
                155, 420, 10, GREEN);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RLUnloadRenderTexture(bulletTexture); // Unload bullet texture

    RL_FREE(bullets);     // Free bullets array data

    RLCloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}