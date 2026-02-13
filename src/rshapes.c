/**********************************************************************************************
*
*   rshapes - Basic functions to draw 2d shapes and check collisions
*
*   ADDITIONAL NOTES:
*       Shapes can be draw using 3 types of primitives: LINES, TRIANGLES and QUADS
*       Some functions implement two drawing options: TRIANGLES and QUADS, by default TRIANGLES
*       are used but QUADS implementation can be selected with SUPPORT_QUADS_DRAW_MODE define
*
*       Some functions define texture coordinates (rlTexCoord2f()) for the shapes and use a
*       user-provided texture with SetShapesTexture(), the pourpouse of this implementation
*       is allowing to reduce draw calls when combined with a texture-atlas
*
*       By default, raylib sets the default texture and rectangle at InitWindow()[rcore] to one
*       white character of default font [rtext], this way, raylib text and shapes can be draw with
*       a single draw call and it also allows users to configure it the same way with their own fonts
*
*   CONFIGURATION:
*       #define SUPPORT_MODULE_RSHAPES
*           rshapes module is included in the build
*
*       #define SUPPORT_QUADS_DRAW_MODE
*           Use QUADS instead of TRIANGLES for drawing when possible. Lines-based shapes still use LINES
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2013-2026 Ramon Santamaria (@raysan5)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#include "raylib.h"     // Declares module functions

#include "config.h"     // Defines module configuration flags

#if defined(SUPPORT_MODULE_RSHAPES)

#include "rlgl.h"       // OpenGL abstraction layer to OpenGL 1.1, 2.1, 3.3+ or ES2
#include "rl_context.h"         // Route2: context management

#include <math.h>       // Required for: sinf(), asinf(), cosf(), acosf(), sqrtf(), fabsf()
#include <float.h>      // Required for: FLT_EPSILON
#include <stdlib.h>     // Required for: RL_FREE

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#ifndef SMOOTH_CIRCLE_ERROR_RATE
    // Define error rate to calculate how many segments are needed to draw a smooth circle
    // REF: https://stackoverflow.com/a/2244088
    #define SMOOTH_CIRCLE_ERROR_RATE    0.5f      // Circle error rate
#endif
#ifndef SPLINE_SEGMENT_DIVISIONS
    #define SPLINE_SEGMENT_DIVISIONS      24      // Spline segment divisions
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Not here...

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
#define texShapes       (RLGetCurrentContext()->stShapesTexture)
#define texShapesRec    (RLGetCurrentContext()->stShapesTextureRec)
#define shapesReady     (RLGetCurrentContext()->bIsShapesTextureReady)

static inline void RLEnsureShapesState(void)
{
    if (!shapesReady)
    {
        texShapes = (RLTexture2D){ (unsigned int)rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
        texShapesRec = (RLRectangle){ 0.0f, 0.0f, 1.0f, 1.0f };
        shapesReady = true;
    }
}
//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------
static float EaseCubicInOut(float t, float b, float c, float d);    // Cubic easing

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
// Set texture and rectangle to be used on shapes drawing
// NOTE: It can be useful when using basic shapes and one single font,
// defining a font char white rectangle would allow drawing everything in a single draw call
void RLSetShapesTexture(RLTexture2D texture, RLRectangle source)
{
    // Reset texture to default pixel if required
    // WARNING: Shapes texture should be probably better validated,
    // it can break the rendering of all shapes if misused
    if ((texture.id == 0) || (source.width == 0) || (source.height == 0))
    {
        texShapes = (RLTexture2D){ (unsigned int)rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
        texShapesRec = (RLRectangle){ 0.0f, 0.0f, 1.0f, 1.0f };
    }
    else
    {
        texShapes = texture;
        texShapesRec = source;
    }

    shapesReady = true;
}

// Get texture that is used for shapes drawing
RLTexture2D RLGetShapesTexture(void)
{
    RLEnsureShapesState();
    return texShapes;
}

// Get texture source rectangle that is used for shapes drawing
RLRectangle RLGetShapesTextureRectangle(void)
{
    RLEnsureShapesState();
    return texShapesRec;
}

// Draw a pixel
void RLDrawPixel(int posX, int posY, RLColor color)
{
    RLDrawPixelV((RLVector2){ (float)posX, (float)posY }, color);
}

// Draw a pixel (Vector version)
void RLDrawPixelV(RLVector2 position, RLColor color)
{
#if defined(SUPPORT_QUADS_DRAW_MODE)
    rlSetTexture(RLGetShapesTexture().id);
    RLRectangle shapeRect = RLGetShapesTextureRectangle();

    rlBegin(RL_QUADS);

        rlNormal3f(0.0f, 0.0f, 1.0f);
        rlColor4ub(color.r, color.g, color.b, color.a);

        rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(position.x, position.y);

        rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(position.x, position.y + 1);

        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(position.x + 1, position.y + 1);

        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(position.x + 1, position.y);

    rlEnd();

    rlSetTexture(0);
#else
    rlBegin(RL_TRIANGLES);

        rlColor4ub(color.r, color.g, color.b, color.a);

        rlVertex2f(position.x, position.y);
        rlVertex2f(position.x, position.y + 1);
        rlVertex2f(position.x + 1, position.y);

        rlVertex2f(position.x + 1, position.y);
        rlVertex2f(position.x, position.y + 1);
        rlVertex2f(position.x + 1, position.y + 1);

    rlEnd();
#endif
}

// Draw a line (using gl lines)
void RLDrawLine(int startPosX, int startPosY, int endPosX, int endPosY, RLColor color)
{
    rlBegin(RL_LINES);
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex2f((float)startPosX, (float)startPosY);
        rlVertex2f((float)endPosX, (float)endPosY);
    rlEnd();
}

// Draw a line defining thickness
void RLDrawLineEx(RLVector2 startPos, RLVector2 endPos, float thick, RLColor color)
{
    RLVector2 delta = { endPos.x - startPos.x, endPos.y - startPos.y };
    float length = sqrtf(delta.x*delta.x + delta.y*delta.y);

    if ((length > 0) && (thick > 0))
    {
        float scale = thick/(2*length);

        RLVector2 radius = { -scale*delta.y, scale*delta.x };
        RLVector2 strip[4] = {
            { startPos.x - radius.x, startPos.y - radius.y },
            { startPos.x + radius.x, startPos.y + radius.y },
            { endPos.x - radius.x, endPos.y - radius.y },
            { endPos.x + radius.x, endPos.y + radius.y }
        };

        RLDrawTriangleStrip(strip, 4, color);
    }
}

// Draw a line (using gl lines)
void RLDrawLineV(RLVector2 startPos, RLVector2 endPos, RLColor color)
{
    rlBegin(RL_LINES);
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex2f(startPos.x, startPos.y);
        rlVertex2f(endPos.x, endPos.y);
    rlEnd();
}

// Draw lines sequuence (using gl lines)
void RLDrawLineStrip(const RLVector2 *points, int pointCount, RLColor color)
{
    if (pointCount < 2) return; // Security check

    rlBegin(RL_LINES);
        rlColor4ub(color.r, color.g, color.b, color.a);

        for (int i = 0; i < pointCount - 1; i++)
        {
            rlVertex2f(points[i].x, points[i].y);
            rlVertex2f(points[i + 1].x, points[i + 1].y);
        }
    rlEnd();
}

// Draw line using cubic-bezier spline, in-out interpolation, no control points
void RLDrawLineBezier(RLVector2 startPos, RLVector2 endPos, float thick, RLColor color)
{
    RLVector2 previous = startPos;
    RLVector2 current = { 0 };

    RLVector2 points[2*SPLINE_SEGMENT_DIVISIONS + 2] = { 0 };

    for (int i = 1; i <= SPLINE_SEGMENT_DIVISIONS; i++)
    {
        // Cubic easing in-out
        // NOTE: Easing is calculated only for y position value
        current.y = EaseCubicInOut((float)i, startPos.y, endPos.y - startPos.y, (float)SPLINE_SEGMENT_DIVISIONS);
        current.x = previous.x + (endPos.x - startPos.x)/(float)SPLINE_SEGMENT_DIVISIONS;

        float dy = current.y - previous.y;
        float dx = current.x - previous.x;
        float size = 0.5f*thick/sqrtf(dx*dx+dy*dy);

        if (i == 1)
        {
            points[0].x = previous.x + dy*size;
            points[0].y = previous.y - dx*size;
            points[1].x = previous.x - dy*size;
            points[1].y = previous.y + dx*size;
        }

        points[2*i + 1].x = current.x - dy*size;
        points[2*i + 1].y = current.y + dx*size;
        points[2*i].x = current.x + dy*size;
        points[2*i].y = current.y - dx*size;

        previous = current;
    }

    RLDrawTriangleStrip(points, 2*SPLINE_SEGMENT_DIVISIONS + 2, color);
}

// Draw a dashed line
void RLDrawLineDashed(RLVector2 startPos, RLVector2 endPos, int dashSize, int spaceSize, RLColor color)
{
    // Calculate the vector and length of the line
    float dx = endPos.x - startPos.x;
    float dy = endPos.y - startPos.y;
    float lineLength = sqrtf(dx*dx + dy*dy);

    // If the line is too short for dashing or dash size is invalid, draw a solid thick line
    if ((lineLength < (dashSize + spaceSize)) || (dashSize <= 0))
    {
        RLDrawLineV(startPos, endPos, color);
        return;
    }

    // Calculate the normalized direction vector of the line
    float invLineLength = 1.0f/lineLength;
    float dirX = dx*invLineLength;
    float dirY = dy*invLineLength;

    RLVector2 currentPos = startPos;
    float distanceTraveled = 0;

    rlBegin(RL_LINES);
        rlColor4ub(color.r, color.g, color.b, color.a);

        while (distanceTraveled < lineLength)
        {
            // Calculate the end of the current dash
            float dashEndDist = distanceTraveled + dashSize;
            if (dashEndDist > lineLength) dashEndDist = lineLength;

            RLVector2 dashEndPos = { startPos.x + dashEndDist*dirX, startPos.y + dashEndDist*dirY };

            // Draw the dash segment
            rlVertex2f(currentPos.x, currentPos.y);
            rlVertex2f(dashEndPos.x, dashEndPos.y);

            // Update the distance traveled and move the current position for the next dash
            distanceTraveled = dashEndDist + spaceSize;
            currentPos.x = startPos.x + distanceTraveled*dirX;
            currentPos.y = startPos.y + distanceTraveled*dirY;
        }
    rlEnd();
}

// Draw a color-filled circle
void RLDrawCircle(int centerX, int centerY, float radius, RLColor color)
{
    RLDrawCircleV((RLVector2){ (float)centerX, (float)centerY }, radius, color);
}

// Draw a color-filled circle (Vector version)
// NOTE: On OpenGL 3.3 and ES2 using QUADS to avoid drawing order issues
void RLDrawCircleV(RLVector2 center, float radius, RLColor color)
{
    RLDrawCircleSector(center, radius, 0, 360, 36, color);
}

// Draw a piece of a circle
void RLDrawCircleSector(RLVector2 center, float radius, float startAngle, float endAngle, int segments, RLColor color)
{
    if (startAngle == endAngle) return;
    if (radius <= 0.0f) radius = 0.1f;  // Avoid div by zero

    // Function expects (endAngle > startAngle)
    if (endAngle < startAngle)
    {
        // Swap values
        float tmp = startAngle;
        startAngle = endAngle;
        endAngle = tmp;
    }

    int minSegments = (int)ceilf((endAngle - startAngle)/90);

    if (segments < minSegments)
    {
        // Calculate the maximum angle between segments based on the error rate (usually 0.5f)
        float th = acosf(2*powf(1 - SMOOTH_CIRCLE_ERROR_RATE/radius, 2) - 1);
        segments = (int)((endAngle - startAngle)*ceilf(2*PI/th)/360);

        if (segments <= 0) segments = minSegments;
    }

    float stepLength = (endAngle - startAngle)/(float)segments;
    float angle = startAngle;

#if defined(SUPPORT_QUADS_DRAW_MODE)
    rlSetTexture(RLGetShapesTexture().id);
    RLRectangle shapeRect = RLGetShapesTextureRectangle();

    rlBegin(RL_QUADS);

        // NOTE: Every QUAD actually represents two segments
        for (int i = 0; i < segments/2; i++)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);

            rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(center.x, center.y);

            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength*2.0f))*radius, center.y + sinf(DEG2RAD*(angle + stepLength*2.0f))*radius);

            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*radius, center.y + sinf(DEG2RAD*(angle + stepLength))*radius);

            rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(center.x + cosf(DEG2RAD*angle)*radius, center.y + sinf(DEG2RAD*angle)*radius);

            angle += (stepLength*2.0f);
        }

        // NOTE: In case number of segments is odd, adding one last piece to the cake
        if ((((unsigned int)segments)%2) == 1)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);

            rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(center.x, center.y);

            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*radius, center.y + sinf(DEG2RAD*(angle + stepLength))*radius);

            rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(center.x + cosf(DEG2RAD*angle)*radius, center.y + sinf(DEG2RAD*angle)*radius);

            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(center.x, center.y);
        }

    rlEnd();

    rlSetTexture(0);
#else
    rlBegin(RL_TRIANGLES);
        for (int i = 0; i < segments; i++)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);

            rlVertex2f(center.x, center.y);
            rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*radius, center.y + sinf(DEG2RAD*(angle + stepLength))*radius);
            rlVertex2f(center.x + cosf(DEG2RAD*angle)*radius, center.y + sinf(DEG2RAD*angle)*radius);

            angle += stepLength;
        }
    rlEnd();
#endif
}

// Draw a piece of a circle outlines
void RLDrawCircleSectorLines(RLVector2 center, float radius, float startAngle, float endAngle, int segments, RLColor color)
{
    if (startAngle == endAngle) return;
    if (radius <= 0.0f) radius = 0.1f;  // Avoid div by zero issue

    // Function expects (endAngle > startAngle)
    if (endAngle < startAngle)
    {
        // Swap values
        float tmp = startAngle;
        startAngle = endAngle;
        endAngle = tmp;
    }

    int minSegments = (int)ceilf((endAngle - startAngle)/90);

    if (segments < minSegments)
    {
        // Calculate the maximum angle between segments based on the error rate (usually 0.5f)
        float th = acosf(2*powf(1 - SMOOTH_CIRCLE_ERROR_RATE/radius, 2) - 1);
        segments = (int)((endAngle - startAngle)*ceilf(2*PI/th)/360);

        if (segments <= 0) segments = minSegments;
    }

    float stepLength = (endAngle - startAngle)/(float)segments;
    float angle = startAngle;
    bool showCapLines = true;

    rlBegin(RL_LINES);
        if (showCapLines)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);
            rlVertex2f(center.x, center.y);
            rlVertex2f(center.x + cosf(DEG2RAD*angle)*radius, center.y + sinf(DEG2RAD*angle)*radius);
        }

        for (int i = 0; i < segments; i++)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);

            rlVertex2f(center.x + cosf(DEG2RAD*angle)*radius, center.y + sinf(DEG2RAD*angle)*radius);
            rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*radius, center.y + sinf(DEG2RAD*(angle + stepLength))*radius);

            angle += stepLength;
        }

        if (showCapLines)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);
            rlVertex2f(center.x, center.y);
            rlVertex2f(center.x + cosf(DEG2RAD*angle)*radius, center.y + sinf(DEG2RAD*angle)*radius);
        }
    rlEnd();
}

// Draw a gradient-filled circle
void RLDrawCircleGradient(int centerX, int centerY, float radius, RLColor inner, RLColor outer)
{
    rlBegin(RL_TRIANGLES);
        for (int i = 0; i < 360; i += 10)
        {
            rlColor4ub(inner.r, inner.g, inner.b, inner.a);
            rlVertex2f((float)centerX, (float)centerY);
            rlColor4ub(outer.r, outer.g, outer.b, outer.a);
            rlVertex2f((float)centerX + cosf(DEG2RAD*(i + 10))*radius, (float)centerY + sinf(DEG2RAD*(i + 10))*radius);
            rlColor4ub(outer.r, outer.g, outer.b, outer.a);
            rlVertex2f((float)centerX + cosf(DEG2RAD*i)*radius, (float)centerY + sinf(DEG2RAD*i)*radius);
        }
    rlEnd();
}

// Draw circle outline
void RLDrawCircleLines(int centerX, int centerY, float radius, RLColor color)
{
    RLDrawCircleLinesV((RLVector2){ (float)centerX, (float)centerY }, radius, color);
}

// Draw circle outline (Vector version)
void RLDrawCircleLinesV(RLVector2 center, float radius, RLColor color)
{
    rlBegin(RL_LINES);
        rlColor4ub(color.r, color.g, color.b, color.a);

        // NOTE: Circle outline is drawn pixel by pixel every degree (0 to 360)
        for (int i = 0; i < 360; i += 10)
        {
            rlVertex2f(center.x + cosf(DEG2RAD*i)*radius, center.y + sinf(DEG2RAD*i)*radius);
            rlVertex2f(center.x + cosf(DEG2RAD*(i + 10))*radius, center.y + sinf(DEG2RAD*(i + 10))*radius);
        }
    rlEnd();
}

// Draw ellipse
void RLDrawEllipse(int centerX, int centerY, float radiusH, float radiusV, RLColor color)
{
    RLDrawEllipseV((RLVector2){ (float)centerX, (float)centerY }, radiusH, radiusV, color);
}

// Draw ellipse (Vector version)
void RLDrawEllipseV(RLVector2 center, float radiusH, float radiusV, RLColor color)
{
    rlBegin(RL_TRIANGLES);
        for (int i = 0; i < 360; i += 10)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);
            rlVertex2f(center.x,  center.y);
            rlVertex2f(center.x + cosf(DEG2RAD*(i + 10))*radiusH, center.y + sinf(DEG2RAD*(i + 10))*radiusV);
            rlVertex2f(center.x + cosf(DEG2RAD*i)*radiusH, center.y + sinf(DEG2RAD*i)*radiusV);
        }
    rlEnd();
}

// Draw ellipse outline
void RLDrawEllipseLines(int centerX, int centerY, float radiusH, float radiusV, RLColor color)
{
    RLDrawEllipseLinesV((RLVector2){ (float)centerX, (float)centerY }, radiusH, radiusV, color);
}

// Draw ellipse outline
void RLDrawEllipseLinesV(RLVector2 center, float radiusH, float radiusV, RLColor color)
{
    rlBegin(RL_LINES);
        for (int i = 0; i < 360; i += 10)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);
            rlVertex2f(center.x + cosf(DEG2RAD*(i + 10))*radiusH, center.y + sinf(DEG2RAD*(i + 10))*radiusV);
            rlVertex2f(center.x + cosf(DEG2RAD*i)*radiusH, center.y + sinf(DEG2RAD*i)*radiusV);
        }
    rlEnd();
}

// Draw ring
void RLDrawRing(RLVector2 center, float innerRadius, float outerRadius, float startAngle, float endAngle, int segments, RLColor color)
{
    if (startAngle == endAngle) return;

    // Function expects (outerRadius > innerRadius)
    if (outerRadius < innerRadius)
    {
        float tmp = outerRadius;
        outerRadius = innerRadius;
        innerRadius = tmp;

        if (outerRadius <= 0.0f) outerRadius = 0.1f;
    }

    // Function expects (endAngle > startAngle)
    if (endAngle < startAngle)
    {
        // Swap values
        float tmp = startAngle;
        startAngle = endAngle;
        endAngle = tmp;
    }

    int minSegments = (int)ceilf((endAngle - startAngle)/90);

    if (segments < minSegments)
    {
        // Calculate the maximum angle between segments based on the error rate (usually 0.5f)
        float th = acosf(2*powf(1 - SMOOTH_CIRCLE_ERROR_RATE/outerRadius, 2) - 1);
        segments = (int)((endAngle - startAngle)*ceilf(2*PI/th)/360);

        if (segments <= 0) segments = minSegments;
    }

    // Not a ring
    if (innerRadius <= 0.0f)
    {
        RLDrawCircleSector(center, outerRadius, startAngle, endAngle, segments, color);
        return;
    }

    float stepLength = (endAngle - startAngle)/(float)segments;
    float angle = startAngle;

#if defined(SUPPORT_QUADS_DRAW_MODE)
    rlSetTexture(RLGetShapesTexture().id);
    RLRectangle shapeRect = RLGetShapesTextureRectangle();

    rlBegin(RL_QUADS);
        for (int i = 0; i < segments; i++)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);

            rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(center.x + cosf(DEG2RAD*angle)*outerRadius, center.y + sinf(DEG2RAD*angle)*outerRadius);

            rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(center.x + cosf(DEG2RAD*angle)*innerRadius, center.y + sinf(DEG2RAD*angle)*innerRadius);

            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*innerRadius, center.y + sinf(DEG2RAD*(angle + stepLength))*innerRadius);

            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*outerRadius, center.y + sinf(DEG2RAD*(angle + stepLength))*outerRadius);

            angle += stepLength;
        }
    rlEnd();

    rlSetTexture(0);
#else
    rlBegin(RL_TRIANGLES);
        for (int i = 0; i < segments; i++)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);

            rlVertex2f(center.x + cosf(DEG2RAD*angle)*innerRadius, center.y + sinf(DEG2RAD*angle)*innerRadius);
            rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*innerRadius, center.y + sinf(DEG2RAD*(angle + stepLength))*innerRadius);
            rlVertex2f(center.x + cosf(DEG2RAD*angle)*outerRadius, center.y + sinf(DEG2RAD*angle)*outerRadius);

            rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*innerRadius, center.y + sinf(DEG2RAD*(angle + stepLength))*innerRadius);
            rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*outerRadius, center.y + sinf(DEG2RAD*(angle + stepLength))*outerRadius);
            rlVertex2f(center.x + cosf(DEG2RAD*angle)*outerRadius, center.y + sinf(DEG2RAD*angle)*outerRadius);

            angle += stepLength;
        }
    rlEnd();
#endif
}

// Draw ring outline
void RLDrawRingLines(RLVector2 center, float innerRadius, float outerRadius, float startAngle, float endAngle, int segments, RLColor color)
{
    if (startAngle == endAngle) return;

    // Function expects (outerRadius > innerRadius)
    if (outerRadius < innerRadius)
    {
        float tmp = outerRadius;
        outerRadius = innerRadius;
        innerRadius = tmp;

        if (outerRadius <= 0.0f) outerRadius = 0.1f;
    }

    // Function expects (endAngle > startAngle)
    if (endAngle < startAngle)
    {
        // Swap values
        float tmp = startAngle;
        startAngle = endAngle;
        endAngle = tmp;
    }

    int minSegments = (int)ceilf((endAngle - startAngle)/90);

    if (segments < minSegments)
    {
        // Calculate the maximum angle between segments based on the error rate (usually 0.5f)
        float th = acosf(2*powf(1 - SMOOTH_CIRCLE_ERROR_RATE/outerRadius, 2) - 1);
        segments = (int)((endAngle - startAngle)*ceilf(2*PI/th)/360);

        if (segments <= 0) segments = minSegments;
    }

    if (innerRadius <= 0.0f)
    {
        RLDrawCircleSectorLines(center, outerRadius, startAngle, endAngle, segments, color);
        return;
    }

    float stepLength = (endAngle - startAngle)/(float)segments;
    float angle = startAngle;
    bool showCapLines = true;

    rlBegin(RL_LINES);
        if (showCapLines)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);
            rlVertex2f(center.x + cosf(DEG2RAD*angle)*outerRadius, center.y + sinf(DEG2RAD*angle)*outerRadius);
            rlVertex2f(center.x + cosf(DEG2RAD*angle)*innerRadius, center.y + sinf(DEG2RAD*angle)*innerRadius);
        }

        for (int i = 0; i < segments; i++)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);

            rlVertex2f(center.x + cosf(DEG2RAD*angle)*outerRadius, center.y + sinf(DEG2RAD*angle)*outerRadius);
            rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*outerRadius, center.y + sinf(DEG2RAD*(angle + stepLength))*outerRadius);

            rlVertex2f(center.x + cosf(DEG2RAD*angle)*innerRadius, center.y + sinf(DEG2RAD*angle)*innerRadius);
            rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*innerRadius, center.y + sinf(DEG2RAD*(angle + stepLength))*innerRadius);

            angle += stepLength;
        }

        if (showCapLines)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);
            rlVertex2f(center.x + cosf(DEG2RAD*angle)*outerRadius, center.y + sinf(DEG2RAD*angle)*outerRadius);
            rlVertex2f(center.x + cosf(DEG2RAD*angle)*innerRadius, center.y + sinf(DEG2RAD*angle)*innerRadius);
        }
    rlEnd();
}

// Draw a color-filled rectangle
void RLDrawRectangle(int posX, int posY, int width, int height, RLColor color)
{
    RLDrawRectangleV((RLVector2){ (float)posX, (float)posY }, (RLVector2){ (float)width, (float)height }, color);
}

// Draw a color-filled rectangle (Vector version)
// NOTE: On OpenGL 3.3 and ES2 using QUADS to avoid drawing order issues
void RLDrawRectangleV(RLVector2 position, RLVector2 size, RLColor color)
{
    RLDrawRectanglePro((RLRectangle){ position.x, position.y, size.x, size.y }, (RLVector2){ 0.0f, 0.0f }, 0.0f, color);
}

// Draw a color-filled rectangle
void RLDrawRectangleRec(RLRectangle rec, RLColor color)
{
    RLDrawRectanglePro(rec, (RLVector2){ 0.0f, 0.0f }, 0.0f, color);
}

// Draw a color-filled rectangle with pro parameters
void RLDrawRectanglePro(RLRectangle rec, RLVector2 origin, float rotation, RLColor color)
{
    RLVector2 topLeft = { 0 };
    RLVector2 topRight = { 0 };
    RLVector2 bottomLeft = { 0 };
    RLVector2 bottomRight = { 0 };

    // Only calculate rotation if needed
    if (rotation == 0.0f)
    {
        float x = rec.x - origin.x;
        float y = rec.y - origin.y;
        topLeft = (RLVector2){ x, y };
        topRight = (RLVector2){ x + rec.width, y };
        bottomLeft = (RLVector2){ x, y + rec.height };
        bottomRight = (RLVector2){ x + rec.width, y + rec.height };
    }
    else
    {
        float sinRotation = sinf(rotation*DEG2RAD);
        float cosRotation = cosf(rotation*DEG2RAD);
        float x = rec.x;
        float y = rec.y;
        float dx = -origin.x;
        float dy = -origin.y;

        topLeft.x = x + dx*cosRotation - dy*sinRotation;
        topLeft.y = y + dx*sinRotation + dy*cosRotation;

        topRight.x = x + (dx + rec.width)*cosRotation - dy*sinRotation;
        topRight.y = y + (dx + rec.width)*sinRotation + dy*cosRotation;

        bottomLeft.x = x + dx*cosRotation - (dy + rec.height)*sinRotation;
        bottomLeft.y = y + dx*sinRotation + (dy + rec.height)*cosRotation;

        bottomRight.x = x + (dx + rec.width)*cosRotation - (dy + rec.height)*sinRotation;
        bottomRight.y = y + (dx + rec.width)*sinRotation + (dy + rec.height)*cosRotation;
    }

#if defined(SUPPORT_QUADS_DRAW_MODE)
    rlSetTexture(RLGetShapesTexture().id);
    RLRectangle shapeRect = RLGetShapesTextureRectangle();

    rlBegin(RL_QUADS);

        rlNormal3f(0.0f, 0.0f, 1.0f);
        rlColor4ub(color.r, color.g, color.b, color.a);

        rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(topLeft.x, topLeft.y);

        rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(bottomLeft.x, bottomLeft.y);

        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(bottomRight.x, bottomRight.y);

        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(topRight.x, topRight.y);

    rlEnd();

    rlSetTexture(0);
#else
    rlBegin(RL_TRIANGLES);

        rlColor4ub(color.r, color.g, color.b, color.a);

        rlVertex2f(topLeft.x, topLeft.y);
        rlVertex2f(bottomLeft.x, bottomLeft.y);
        rlVertex2f(topRight.x, topRight.y);

        rlVertex2f(topRight.x, topRight.y);
        rlVertex2f(bottomLeft.x, bottomLeft.y);
        rlVertex2f(bottomRight.x, bottomRight.y);

    rlEnd();
#endif
}

// Draw a vertical-gradient-filled rectangle
void RLDrawRectangleGradientV(int posX, int posY, int width, int height, RLColor top, RLColor bottom)
{
    RLDrawRectangleGradientEx((RLRectangle){ (float)posX, (float)posY, (float)width, (float)height }, top, bottom, bottom, top);
}

// Draw a horizontal-gradient-filled rectangle
void RLDrawRectangleGradientH(int posX, int posY, int width, int height, RLColor left, RLColor right)
{
    RLDrawRectangleGradientEx((RLRectangle){ (float)posX, (float)posY, (float)width, (float)height }, left, left, right, right);
}

// Draw a gradient-filled rectangle
void RLDrawRectangleGradientEx(RLRectangle rec, RLColor topLeft, RLColor bottomLeft, RLColor bottomRight, RLColor topRight)
{
    rlSetTexture(RLGetShapesTexture().id);
    RLRectangle shapeRect = RLGetShapesTextureRectangle();

    rlBegin(RL_QUADS);
        rlNormal3f(0.0f, 0.0f, 1.0f);

        // NOTE: Default raylib font character 95 is a white square
        rlColor4ub(topLeft.r, topLeft.g, topLeft.b, topLeft.a);
        rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(rec.x, rec.y);

        rlColor4ub(bottomLeft.r, bottomLeft.g, bottomLeft.b, bottomLeft.a);
        rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(rec.x, rec.y + rec.height);

        rlColor4ub(bottomRight.r, bottomRight.g, bottomRight.b, bottomRight.a);
        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(rec.x + rec.width, rec.y + rec.height);

        rlColor4ub(topRight.r, topRight.g, topRight.b, topRight.a);
        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(rec.x + rec.width, rec.y);
    rlEnd();

    rlSetTexture(0);
}

// Draw rectangle outline
// WARNING: All Draw*Lines() functions use RL_LINES for drawing,
// it implies flushing the current batch and changing draw mode to RL_LINES
// but it solves another issue: https://github.com/raysan5/raylib/issues/3884
void RLDrawRectangleLines(int posX, int posY, int width, int height, RLColor color)
{
    RLMatrix mat = rlGetMatrixTransform();
    float xOffset = 0.5f/mat.m0;
    float yOffset = 0.5f/mat.m5;

    rlBegin(RL_LINES);
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex2f((float)posX + xOffset, (float)posY + yOffset);
        rlVertex2f((float)posX + (float)width - xOffset, (float)posY + yOffset);

        rlVertex2f((float)posX + (float)width - xOffset, (float)posY + yOffset);
        rlVertex2f((float)posX + (float)width - xOffset, (float)posY + (float)height - yOffset);

        rlVertex2f((float)posX + (float)width - xOffset, (float)posY + (float)height - yOffset);
        rlVertex2f((float)posX + xOffset, (float)posY + (float)height - yOffset);

        rlVertex2f((float)posX + xOffset, (float)posY + (float)height - yOffset);
        rlVertex2f((float)posX + xOffset, (float)posY + yOffset);
    rlEnd();

/*
// Previous implementation, it has issues... but it does not require view matrix...
#if defined(SUPPORT_QUADS_DRAW_MODE)
    DrawRectangle(posX, posY, width, 1, color);
    DrawRectangle(posX + width - 1, posY + 1, 1, height - 2, color);
    DrawRectangle(posX, posY + height - 1, width, 1, color);
    DrawRectangle(posX, posY + 1, 1, height - 2, color);
#else
    rlBegin(RL_LINES);
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex2f((float)posX, (float)posY);
        rlVertex2f((float)posX + (float)width, (float)posY + 1);

        rlVertex2f((float)posX + (float)width, (float)posY + 1);
        rlVertex2f((float)posX + (float)width, (float)posY + (float)height);

        rlVertex2f((float)posX + (float)width, (float)posY + (float)height);
        rlVertex2f((float)posX + 1, (float)posY + (float)height);

        rlVertex2f((float)posX + 1, (float)posY + (float)height);
        rlVertex2f((float)posX + 1, (float)posY + 1);
    rlEnd();
#endif
*/
}

// Draw rectangle outline with extended parameters
void RLDrawRectangleLinesEx(RLRectangle rec, float lineThick, RLColor color)
{
    if ((lineThick > rec.width) || (lineThick > rec.height))
    {
        if (rec.width >= rec.height) lineThick = rec.height/2;
        else if (rec.width <= rec.height) lineThick = rec.width/2;
    }

    // When rec = { x, y, 8.0f, 6.0f } and lineThick = 2, the following
    // four rectangles are drawn ([T]op, [B]ottom, [L]eft, [R]ight):
    //
    //   TTTTTTTT
    //   TTTTTTTT
    //   LL    RR
    //   LL    RR
    //   BBBBBBBB
    //   BBBBBBBB
    //

    RLRectangle top = { rec.x, rec.y, rec.width, lineThick };
    RLRectangle bottom = { rec.x, rec.y - lineThick + rec.height, rec.width, lineThick };
    RLRectangle left = { rec.x, rec.y + lineThick, lineThick, rec.height - lineThick*2.0f };
    RLRectangle right = { rec.x - lineThick + rec.width, rec.y + lineThick, lineThick, rec.height - lineThick*2.0f };

    RLDrawRectangleRec(top, color);
    RLDrawRectangleRec(bottom, color);
    RLDrawRectangleRec(left, color);
    RLDrawRectangleRec(right, color);
}

// Draw rectangle with rounded edges
void RLDrawRectangleRounded(RLRectangle rec, float roundness, int segments, RLColor color)
{
    // Not a rounded rectangle
    if (roundness <= 0.0f)
    {
        RLDrawRectangleRec(rec, color);
        return;
    }

    if (roundness >= 1.0f) roundness = 1.0f;

    // Calculate corner radius
    float radius = (rec.width > rec.height)? (rec.height*roundness)/2 : (rec.width*roundness)/2;
    if (radius <= 0.0f) return;

    // Calculate number of segments to use for the corners
    if (segments < 4)
    {
        // Calculate the maximum angle between segments based on the error rate (usually 0.5f)
        float th = acosf(2*powf(1 - SMOOTH_CIRCLE_ERROR_RATE/radius, 2) - 1);
        segments = (int)(ceilf(2*PI/th)/4.0f);
        if (segments <= 0) segments = 4;
    }

    float stepLength = 90.0f/(float)segments;

    /*
    Quick sketch to make sense of all of this,
    there are 9 parts to draw, also mark the 12 points used

          P0____________________P1
          /|                    |\
         /1|          2         |3\
     P7 /__|____________________|__\ P2
       |   |P8                P9|   |
       | 8 |          9         | 4 |
       | __|____________________|__ |
     P6 \  |P11              P10|  / P3
         \7|          6         |5/
          \|____________________|/
          P5                    P4
    */
    // Coordinates of the 12 points that define the rounded rect
    const RLVector2 point[12] = {
        {(float)rec.x + radius, rec.y}, {(float)(rec.x + rec.width) - radius, rec.y}, { rec.x + rec.width, (float)rec.y + radius },     // PO, P1, P2
        {rec.x + rec.width, (float)(rec.y + rec.height) - radius}, {(float)(rec.x + rec.width) - radius, rec.y + rec.height},           // P3, P4
        {(float)rec.x + radius, rec.y + rec.height}, { rec.x, (float)(rec.y + rec.height) - radius}, {rec.x, (float)rec.y + radius},    // P5, P6, P7
        {(float)rec.x + radius, (float)rec.y + radius}, {(float)(rec.x + rec.width) - radius, (float)rec.y + radius},                   // P8, P9
        {(float)(rec.x + rec.width) - radius, (float)(rec.y + rec.height) - radius}, {(float)rec.x + radius, (float)(rec.y + rec.height) - radius} // P10, P11
    };

    const RLVector2 centers[4] = { point[8], point[9], point[10], point[11] };
    const float angles[4] = { 180.0f, 270.0f, 0.0f, 90.0f };

#if defined(SUPPORT_QUADS_DRAW_MODE)
    rlSetTexture(RLGetShapesTexture().id);
    RLRectangle shapeRect = RLGetShapesTextureRectangle();

    rlBegin(RL_QUADS);
        // Draw all the 4 corners: [1] Upper Left Corner, [3] Upper Right Corner, [5] Lower Right Corner, [7] Lower Left Corner
        for (int k = 0; k < 4; ++k) // Hope the compiler is smart enough to unroll this loop
        {
            float angle = angles[k];
            const RLVector2 center = centers[k];

            // NOTE: Every QUAD actually represents two segments
            for (int i = 0; i < segments/2; i++)
            {
                rlColor4ub(color.r, color.g, color.b, color.a);
                rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
                rlVertex2f(center.x, center.y);

                rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
                rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength*2))*radius, center.y + sinf(DEG2RAD*(angle + stepLength*2))*radius);

                rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
                rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*radius, center.y + sinf(DEG2RAD*(angle + stepLength))*radius);

                rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
                rlVertex2f(center.x + cosf(DEG2RAD*angle)*radius, center.y + sinf(DEG2RAD*angle)*radius);

                angle += (stepLength*2);
            }

            // NOTE: In case number of segments is odd, adding one last piece to the cake
            if (segments%2)
            {
                rlColor4ub(color.r, color.g, color.b, color.a);
                rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
                rlVertex2f(center.x, center.y);

                rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
                rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*radius, center.y + sinf(DEG2RAD*(angle + stepLength))*radius);

                rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
                rlVertex2f(center.x + cosf(DEG2RAD*angle)*radius, center.y + sinf(DEG2RAD*angle)*radius);

                rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
                rlVertex2f(center.x, center.y);
            }
        }

        // [2] Upper Rectangle
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(point[0].x, point[0].y);
        rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(point[8].x, point[8].y);
        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(point[9].x, point[9].y);
        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(point[1].x, point[1].y);

        // [4] Right Rectangle
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(point[2].x, point[2].y);
        rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(point[9].x, point[9].y);
        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(point[10].x, point[10].y);
        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(point[3].x, point[3].y);

        // [6] Bottom Rectangle
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(point[11].x, point[11].y);
        rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(point[5].x, point[5].y);
        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(point[4].x, point[4].y);
        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(point[10].x, point[10].y);

        // [8] Left Rectangle
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(point[7].x, point[7].y);
        rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(point[6].x, point[6].y);
        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(point[11].x, point[11].y);
        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(point[8].x, point[8].y);

        // [9] Middle Rectangle
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(point[8].x, point[8].y);
        rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(point[11].x, point[11].y);
        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(point[10].x, point[10].y);
        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(point[9].x, point[9].y);

    rlEnd();
    rlSetTexture(0);
#else
    rlBegin(RL_TRIANGLES);

        // Draw all of the 4 corners: [1] Upper Left Corner, [3] Upper Right Corner, [5] Lower Right Corner, [7] Lower Left Corner
        for (int k = 0; k < 4; ++k) // Hope the compiler is smart enough to unroll this loop
        {
            float angle = angles[k];
            const RLVector2 center = centers[k];
            for (int i = 0; i < segments; i++)
            {
                rlColor4ub(color.r, color.g, color.b, color.a);
                rlVertex2f(center.x, center.y);
                rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*radius, center.y + sinf(DEG2RAD*(angle + stepLength))*radius);
                rlVertex2f(center.x + cosf(DEG2RAD*angle)*radius, center.y + sinf(DEG2RAD*angle)*radius);
                angle += stepLength;
            }
        }

        // [2] Upper Rectangle
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex2f(point[0].x, point[0].y);
        rlVertex2f(point[8].x, point[8].y);
        rlVertex2f(point[9].x, point[9].y);
        rlVertex2f(point[1].x, point[1].y);
        rlVertex2f(point[0].x, point[0].y);
        rlVertex2f(point[9].x, point[9].y);

        // [4] Right Rectangle
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex2f(point[9].x, point[9].y);
        rlVertex2f(point[10].x, point[10].y);
        rlVertex2f(point[3].x, point[3].y);
        rlVertex2f(point[2].x, point[2].y);
        rlVertex2f(point[9].x, point[9].y);
        rlVertex2f(point[3].x, point[3].y);

        // [6] Bottom Rectangle
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex2f(point[11].x, point[11].y);
        rlVertex2f(point[5].x, point[5].y);
        rlVertex2f(point[4].x, point[4].y);
        rlVertex2f(point[10].x, point[10].y);
        rlVertex2f(point[11].x, point[11].y);
        rlVertex2f(point[4].x, point[4].y);

        // [8] Left Rectangle
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex2f(point[7].x, point[7].y);
        rlVertex2f(point[6].x, point[6].y);
        rlVertex2f(point[11].x, point[11].y);
        rlVertex2f(point[8].x, point[8].y);
        rlVertex2f(point[7].x, point[7].y);
        rlVertex2f(point[11].x, point[11].y);

        // [9] Middle Rectangle
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex2f(point[8].x, point[8].y);
        rlVertex2f(point[11].x, point[11].y);
        rlVertex2f(point[10].x, point[10].y);
        rlVertex2f(point[9].x, point[9].y);
        rlVertex2f(point[8].x, point[8].y);
        rlVertex2f(point[10].x, point[10].y);
    rlEnd();
#endif
}

// Draw rectangle with rounded edges
void RLDrawRectangleRoundedLines(RLRectangle rec, float roundness, int segments, RLColor color)
{
    // NOTE: For line thicknes <=1.0f using RL_LINES, otherwise using RL_QUADS/RL_TRIANGLES
    RLDrawRectangleRoundedLinesEx(rec, roundness, segments, 1.0f, color);
}

// Draw rectangle with rounded edges outline
void RLDrawRectangleRoundedLinesEx(RLRectangle rec, float roundness, int segments, float lineThick, RLColor color)
{
    if (lineThick < 0) lineThick = 0;

    // Not a rounded rectangle
    if (roundness <= 0.0f)
    {
        RLDrawRectangleLinesEx((RLRectangle){rec.x-lineThick, rec.y-lineThick, rec.width+2*lineThick, rec.height+2*lineThick}, lineThick, color);
        return;
    }

    if (roundness >= 1.0f) roundness = 1.0f;

    // Calculate corner radius
    float radius = (rec.width > rec.height)? (rec.height*roundness)/2 : (rec.width*roundness)/2;
    if (radius <= 0.0f) return;

    // Calculate number of segments to use for the corners
    if (segments < 4)
    {
        // Calculate the maximum angle between segments based on the error rate (usually 0.5f)
        float th = acosf(2*powf(1 - SMOOTH_CIRCLE_ERROR_RATE/radius, 2) - 1);
        segments = (int)(ceilf(2*PI/th)/2.0f);
        if (segments <= 0) segments = 4;
    }

    float stepLength = 90.0f/(float)segments;
    const float outerRadius = radius + lineThick, innerRadius = radius;

    /*
    Quick sketch to make sense of all of this,
    marks the 16 + 4(corner centers P16-19) points used

           P0 ================== P1
          // P8                P9 \\
         //                        \\
     P7 // P15                  P10 \\ P2
       ||   *P16             P17*    ||
       ||                            ||
       || P14                   P11  ||
     P6 \\  *P19             P18*   // P3
         \\                        //
          \\ P13              P12 //
           P5 ================== P4
    */
    const RLVector2 point[16] = {
        {(float)rec.x + innerRadius + 0.5f, rec.y - lineThick + 0.5f},
        {(float)(rec.x + rec.width) - innerRadius - 0.5f, rec.y - lineThick + 0.5f},
        {rec.x + rec.width + lineThick - 0.5f, (float)rec.y + innerRadius + 0.5f}, // PO, P1, P2
        {rec.x + rec.width + lineThick - 0.5f, (float)(rec.y + rec.height) - innerRadius - 0.5f},
        {(float)(rec.x + rec.width) - innerRadius - 0.5f, rec.y + rec.height + lineThick - 0.5f}, // P3, P4
        {(float)rec.x + innerRadius + 0.5f, rec.y + rec.height + lineThick - 0.5f},
        {rec.x - lineThick + 0.5f, (float)(rec.y + rec.height) - innerRadius - 0.5f},
        {rec.x - lineThick + 0.5f, (float)rec.y + innerRadius + 0.5f}, // P5, P6, P7
        {(float)rec.x + innerRadius + 0.5f, rec.y + 0.5f},
        {(float)(rec.x + rec.width) - innerRadius - 0.5f, rec.y + 0.5f}, // P8, P9
        {rec.x + rec.width - 0.5f, (float)rec.y + innerRadius + 0.5f},
        {rec.x + rec.width - 0.5f, (float)(rec.y + rec.height) - innerRadius - 0.5f}, // P10, P11
        {(float)(rec.x + rec.width) - innerRadius - 0.5f, rec.y + rec.height - 0.5f},
        {(float)rec.x + innerRadius + 0.5f, rec.y + rec.height - 0.5f}, // P12, P13
        {rec.x + 0.5f, (float)(rec.y + rec.height) - innerRadius - 0.5f},
        {rec.x + 0.5f, (float)rec.y + innerRadius + 0.5f} // P14, P15
    };

    const RLVector2 centers[4] = {
        {(float)rec.x + innerRadius + 0.5f, (float)rec.y + innerRadius + 0.5f},
        {(float)(rec.x + rec.width) - innerRadius - 0.5f, (float)rec.y + innerRadius + 0.5f}, // P16, P17
        {(float)(rec.x + rec.width) - innerRadius - 0.5f, (float)(rec.y + rec.height) - innerRadius - 0.5f},
        {(float)rec.x + innerRadius + 0.5f, (float)(rec.y + rec.height) - innerRadius - 0.5f} // P18, P19
    };

    const float angles[4] = { 180.0f, 270.0f, 0.0f, 90.0f };

    if (lineThick > 1)
    {
#if defined(SUPPORT_QUADS_DRAW_MODE)
        rlSetTexture(RLGetShapesTexture().id);
        RLRectangle shapeRect = RLGetShapesTextureRectangle();

        rlBegin(RL_QUADS);

            // Draw all the 4 corners first: Upper Left Corner, Upper Right Corner, Lower Right Corner, Lower Left Corner
            for (int k = 0; k < 4; ++k) // Hope the compiler is smart enough to unroll this loop
            {
                float angle = angles[k];
                const RLVector2 center = centers[k];
                for (int i = 0; i < segments; i++)
                {
                    rlColor4ub(color.r, color.g, color.b, color.a);

                    rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
                    rlVertex2f(center.x + cosf(DEG2RAD*angle)*innerRadius, center.y + sinf(DEG2RAD*angle)*innerRadius);

                    rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
                    rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*innerRadius, center.y + sinf(DEG2RAD*(angle + stepLength))*innerRadius);

                    rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
                    rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*outerRadius, center.y + sinf(DEG2RAD*(angle + stepLength))*outerRadius);

                    rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
                    rlVertex2f(center.x + cosf(DEG2RAD*angle)*outerRadius, center.y + sinf(DEG2RAD*angle)*outerRadius);

                    angle += stepLength;
                }
            }

            // Upper rectangle
            rlColor4ub(color.r, color.g, color.b, color.a);
            rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(point[0].x, point[0].y);
            rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(point[8].x, point[8].y);
            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(point[9].x, point[9].y);
            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(point[1].x, point[1].y);

            // Right rectangle
            rlColor4ub(color.r, color.g, color.b, color.a);
            rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(point[2].x, point[2].y);
            rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(point[10].x, point[10].y);
            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(point[11].x, point[11].y);
            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(point[3].x, point[3].y);

            // Lower rectangle
            rlColor4ub(color.r, color.g, color.b, color.a);
            rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(point[13].x, point[13].y);
            rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(point[5].x, point[5].y);
            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(point[4].x, point[4].y);
            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(point[12].x, point[12].y);

            // Left rectangle
            rlColor4ub(color.r, color.g, color.b, color.a);
            rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(point[15].x, point[15].y);
            rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(point[7].x, point[7].y);
            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(point[6].x, point[6].y);
            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(point[14].x, point[14].y);

        rlEnd();
        rlSetTexture(0);
#else
        rlBegin(RL_TRIANGLES);

            // Draw all of the 4 corners first: Upper Left Corner, Upper Right Corner, Lower Right Corner, Lower Left Corner
            for (int k = 0; k < 4; ++k) // Hope the compiler is smart enough to unroll this loop
            {
                float angle = angles[k];
                const RLVector2 center = centers[k];

                for (int i = 0; i < segments; i++)
                {
                    rlColor4ub(color.r, color.g, color.b, color.a);

                    rlVertex2f(center.x + cosf(DEG2RAD*angle)*innerRadius, center.y + sinf(DEG2RAD*angle)*innerRadius);
                    rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*innerRadius, center.y + sinf(DEG2RAD*(angle + stepLength))*innerRadius);
                    rlVertex2f(center.x + cosf(DEG2RAD*angle)*outerRadius, center.y + sinf(DEG2RAD*angle)*outerRadius);

                    rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*innerRadius, center.y + sinf(DEG2RAD*(angle + stepLength))*innerRadius);
                    rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*outerRadius, center.y + sinf(DEG2RAD*(angle + stepLength))*outerRadius);
                    rlVertex2f(center.x + cosf(DEG2RAD*angle)*outerRadius, center.y + sinf(DEG2RAD*angle)*outerRadius);

                    angle += stepLength;
                }
            }

            // Upper rectangle
            rlColor4ub(color.r, color.g, color.b, color.a);
            rlVertex2f(point[0].x, point[0].y);
            rlVertex2f(point[8].x, point[8].y);
            rlVertex2f(point[9].x, point[9].y);
            rlVertex2f(point[1].x, point[1].y);
            rlVertex2f(point[0].x, point[0].y);
            rlVertex2f(point[9].x, point[9].y);

            // Right rectangle
            rlColor4ub(color.r, color.g, color.b, color.a);
            rlVertex2f(point[10].x, point[10].y);
            rlVertex2f(point[11].x, point[11].y);
            rlVertex2f(point[3].x, point[3].y);
            rlVertex2f(point[2].x, point[2].y);
            rlVertex2f(point[10].x, point[10].y);
            rlVertex2f(point[3].x, point[3].y);

            // Lower rectangle
            rlColor4ub(color.r, color.g, color.b, color.a);
            rlVertex2f(point[13].x, point[13].y);
            rlVertex2f(point[5].x, point[5].y);
            rlVertex2f(point[4].x, point[4].y);
            rlVertex2f(point[12].x, point[12].y);
            rlVertex2f(point[13].x, point[13].y);
            rlVertex2f(point[4].x, point[4].y);

            // Left rectangle
            rlColor4ub(color.r, color.g, color.b, color.a);
            rlVertex2f(point[7].x, point[7].y);
            rlVertex2f(point[6].x, point[6].y);
            rlVertex2f(point[14].x, point[14].y);
            rlVertex2f(point[15].x, point[15].y);
            rlVertex2f(point[7].x, point[7].y);
            rlVertex2f(point[14].x, point[14].y);
        rlEnd();
#endif
    }
    else
    {
        // Use LINES to draw the outline
        rlBegin(RL_LINES);
            // Draw all the 4 corners first: Upper Left Corner, Upper Right Corner, Lower Right Corner, Lower Left Corner
            for (int k = 0; k < 4; ++k) // Hope the compiler is smart enough to unroll this loop
            {
                float angle = angles[k];
                const RLVector2 center = centers[k];

                for (int i = 0; i < segments; i++)
                {
                    rlColor4ub(color.r, color.g, color.b, color.a);
                    rlVertex2f(center.x + cosf(DEG2RAD*angle)*outerRadius, center.y + sinf(DEG2RAD*angle)*outerRadius);
                    rlVertex2f(center.x + cosf(DEG2RAD*(angle + stepLength))*outerRadius, center.y + sinf(DEG2RAD*(angle + stepLength))*outerRadius);
                    angle += stepLength;
                }
            }

            // And now the remaining 4 lines
            for (int i = 0; i < 8; i += 2)
            {
                rlColor4ub(color.r, color.g, color.b, color.a);
                rlVertex2f(point[i].x, point[i].y);
                rlVertex2f(point[i + 1].x, point[i + 1].y);
            }
        rlEnd();
    }
}

// Draw a triangle
// NOTE: Vertex must be provided in counter-clockwise order
void RLDrawTriangle(RLVector2 v1, RLVector2 v2, RLVector2 v3, RLColor color)
{
#if defined(SUPPORT_QUADS_DRAW_MODE)
    rlSetTexture(RLGetShapesTexture().id);
    RLRectangle shapeRect = RLGetShapesTextureRectangle();

    rlBegin(RL_QUADS);
        rlNormal3f(0.0f, 0.0f, 1.0f);
        rlColor4ub(color.r, color.g, color.b, color.a);

        rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(v1.x, v1.y);

        rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(v2.x, v2.y);

        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
        rlVertex2f(v3.x, v3.y);

        rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
        rlVertex2f(v3.x, v3.y);
    rlEnd();

    rlSetTexture(0);
#else
    rlBegin(RL_TRIANGLES);
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex2f(v1.x, v1.y);
        rlVertex2f(v2.x, v2.y);
        rlVertex2f(v3.x, v3.y);
    rlEnd();
#endif
}

// Draw a triangle using lines
// NOTE: Vertex must be provided in counter-clockwise order
void RLDrawTriangleLines(RLVector2 v1, RLVector2 v2, RLVector2 v3, RLColor color)
{
    rlBegin(RL_LINES);
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex2f(v1.x, v1.y);
        rlVertex2f(v2.x, v2.y);

        rlVertex2f(v2.x, v2.y);
        rlVertex2f(v3.x, v3.y);

        rlVertex2f(v3.x, v3.y);
        rlVertex2f(v1.x, v1.y);
    rlEnd();
}

// Draw a triangle fan defined by points
// NOTE: First vertex provided is the center, shared by all triangles
// By default, following vertex should be provided in counter-clockwise order
void RLDrawTriangleFan(const RLVector2 *points, int pointCount, RLColor color)
{
    if (pointCount >= 3)
    {
        rlSetTexture(RLGetShapesTexture().id);
        RLRectangle shapeRect = RLGetShapesTextureRectangle();

        rlBegin(RL_QUADS);
            rlColor4ub(color.r, color.g, color.b, color.a);

            for (int i = 1; i < pointCount - 1; i++)
            {
                rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
                rlVertex2f(points[0].x, points[0].y);

                rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
                rlVertex2f(points[i].x, points[i].y);

                rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
                rlVertex2f(points[i + 1].x, points[i + 1].y);

                rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
                rlVertex2f(points[i + 1].x, points[i + 1].y);
            }
        rlEnd();
        rlSetTexture(0);
    }
}

// Draw a triangle strip defined by points
// NOTE: Every new vertex connects with previous two
void RLDrawTriangleStrip(const RLVector2 *points, int pointCount, RLColor color)
{
    if (pointCount >= 3)
    {
        rlBegin(RL_TRIANGLES);
            rlColor4ub(color.r, color.g, color.b, color.a);

            for (int i = 2; i < pointCount; i++)
            {
                if ((i%2) == 0)
                {
                    rlVertex2f(points[i].x, points[i].y);
                    rlVertex2f(points[i - 2].x, points[i - 2].y);
                    rlVertex2f(points[i - 1].x, points[i - 1].y);
                }
                else
                {
                    rlVertex2f(points[i].x, points[i].y);
                    rlVertex2f(points[i - 1].x, points[i - 1].y);
                    rlVertex2f(points[i - 2].x, points[i - 2].y);
                }
            }
        rlEnd();
    }
}

// Draw a regular polygon of n sides (Vector version)
void RLDrawPoly(RLVector2 center, int sides, float radius, float rotation, RLColor color)
{
    if (sides < 3) sides = 3;
    float centralAngle = rotation*DEG2RAD;
    float angleStep = 360.0f/(float)sides*DEG2RAD;

#if defined(SUPPORT_QUADS_DRAW_MODE)
    rlSetTexture(RLGetShapesTexture().id);
    RLRectangle shapeRect = RLGetShapesTextureRectangle();

    rlBegin(RL_QUADS);
        for (int i = 0; i < sides; i++)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);
            float nextAngle = centralAngle + angleStep;

            rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(center.x, center.y);

            rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(center.x + cosf(centralAngle)*radius, center.y + sinf(centralAngle)*radius);

            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(center.x + cosf(nextAngle)*radius, center.y + sinf(nextAngle)*radius);

            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(center.x + cosf(centralAngle)*radius, center.y + sinf(centralAngle)*radius);

            centralAngle = nextAngle;
        }
    rlEnd();
    rlSetTexture(0);
#else
    rlBegin(RL_TRIANGLES);
        for (int i = 0; i < sides; i++)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);

            rlVertex2f(center.x, center.y);
            rlVertex2f(center.x + cosf(centralAngle + angleStep)*radius, center.y + sinf(centralAngle + angleStep)*radius);
            rlVertex2f(center.x + cosf(centralAngle)*radius, center.y + sinf(centralAngle)*radius);

            centralAngle += angleStep;
        }
    rlEnd();
#endif
}

// Draw a polygon outline of n sides
void RLDrawPolyLines(RLVector2 center, int sides, float radius, float rotation, RLColor color)
{
    if (sides < 3) sides = 3;
    float centralAngle = rotation*DEG2RAD;
    float angleStep = 360.0f/(float)sides*DEG2RAD;

    rlBegin(RL_LINES);
        for (int i = 0; i < sides; i++)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);

            rlVertex2f(center.x + cosf(centralAngle)*radius, center.y + sinf(centralAngle)*radius);
            rlVertex2f(center.x + cosf(centralAngle + angleStep)*radius, center.y + sinf(centralAngle + angleStep)*radius);

            centralAngle += angleStep;
        }
    rlEnd();
}

void RLDrawPolyLinesEx(RLVector2 center, int sides, float radius, float rotation, float lineThick, RLColor color)
{
    if (sides < 3) sides = 3;
    float centralAngle = rotation*DEG2RAD;
    float exteriorAngle = 360.0f/(float)sides*DEG2RAD;
    float innerRadius = radius - (lineThick*cosf(DEG2RAD*exteriorAngle/2.0f));

#if defined(SUPPORT_QUADS_DRAW_MODE)
    rlSetTexture(RLGetShapesTexture().id);
    RLRectangle shapeRect = RLGetShapesTextureRectangle();

    rlBegin(RL_QUADS);
        for (int i = 0; i < sides; i++)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);
            float nextAngle = centralAngle + exteriorAngle;

            rlTexCoord2f(shapeRect.x/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(center.x + cosf(centralAngle)*radius, center.y + sinf(centralAngle)*radius);

            rlTexCoord2f(shapeRect.x/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(center.x + cosf(centralAngle)*innerRadius, center.y + sinf(centralAngle)*innerRadius);

            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, (shapeRect.y + shapeRect.height)/texShapes.height);
            rlVertex2f(center.x + cosf(nextAngle)*innerRadius, center.y + sinf(nextAngle)*innerRadius);

            rlTexCoord2f((shapeRect.x + shapeRect.width)/texShapes.width, shapeRect.y/texShapes.height);
            rlVertex2f(center.x + cosf(nextAngle)*radius, center.y + sinf(nextAngle)*radius);

            centralAngle = nextAngle;
        }
    rlEnd();
    rlSetTexture(0);
#else
    rlBegin(RL_TRIANGLES);
        for (int i = 0; i < sides; i++)
        {
            rlColor4ub(color.r, color.g, color.b, color.a);
            float nextAngle = centralAngle + exteriorAngle;

            rlVertex2f(center.x + cosf(nextAngle)*radius, center.y + sinf(nextAngle)*radius);
            rlVertex2f(center.x + cosf(centralAngle)*radius, center.y + sinf(centralAngle)*radius);
            rlVertex2f(center.x + cosf(centralAngle)*innerRadius, center.y + sinf(centralAngle)*innerRadius);

            rlVertex2f(center.x + cosf(centralAngle)*innerRadius, center.y + sinf(centralAngle)*innerRadius);
            rlVertex2f(center.x + cosf(nextAngle)*innerRadius, center.y + sinf(nextAngle)*innerRadius);
            rlVertex2f(center.x + cosf(nextAngle)*radius, center.y + sinf(nextAngle)*radius);

            centralAngle = nextAngle;
        }
    rlEnd();
#endif
}

//----------------------------------------------------------------------------------
// Module Functions Definition - Splines functions
//----------------------------------------------------------------------------------

// Draw spline: linear, minimum 2 points
void RLDrawSplineLinear(const RLVector2 *points, int pointCount, float thick, RLColor color)
{
    if (pointCount < 2) return;

#if defined(SUPPORT_SPLINE_MITERS)
    RLVector2 prevNormal = (RLVector2){-(points[1].y - points[0].y), (points[1].x - points[0].x)};
    float prevLength = sqrtf(prevNormal.x*prevNormal.x + prevNormal.y*prevNormal.y);

    if (prevLength > 0.0f)
    {
        prevNormal.x /= prevLength;
        prevNormal.y /= prevLength;
    }
    else
    {
        prevNormal.x = 0.0f;
        prevNormal.y = 0.0f;
    }

    RLVector2 prevRadius = { 0.5f*thick*prevNormal.x, 0.5f*thick*prevNormal.y };

    for (int i = 0; i < pointCount - 1; i++)
    {
        RLVector2 normal = { 0 };

        if (i < pointCount - 2)
        {
            normal = (RLVector2){-(points[i + 2].y - points[i + 1].y), (points[i + 2].x - points[i + 1].x)};
            float normalLength = sqrtf(normal.x*normal.x + normal.y*normal.y);

            if (normalLength > 0.0f)
            {
                normal.x /= normalLength;
                normal.y /= normalLength;
            }
            else
            {
                normal.x = 0.0f;
                normal.y = 0.0f;
            }
        }
        else
        {
            normal = prevNormal;
        }

        RLVector2 radius = { prevNormal.x + normal.x, prevNormal.y + normal.y };
        float radiusLength = sqrtf(radius.x*radius.x + radius.y*radius.y);

        if (radiusLength > 0.0f)
        {
            radius.x /= radiusLength;
            radius.y /= radiusLength;
        }
        else
        {
            radius.x = 0.0f;
            radius.y = 0.0f;
        }

        float cosTheta = radius.x*normal.x + radius.y*normal.y;

        if (cosTheta != 0.0f)
        {
            radius.x *= (thick*0.5f/cosTheta);
            radius.y *= (thick*0.5f/cosTheta);
        }
        else
        {
            radius.x = 0.0f;
            radius.y = 0.0f;
        }

        RLVector2 strip[4] = {
            { points[i].x - prevRadius.x, points[i].y - prevRadius.y },
            { points[i].x + prevRadius.x, points[i].y + prevRadius.y },
            { points[i + 1].x - radius.x, points[i + 1].y - radius.y },
            { points[i + 1].x + radius.x, points[i + 1].y + radius.y }
        };

        RLDrawTriangleStrip(strip, 4, color);

        prevRadius = radius;
        prevNormal = normal;
    }

#else   // !SUPPORT_SPLINE_MITERS

    RLVector2 delta = { 0 };
    float length = 0.0f;
    float scale = 0.0f;

    for (int i = 0; i < pointCount - 1; i++)
    {
        delta = (RLVector2){ points[i + 1].x - points[i].x, points[i + 1].y - points[i].y };
        length = sqrtf(delta.x*delta.x + delta.y*delta.y);

        if (length > 0) scale = thick/(2*length);

        RLVector2 radius = { -scale*delta.y, scale*delta.x };
        RLVector2 strip[4] = {
            { points[i].x - radius.x, points[i].y - radius.y },
            { points[i].x + radius.x, points[i].y + radius.y },
            { points[i + 1].x - radius.x, points[i + 1].y - radius.y },
            { points[i + 1].x + radius.x, points[i + 1].y + radius.y }
        };

        RLDrawTriangleStrip(strip, 4, color);
    }
#endif

#if defined(SUPPORT_SPLINE_SEGMENT_CAPS)
    // TODO: Add spline segment rounded caps at the begin/end of the spline
#endif
}

// Draw spline: B-Spline, minimum 4 points
void RLDrawSplineBasis(const RLVector2 *points, int pointCount, float thick, RLColor color)
{
    if (pointCount < 4) return;

    float a[4] = { 0 };
    float b[4] = { 0 };
    float dy = 0.0f;
    float dx = 0.0f;
    float size = 0.0f;

    RLVector2 currentPoint = { 0 };
    RLVector2 nextPoint = { 0 };
    RLVector2 vertices[2*SPLINE_SEGMENT_DIVISIONS + 2] = { 0 };

    for (int i = 0; i < (pointCount - 3); i++)
    {
        float t = 0.0f;
        RLVector2 p1 = points[i], p2 = points[i + 1], p3 = points[i + 2], p4 = points[i + 3];

        a[0] = (-p1.x + 3.0f*p2.x - 3.0f*p3.x + p4.x)/6.0f;
        a[1] = (3.0f*p1.x - 6.0f*p2.x + 3.0f*p3.x)/6.0f;
        a[2] = (-3.0f*p1.x + 3.0f*p3.x)/6.0f;
        a[3] = (p1.x + 4.0f*p2.x + p3.x)/6.0f;

        b[0] = (-p1.y + 3.0f*p2.y - 3.0f*p3.y + p4.y)/6.0f;
        b[1] = (3.0f*p1.y - 6.0f*p2.y + 3.0f*p3.y)/6.0f;
        b[2] = (-3.0f*p1.y + 3.0f*p3.y)/6.0f;
        b[3] = (p1.y + 4.0f*p2.y + p3.y)/6.0f;

        currentPoint.x = a[3];
        currentPoint.y = b[3];

        if (i == 0) RLDrawCircleV(currentPoint, thick/2.0f, color);   // Draw init line circle-cap

        if (i > 0)
        {
            vertices[0].x = currentPoint.x + dy*size;
            vertices[0].y = currentPoint.y - dx*size;
            vertices[1].x = currentPoint.x - dy*size;
            vertices[1].y = currentPoint.y + dx*size;
        }

        for (int j = 1; j <= SPLINE_SEGMENT_DIVISIONS; j++)
        {
            t = ((float)j)/((float)SPLINE_SEGMENT_DIVISIONS);

            nextPoint.x = a[3] + t*(a[2] + t*(a[1] + t*a[0]));
            nextPoint.y = b[3] + t*(b[2] + t*(b[1] + t*b[0]));

            dy = nextPoint.y - currentPoint.y;
            dx = nextPoint.x - currentPoint.x;
            size = 0.5f*thick/sqrtf(dx*dx+dy*dy);

            if ((i == 0) && (j == 1))
            {
                vertices[0].x = currentPoint.x + dy*size;
                vertices[0].y = currentPoint.y - dx*size;
                vertices[1].x = currentPoint.x - dy*size;
                vertices[1].y = currentPoint.y + dx*size;
            }

            vertices[2*j + 1].x = nextPoint.x - dy*size;
            vertices[2*j + 1].y = nextPoint.y + dx*size;
            vertices[2*j].x = nextPoint.x + dy*size;
            vertices[2*j].y = nextPoint.y - dx*size;

            currentPoint = nextPoint;
        }

        RLDrawTriangleStrip(vertices, 2*SPLINE_SEGMENT_DIVISIONS + 2, color);
    }

    // Cap circle drawing at the end of every segment
    RLDrawCircleV(currentPoint, thick/2.0f, color);
}

// Draw spline: Catmull-Rom, minimum 4 points
void RLDrawSplineCatmullRom(const RLVector2 *points, int pointCount, float thick, RLColor color)
{
    if (pointCount < 4) return;

    float dy = 0.0f;
    float dx = 0.0f;
    float size = 0.0f;

    RLVector2 currentPoint = points[1];
    RLVector2 nextPoint = { 0 };
    RLVector2 vertices[2*SPLINE_SEGMENT_DIVISIONS + 2] = { 0 };

    RLDrawCircleV(currentPoint, thick/2.0f, color);   // Draw init line circle-cap

    for (int i = 0; i < (pointCount - 3); i++)
    {
        float t = 0.0f;
        RLVector2 p1 = points[i], p2 = points[i + 1], p3 = points[i + 2], p4 = points[i + 3];

        if (i > 0)
        {
            vertices[0].x = currentPoint.x + dy*size;
            vertices[0].y = currentPoint.y - dx*size;
            vertices[1].x = currentPoint.x - dy*size;
            vertices[1].y = currentPoint.y + dx*size;
        }

        for (int j = 1; j <= SPLINE_SEGMENT_DIVISIONS; j++)
        {
            t = ((float)j)/((float)SPLINE_SEGMENT_DIVISIONS);

            float q0 = (-1.0f*t*t*t) + (2.0f*t*t) + (-1.0f*t);
            float q1 = (3.0f*t*t*t) + (-5.0f*t*t) + 2.0f;
            float q2 = (-3.0f*t*t*t) + (4.0f*t*t) + t;
            float q3 = t*t*t - t*t;

            nextPoint.x = 0.5f*((p1.x*q0) + (p2.x*q1) + (p3.x*q2) + (p4.x*q3));
            nextPoint.y = 0.5f*((p1.y*q0) + (p2.y*q1) + (p3.y*q2) + (p4.y*q3));

            dy = nextPoint.y - currentPoint.y;
            dx = nextPoint.x - currentPoint.x;
            size = (0.5f*thick)/sqrtf(dx*dx + dy*dy);

            if ((i == 0) && (j == 1))
            {
                vertices[0].x = currentPoint.x + dy*size;
                vertices[0].y = currentPoint.y - dx*size;
                vertices[1].x = currentPoint.x - dy*size;
                vertices[1].y = currentPoint.y + dx*size;
            }

            vertices[2*j + 1].x = nextPoint.x - dy*size;
            vertices[2*j + 1].y = nextPoint.y + dx*size;
            vertices[2*j].x = nextPoint.x + dy*size;
            vertices[2*j].y = nextPoint.y - dx*size;

            currentPoint = nextPoint;
        }

        RLDrawTriangleStrip(vertices, 2*SPLINE_SEGMENT_DIVISIONS + 2, color);
    }

    // Cap circle drawing at the end of every segment
    RLDrawCircleV(currentPoint, thick/2.0f, color);
}

// Draw spline: Quadratic Bezier, minimum 3 points (1 control point): [p1, c2, p3, c4...]
void RLDrawSplineBezierQuadratic(const RLVector2 *points, int pointCount, float thick, RLColor color)
{
    if (pointCount >= 3)
    {
        for (int i = 0; i < pointCount - 2; i += 2) RLDrawSplineSegmentBezierQuadratic(points[i], points[i + 1], points[i + 2], thick, color);

        // Cap circle drawing at the end of every segment
        //for (int i = 2; i < pointCount - 2; i += 2) DrawCircleV(points[i], thick/2.0f, color);
    }
}

// Draw spline: Cubic Bezier, minimum 4 points (2 control points): [p1, c2, c3, p4, c5, c6...]
void RLDrawSplineBezierCubic(const RLVector2 *points, int pointCount, float thick, RLColor color)
{
    if (pointCount >= 4)
    {
        for (int i = 0; i < pointCount - 3; i += 3) RLDrawSplineSegmentBezierCubic(points[i], points[i + 1], points[i + 2], points[i + 3], thick, color);

        // Cap circle drawing at the end of every segment
        //for (int i = 3; i < pointCount - 3; i += 3) DrawCircleV(points[i], thick/2.0f, color);
    }
}

// Draw spline segment: Linear, 2 points
void RLDrawSplineSegmentLinear(RLVector2 p1, RLVector2 p2, float thick, RLColor color)
{
    // NOTE: For the linear spline no subdivisions are used, just a single quad

    RLVector2 delta = { p2.x - p1.x, p2.y - p1.y };
    float length = sqrtf(delta.x*delta.x + delta.y*delta.y);

    if ((length > 0) && (thick > 0))
    {
        float scale = thick/(2*length);

        RLVector2 radius = { -scale*delta.y, scale*delta.x };
        RLVector2 strip[4] = {
            { p1.x - radius.x, p1.y - radius.y },
            { p1.x + radius.x, p1.y + radius.y },
            { p2.x - radius.x, p2.y - radius.y },
            { p2.x + radius.x, p2.y + radius.y }
        };

        RLDrawTriangleStrip(strip, 4, color);
    }
}

// Draw spline segment: B-Spline, 4 points
void RLDrawSplineSegmentBasis(RLVector2 p1, RLVector2 p2, RLVector2 p3, RLVector2 p4, float thick, RLColor color)
{
    const float step = 1.0f/SPLINE_SEGMENT_DIVISIONS;

    RLVector2 currentPoint = { 0 };
    RLVector2 nextPoint = { 0 };
    float t = 0.0f;

    RLVector2 points[2*SPLINE_SEGMENT_DIVISIONS + 2] = { 0 };

    float a[4] = { 0 };
    float b[4] = { 0 };

    a[0] = (-p1.x + 3*p2.x - 3*p3.x + p4.x)/6.0f;
    a[1] = (3*p1.x - 6*p2.x + 3*p3.x)/6.0f;
    a[2] = (-3*p1.x + 3*p3.x)/6.0f;
    a[3] = (p1.x + 4*p2.x + p3.x)/6.0f;

    b[0] = (-p1.y + 3*p2.y - 3*p3.y + p4.y)/6.0f;
    b[1] = (3*p1.y - 6*p2.y + 3*p3.y)/6.0f;
    b[2] = (-3*p1.y + 3*p3.y)/6.0f;
    b[3] = (p1.y + 4*p2.y + p3.y)/6.0f;

    currentPoint.x = a[3];
    currentPoint.y = b[3];

    for (int i = 0; i <= SPLINE_SEGMENT_DIVISIONS; i++)
    {
        t = step*(float)i;

        nextPoint.x = a[3] + t*(a[2] + t*(a[1] + t*a[0]));
        nextPoint.y = b[3] + t*(b[2] + t*(b[1] + t*b[0]));

        float dy = nextPoint.y - currentPoint.y;
        float dx = nextPoint.x - currentPoint.x;
        float size = (0.5f*thick)/sqrtf(dx*dx + dy*dy);

        if (i == 1)
        {
            points[0].x = currentPoint.x + dy*size;
            points[0].y = currentPoint.y - dx*size;
            points[1].x = currentPoint.x - dy*size;
            points[1].y = currentPoint.y + dx*size;
        }

        points[2*i + 1].x = nextPoint.x - dy*size;
        points[2*i + 1].y = nextPoint.y + dx*size;
        points[2*i].x = nextPoint.x + dy*size;
        points[2*i].y = nextPoint.y - dx*size;

        currentPoint = nextPoint;
    }

    RLDrawTriangleStrip(points, 2*SPLINE_SEGMENT_DIVISIONS+2, color);
}

// Draw spline segment: Catmull-Rom, 4 points
void RLDrawSplineSegmentCatmullRom(RLVector2 p1, RLVector2 p2, RLVector2 p3, RLVector2 p4, float thick, RLColor color)
{
    const float step = 1.0f/SPLINE_SEGMENT_DIVISIONS;

    RLVector2 currentPoint = p1;
    RLVector2 nextPoint = { 0 };
    float t = 0.0f;

    RLVector2 points[2*SPLINE_SEGMENT_DIVISIONS + 2] = { 0 };

    for (int i = 0; i <= SPLINE_SEGMENT_DIVISIONS; i++)
    {
        t = step*(float)i;

        float q0 = (-1*t*t*t) + (2*t*t) + (-1*t);
        float q1 = (3*t*t*t) + (-5*t*t) + 2;
        float q2 = (-3*t*t*t) + (4*t*t) + t;
        float q3 = t*t*t - t*t;

        nextPoint.x = 0.5f*((p1.x*q0) + (p2.x*q1) + (p3.x*q2) + (p4.x*q3));
        nextPoint.y = 0.5f*((p1.y*q0) + (p2.y*q1) + (p3.y*q2) + (p4.y*q3));

        float dy = nextPoint.y - currentPoint.y;
        float dx = nextPoint.x - currentPoint.x;
        float size = (0.5f*thick)/sqrtf(dx*dx + dy*dy);

        if (i == 1)
        {
            points[0].x = currentPoint.x + dy*size;
            points[0].y = currentPoint.y - dx*size;
            points[1].x = currentPoint.x - dy*size;
            points[1].y = currentPoint.y + dx*size;
        }

        points[2*i + 1].x = nextPoint.x - dy*size;
        points[2*i + 1].y = nextPoint.y + dx*size;
        points[2*i].x = nextPoint.x + dy*size;
        points[2*i].y = nextPoint.y - dx*size;

        currentPoint = nextPoint;
    }

    RLDrawTriangleStrip(points, 2*SPLINE_SEGMENT_DIVISIONS + 2, color);
}

// Draw spline segment: Quadratic Bezier, 2 points, 1 control point
void RLDrawSplineSegmentBezierQuadratic(RLVector2 p1, RLVector2 c2, RLVector2 p3, float thick, RLColor color)
{
    const float step = 1.0f/SPLINE_SEGMENT_DIVISIONS;

    RLVector2 previous = p1;
    RLVector2 current = { 0 };
    float t = 0.0f;

    RLVector2 points[2*SPLINE_SEGMENT_DIVISIONS + 2] = { 0 };

    for (int i = 1; i <= SPLINE_SEGMENT_DIVISIONS; i++)
    {
        t = step*(float)i;

        float a = powf(1.0f - t, 2);
        float b = 2.0f*(1.0f - t)*t;
        float c = powf(t, 2);

        // NOTE: The easing functions aren't suitable here because they don't take a control point
        current.y = a*p1.y + b*c2.y + c*p3.y;
        current.x = a*p1.x + b*c2.x + c*p3.x;

        float dy = current.y - previous.y;
        float dx = current.x - previous.x;
        float size = 0.5f*thick/sqrtf(dx*dx+dy*dy);

        if (i == 1)
        {
            points[0].x = previous.x + dy*size;
            points[0].y = previous.y - dx*size;
            points[1].x = previous.x - dy*size;
            points[1].y = previous.y + dx*size;
        }

        points[2*i + 1].x = current.x - dy*size;
        points[2*i + 1].y = current.y + dx*size;
        points[2*i].x = current.x + dy*size;
        points[2*i].y = current.y - dx*size;

        previous = current;
    }

    RLDrawTriangleStrip(points, 2*SPLINE_SEGMENT_DIVISIONS + 2, color);
}

// Draw spline segment: Cubic Bezier, 2 points, 2 control points
void RLDrawSplineSegmentBezierCubic(RLVector2 p1, RLVector2 c2, RLVector2 c3, RLVector2 p4, float thick, RLColor color)
{
    const float step = 1.0f/SPLINE_SEGMENT_DIVISIONS;

    RLVector2 previous = p1;
    RLVector2 current = { 0 };
    float t = 0.0f;

    RLVector2 points[2*SPLINE_SEGMENT_DIVISIONS + 2] = { 0 };

    for (int i = 1; i <= SPLINE_SEGMENT_DIVISIONS; i++)
    {
        t = step*(float)i;

        float a = powf(1.0f - t, 3);
        float b = 3.0f*powf(1.0f - t, 2)*t;
        float c = 3.0f*(1.0f - t)*powf(t, 2);
        float d = powf(t, 3);

        current.y = a*p1.y + b*c2.y + c*c3.y + d*p4.y;
        current.x = a*p1.x + b*c2.x + c*c3.x + d*p4.x;

        float dy = current.y - previous.y;
        float dx = current.x - previous.x;
        float size = 0.5f*thick/sqrtf(dx*dx+dy*dy);

        if (i == 1)
        {
            points[0].x = previous.x + dy*size;
            points[0].y = previous.y - dx*size;
            points[1].x = previous.x - dy*size;
            points[1].y = previous.y + dx*size;
        }

        points[2*i + 1].x = current.x - dy*size;
        points[2*i + 1].y = current.y + dx*size;
        points[2*i].x = current.x + dy*size;
        points[2*i].y = current.y - dx*size;

        previous = current;
    }

    RLDrawTriangleStrip(points, 2*SPLINE_SEGMENT_DIVISIONS + 2, color);
}

// Get spline point for a given t [0.0f .. 1.0f], Linear
RLVector2 RLGetSplinePointLinear(RLVector2 startPos, RLVector2 endPos, float t)
{
    RLVector2 point = { 0 };

    point.x = startPos.x*(1.0f - t) + endPos.x*t;
    point.y = startPos.y*(1.0f - t) + endPos.y*t;

    return point;
}

// Get spline point for a given t [0.0f .. 1.0f], B-Spline
RLVector2 RLGetSplinePointBasis(RLVector2 p1, RLVector2 p2, RLVector2 p3, RLVector2 p4, float t)
{
    RLVector2 point = { 0 };

    float a[4] = { 0 };
    float b[4] = { 0 };

    a[0] = (-p1.x + 3*p2.x - 3*p3.x + p4.x)/6.0f;
    a[1] = (3*p1.x - 6*p2.x + 3*p3.x)/6.0f;
    a[2] = (-3*p1.x + 3*p3.x)/6.0f;
    a[3] = (p1.x + 4*p2.x + p3.x)/6.0f;

    b[0] = (-p1.y + 3*p2.y - 3*p3.y + p4.y)/6.0f;
    b[1] = (3*p1.y - 6*p2.y + 3*p3.y)/6.0f;
    b[2] = (-3*p1.y + 3*p3.y)/6.0f;
    b[3] = (p1.y + 4*p2.y + p3.y)/6.0f;

    point.x = a[3] + t*(a[2] + t*(a[1] + t*a[0]));
    point.y = b[3] + t*(b[2] + t*(b[1] + t*b[0]));

    return point;
}

// Get spline point for a given t [0.0f .. 1.0f], Catmull-Rom
RLVector2 RLGetSplinePointCatmullRom(RLVector2 p1, RLVector2 p2, RLVector2 p3, RLVector2 p4, float t)
{
    RLVector2 point = { 0 };

    float q0 = (-1*t*t*t) + (2*t*t) + (-1*t);
    float q1 = (3*t*t*t) + (-5*t*t) + 2;
    float q2 = (-3*t*t*t) + (4*t*t) + t;
    float q3 = t*t*t - t*t;

    point.x = 0.5f*((p1.x*q0) + (p2.x*q1) + (p3.x*q2) + (p4.x*q3));
    point.y = 0.5f*((p1.y*q0) + (p2.y*q1) + (p3.y*q2) + (p4.y*q3));

    return point;
}

// Get spline point for a given t [0.0f .. 1.0f], Quadratic Bezier
RLVector2 RLGetSplinePointBezierQuad(RLVector2 startPos, RLVector2 controlPos, RLVector2 endPos, float t)
{
    RLVector2 point = { 0 };

    float a = powf(1.0f - t, 2);
    float b = 2.0f*(1.0f - t)*t;
    float c = powf(t, 2);

    point.y = a*startPos.y + b*controlPos.y + c*endPos.y;
    point.x = a*startPos.x + b*controlPos.x + c*endPos.x;

    return point;
}

// Get spline point for a given t [0.0f .. 1.0f], Cubic Bezier
RLVector2 RLGetSplinePointBezierCubic(RLVector2 startPos, RLVector2 startControlPos, RLVector2 endControlPos, RLVector2 endPos, float t)
{
    RLVector2 point = { 0 };

    float a = powf(1.0f - t, 3);
    float b = 3.0f*powf(1.0f - t, 2)*t;
    float c = 3.0f*(1.0f - t)*powf(t, 2);
    float d = powf(t, 3);

    point.y = a*startPos.y + b*startControlPos.y + c*endControlPos.y + d*endPos.y;
    point.x = a*startPos.x + b*startControlPos.x + c*endControlPos.x + d*endPos.x;

    return point;
}

//----------------------------------------------------------------------------------
// Module Functions Definition - Collision Detection functions
//----------------------------------------------------------------------------------

// Check if point is inside rectangle
bool RLCheckCollisionPointRec(RLVector2 point, RLRectangle rec)
{
    bool collision = false;

    if ((point.x >= rec.x) && (point.x < (rec.x + rec.width)) && (point.y >= rec.y) && (point.y < (rec.y + rec.height))) collision = true;

    return collision;
}

// Check if point is inside circle
bool RLCheckCollisionPointCircle(RLVector2 point, RLVector2 center, float radius)
{
    bool collision = false;

    float distanceSquared = (point.x - center.x)*(point.x - center.x) + (point.y - center.y)*(point.y - center.y);

    if (distanceSquared <= radius*radius) collision = true;

    return collision;
}

// Check if point is inside a triangle defined by three points (p1, p2, p3)
bool RLCheckCollisionPointTriangle(RLVector2 point, RLVector2 p1, RLVector2 p2, RLVector2 p3)
{
    bool collision = false;

    float alpha = ((p2.y - p3.y)*(point.x - p3.x) + (p3.x - p2.x)*(point.y - p3.y)) /
                  ((p2.y - p3.y)*(p1.x - p3.x) + (p3.x - p2.x)*(p1.y - p3.y));

    float beta = ((p3.y - p1.y)*(point.x - p3.x) + (p1.x - p3.x)*(point.y - p3.y)) /
                 ((p2.y - p3.y)*(p1.x - p3.x) + (p3.x - p2.x)*(p1.y - p3.y));

    float gamma = 1.0f - alpha - beta;

    if ((alpha > 0) && (beta > 0) && (gamma > 0)) collision = true;

    return collision;
}

// Check if point is within a polygon described by array of vertices
// NOTE: Based on http://jeffreythompson.org/collision-detection/poly-point.php
bool RLCheckCollisionPointPoly(RLVector2 point, const RLVector2 *points, int pointCount)
{
    bool collision = false;

    if (pointCount > 2)
    {
        for (int i = 0, j = pointCount - 1; i < pointCount; j = i++)
        {
            if ((points[i].y > point.y) != (points[j].y > point.y) &&
                (point.x < (points[j].x - points[i].x)*(point.y - points[i].y)/(points[j].y - points[i].y) + points[i].x))
            {
                collision = !collision;
            }
        }
    }

    return collision;
}

// Check collision between two rectangles
bool RLCheckCollisionRecs(RLRectangle rec1, RLRectangle rec2)
{
    bool collision = false;

    if ((rec1.x < (rec2.x + rec2.width) && (rec1.x + rec1.width) > rec2.x) &&
        (rec1.y < (rec2.y + rec2.height) && (rec1.y + rec1.height) > rec2.y)) collision = true;

    return collision;
}

// Check collision between two circles
bool RLCheckCollisionCircles(RLVector2 center1, float radius1, RLVector2 center2, float radius2)
{
    bool collision = false;

    float dx = center2.x - center1.x;      // X distance between centers
    float dy = center2.y - center1.y;      // Y distance between centers

    float distanceSquared = dx*dx + dy*dy; // Distance between centers squared
    float radiusSum = radius1 + radius2;

    collision = (distanceSquared <= (radiusSum*radiusSum));

    return collision;
}

// Check collision between circle and rectangle
// NOTE: Reviewed version to take into account corner limit case
bool RLCheckCollisionCircleRec(RLVector2 center, float radius, RLRectangle rec)
{
    bool collision = false;

    float recCenterX = rec.x + rec.width/2.0f;
    float recCenterY = rec.y + rec.height/2.0f;

    float dx = fabsf(center.x - recCenterX);
    float dy = fabsf(center.y - recCenterY);

    if (dx > (rec.width/2.0f + radius)) { return false; }
    if (dy > (rec.height/2.0f + radius)) { return false; }

    if (dx <= (rec.width/2.0f)) { return true; }
    if (dy <= (rec.height/2.0f)) { return true; }

    float cornerDistanceSq = (dx - rec.width/2.0f)*(dx - rec.width/2.0f) +
                             (dy - rec.height/2.0f)*(dy - rec.height/2.0f);

    collision = (cornerDistanceSq <= (radius*radius));

    return collision;
}

// Check the collision between two lines defined by two points each, returns collision point by reference
// REF: https://en.wikipedia.org/wiki/Line–line_intersection#Given_two_points_on_each_line_segment
bool RLCheckCollisionLines(RLVector2 startPos1, RLVector2 endPos1, RLVector2 startPos2, RLVector2 endPos2, RLVector2 *collisionPoint)
{
    bool collision = false;

    float rx = endPos1.x - startPos1.x;
    float ry = endPos1.y - startPos1.y;
    float sx = endPos2.x - startPos2.x;
    float sy = endPos2.y - startPos2.y;

    float div = rx*sy - ry*sx;

    if (fabsf(div) >= FLT_EPSILON)
    {
        float s12x = startPos2.x - startPos1.x;
        float s12y = startPos2.y - startPos1.y;

        float t = (s12x*sy - s12y*sx)/div;
        float u = (s12x*ry - s12y*rx)/div;

        if ((0.0f <= t) && (t <= 1.0f) && (0.0f <= u) && (u <= 1.0f))
        {
            collisionPoint->x = startPos1.x + t*rx;
            collisionPoint->y = startPos1.y + t*ry;
            
            collision = true;
        }
    }
    
    return collision;
}

// Check if point belongs to line created between two points [p1] and [p2] with defined margin in pixels [threshold]
bool RLCheckCollisionPointLine(RLVector2 point, RLVector2 p1, RLVector2 p2, int threshold)
{
    bool collision = false;

    float dxc = point.x - p1.x;
    float dyc = point.y - p1.y;
    float dxl = p2.x - p1.x;
    float dyl = p2.y - p1.y;
    float cross = dxc*dyl - dyc*dxl;

    if (fabsf(cross) < (threshold*fmaxf(fabsf(dxl), fabsf(dyl))))
    {
        if (fabsf(dxl) >= fabsf(dyl)) collision = (dxl > 0)? ((p1.x <= point.x) && (point.x <= p2.x)) : ((p2.x <= point.x) && (point.x <= p1.x));
        else collision = (dyl > 0)? ((p1.y <= point.y) && (point.y <= p2.y)) : ((p2.y <= point.y) && (point.y <= p1.y));
    }

    return collision;
}

// Check if circle collides with a line created between two points [p1] and [p2]
bool RLCheckCollisionCircleLine(RLVector2 center, float radius, RLVector2 p1, RLVector2 p2)
{
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;

    if ((fabsf(dx) + fabsf(dy)) <= FLT_EPSILON)
    {
        return RLCheckCollisionCircles(p1, 0, center, radius);
    }

    float lengthSQ = ((dx*dx) + (dy*dy));
    float dotProduct = (((center.x - p1.x)*(p2.x - p1.x)) + ((center.y - p1.y)*(p2.y - p1.y)))/(lengthSQ);

    if (dotProduct > 1.0f) dotProduct = 1.0f;
    else if (dotProduct < 0.0f) dotProduct = 0.0f;

    float dx2 = (p1.x - (dotProduct*(dx))) - center.x;
    float dy2 = (p1.y - (dotProduct*(dy))) - center.y;
    float distanceSQ = ((dx2*dx2) + (dy2*dy2));

    return (distanceSQ <= radius*radius);
}

// Get collision rectangle for two rectangles collision
RLRectangle RLGetCollisionRec(RLRectangle rec1, RLRectangle rec2)
{
    RLRectangle overlap = { 0 };

    float left = (rec1.x > rec2.x)? rec1.x : rec2.x;
    float right1 = rec1.x + rec1.width;
    float right2 = rec2.x + rec2.width;
    float right = (right1 < right2)? right1 : right2;
    float top = (rec1.y > rec2.y)? rec1.y : rec2.y;
    float bottom1 = rec1.y + rec1.height;
    float bottom2 = rec2.y + rec2.height;
    float bottom = (bottom1 < bottom2)? bottom1 : bottom2;

    if ((left < right) && (top < bottom))
    {
        overlap.x = left;
        overlap.y = top;
        overlap.width = right - left;
        overlap.height = bottom - top;
    }

    return overlap;
}

//----------------------------------------------------------------------------------
// Module Internal Functions Definition
//----------------------------------------------------------------------------------

// Cubic easing in-out
// NOTE: Used by DrawLineBezier() only
static float EaseCubicInOut(float t, float b, float c, float d)
{
    float result = 0.0f;

    if ((t /= 0.5f*d) < 1) result = 0.5f*c*t*t*t + b;
    else
    {
        t -= 2;
        result = 0.5f*c*(t*t*t + 2.0f) + b;
    }

    return result;
}

#endif      // SUPPORT_MODULE_RSHAPES
