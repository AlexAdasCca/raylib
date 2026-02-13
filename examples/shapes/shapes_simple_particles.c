/*******************************************************************************************
*
*   raylib [shapes] example - simple particles
*
*   Example complexity rating: [★★☆☆] 2/4
*
*   Example originally created with raylib 5.6, last time updated with raylib 5.6
*
*   Example contributed by Jordi Santonja (@JordSant)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2025 Jordi Santonja (@JordSant)
*
********************************************************************************************/

#include "raylib.h"

#include <stdlib.h>         // Required for: calloc(), free()
#include <math.h>           // Required for: cosf(), sinf()

#define MAX_PARTICLES 3000  // Max number of particles

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum ParticleType {
    WATER = 0,
    SMOKE,
    FIRE
} ParticleType;

static const char particleTypeNames[3][10] = { "WATER", "SMOKE", "FIRE" };

typedef struct Particle {
    ParticleType type;      // Particle type (WATER, SMOKE, FIRE)
    RLVector2 position;       // Particle position on screen
    RLVector2 velocity;       // Particle current speed and direction
    float radius;           // Particle radius
    RLColor color;            // Particle color

    float lifeTime;         // Particle life time
    bool alive;             // Particle alive: inside screen and life time
} Particle;

typedef struct CircularBuffer {
    int head;               // Index for the next write
    int tail;               // Index for the next read
    Particle *buffer;       // Particle buffer array
} CircularBuffer;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void EmitParticle(CircularBuffer *circularBuffer, RLVector2 emitterPosition, ParticleType type);
static Particle *AddToCircularBuffer(CircularBuffer *circularBuffer);
static void UpdateParticles(CircularBuffer *circularBuffer, int screenWidth, int screenHeight);
static void UpdateCircularBuffer(CircularBuffer *circularBuffer);
static void DrawParticles(CircularBuffer *circularBuffer);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    RLInitWindow(screenWidth, screenHeight, "raylib [shapes] example - simple particles");

    // Definition of particles
    Particle *particles = (Particle*)RL_CALLOC(MAX_PARTICLES, sizeof(Particle)); // Particle array
    CircularBuffer circularBuffer = { 0, 0, particles };

    // Particle emitter parameters
    int emissionRate = -2;          // Negative: on average every -X frames. Positive: particles per frame
    ParticleType currentType = WATER;
    RLVector2 emitterPosition = { screenWidth/2.0f, screenHeight/2.0f };

    RLSetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!RLWindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // Emit new particles: when emissionRate is 1, emit every frame
        if (emissionRate < 0)
        {
            if (rand()%(-emissionRate) == 0) EmitParticle(&circularBuffer, emitterPosition, currentType);
        }
        else
        {
            for (int i = 0; i <= emissionRate; i++) EmitParticle(&circularBuffer, emitterPosition, currentType);
        }

        // Update the parameters of each particle
        UpdateParticles(&circularBuffer, screenWidth, screenHeight);

        // Remove dead particles from the circular buffer
        UpdateCircularBuffer(&circularBuffer);

        // Change Particle Emission Rate (UP/DOWN arrows)
        if (RLIsKeyPressed(KEY_UP)) emissionRate++;
        if (RLIsKeyPressed(KEY_DOWN)) emissionRate--;

        // Change Particle Type (LEFT/RIGHT arrows)
        if (RLIsKeyPressed(KEY_RIGHT)) (currentType == FIRE)? (currentType = WATER) : currentType++;
        if (RLIsKeyPressed(KEY_LEFT)) (currentType == WATER)? (currentType = FIRE) : currentType--;

        if (RLIsMouseButtonDown(MOUSE_LEFT_BUTTON)) emitterPosition = RLGetMousePosition();
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        RLBeginDrawing();

            RLClearBackground(RAYWHITE);

            // Call the function with a loop to draw all particles
            DrawParticles(&circularBuffer);

            // Draw UI and Instructions
            RLDrawRectangle(5, 5, 315, 75, RLFade(SKYBLUE, 0.5f));
            RLDrawRectangleLines(5, 5, 315, 75, BLUE);

            RLDrawText("CONTROLS:", 15, 15, 10, BLACK);
            RLDrawText("UP/DOWN: Change Particle Emission Rate", 15, 35, 10, BLACK);
            RLDrawText("LEFT/RIGHT: Change Particle Type (Water, Smoke, Fire)", 15, 55, 10, BLACK);

            if (emissionRate < 0) RLDrawText(RLTextFormat("Particles every %d frames | Type: %s", -emissionRate, particleTypeNames[currentType]), 15, 95, 10, DARKGRAY);
            else RLDrawText(RLTextFormat("%d Particles per frame | Type: %s", emissionRate + 1, particleTypeNames[currentType]), 15, 95, 10, DARKGRAY);

            RLDrawFPS(screenWidth - 80, 10);

        RLEndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    RL_FREE(particles);     // Free particles array data

    RLCloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
static void EmitParticle(CircularBuffer *circularBuffer, RLVector2 emitterPosition, ParticleType type)
{
    Particle *newParticle = AddToCircularBuffer(circularBuffer);

    // If buffer is full, newParticle is NULL
    if (newParticle != NULL)
    {
        // Fill particle properties
        newParticle->position = emitterPosition;
        newParticle->alive = true;
        newParticle->lifeTime = 0.0f;
        newParticle->type = type;
        float speed = (float)(rand()%10)/5.0f;
        switch (type)
        {
            case WATER:
            {
                newParticle->radius = 5.0f;
                newParticle->color = BLUE;
            } break;
            case SMOKE:
            {
                newParticle->radius = 7.0f;
                newParticle->color = GRAY;
            } break;
            case FIRE:
            {
                newParticle->radius = 10.0f;
                newParticle->color = YELLOW;
                speed /= 10.0f;
            } break;
            default: break;
        }

        float direction = (float)(rand()%360);
        newParticle->velocity = (RLVector2){ speed*cosf(direction*DEG2RAD), speed*sinf(direction*DEG2RAD) };
    }
}

static Particle *AddToCircularBuffer(CircularBuffer *circularBuffer)
{
    Particle *particle = NULL;

    // Check if buffer full
    if (((circularBuffer->head + 1)%MAX_PARTICLES) != circularBuffer->tail)
    {
        // Add new particle to the head position and advance head
        particle = &circularBuffer->buffer[circularBuffer->head];
        circularBuffer->head = (circularBuffer->head + 1)%MAX_PARTICLES;
    }

    return particle;
}

static void UpdateParticles(CircularBuffer *circularBuffer, int screenWidth, int screenHeight)
{
    for (int i = circularBuffer->tail; i != circularBuffer->head; i = (i + 1)%MAX_PARTICLES)
    {
        // Update particle life and positions
        circularBuffer->buffer[i].lifeTime += 1.0f/60.0f; // 60 FPS -> 1/60 seconds per frame

        switch (circularBuffer->buffer[i].type)
        {
            case WATER:
            {
                circularBuffer->buffer[i].position.x += circularBuffer->buffer[i].velocity.x;
                circularBuffer->buffer[i].velocity.y += 0.2f;   // Gravity
                circularBuffer->buffer[i].position.y += circularBuffer->buffer[i].velocity.y;
            } break;
            case SMOKE:
            {
                circularBuffer->buffer[i].position.x += circularBuffer->buffer[i].velocity.x;
                circularBuffer->buffer[i].velocity.y -= 0.05f;  // Upwards
                circularBuffer->buffer[i].position.y += circularBuffer->buffer[i].velocity.y;
                circularBuffer->buffer[i].radius += 0.5f;       // Increment radius: smoke expands
                circularBuffer->buffer[i].color.a -= 4;         // Decrement alpha: smoke fades

                // If alpha transparent, particle dies
                if (circularBuffer->buffer[i].color.a < 4) circularBuffer->buffer[i].alive = false;
            } break;
            case FIRE:
            {
                // Add a little horizontal oscillation to fire particles
                circularBuffer->buffer[i].position.x += circularBuffer->buffer[i].velocity.x + cosf(circularBuffer->buffer[i].lifeTime*215.0f);
                circularBuffer->buffer[i].velocity.y -= 0.05f;  // Upwards
                circularBuffer->buffer[i].position.y += circularBuffer->buffer[i].velocity.y;
                circularBuffer->buffer[i].radius -= 0.15f;      // Decrement radius: fire shrinks
                circularBuffer->buffer[i].color.g -= 3;         // Decrement green: fire turns reddish starting from yellow

                // If radius too small, particle dies
                if (circularBuffer->buffer[i].radius <= 0.02f) circularBuffer->buffer[i].alive = false;
            } break;
            default: break;
        }

        // Disable particle when out of screen
        RLVector2 center = circularBuffer->buffer[i].position;
        float radius = circularBuffer->buffer[i].radius;

        if ((center.x < -radius) || (center.x > (screenWidth + radius)) ||
            (center.y < -radius) || (center.y > (screenHeight + radius)))
        {
            circularBuffer->buffer[i].alive = false;
        }
    }
}

static void UpdateCircularBuffer(CircularBuffer *circularBuffer)
{
    // Update circular buffer: advance tail over dead particles
    while ((circularBuffer->tail != circularBuffer->head) && !circularBuffer->buffer[circularBuffer->tail].alive)
    {
        circularBuffer->tail = (circularBuffer->tail + 1)%MAX_PARTICLES;
    }
}

static void DrawParticles(CircularBuffer *circularBuffer)
{
    for (int i = circularBuffer->tail; i != circularBuffer->head; i = (i + 1)%MAX_PARTICLES)
    {
        if (circularBuffer->buffer[i].alive)
        {
            RLDrawCircleV(circularBuffer->buffer[i].position,
                        circularBuffer->buffer[i].radius,
                        circularBuffer->buffer[i].color);
        }
    }
}
