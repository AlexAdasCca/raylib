/*******************************************************************************************
*
*   raylib example - fluent button
*
*   This example implements a set of basic UI building blocks (buttons and flyout)
*   with a Fluent-like feel.
*
*   Key features:
*   - Smooth theme transitions (colors blend over time)
*   - Optional background image and blur (down-sample RenderTexture)
*   - Acrylic-like surfaces for card and flyout (blurred backdrop sampling, tint and grain)
*   - Flyout popup anchored to a button
*
*   Controls:
*   - Tab / Shift+Tab: focus (main buttons)
*   - Enter/Space: activate focused button
*   - T: toggle theme (animated)
*   - B: toggle background image
*   - V: toggle background blur (for the window background)
*   - [ / ]: blur strength (window background)
*   - A: toggle Acrylic surfaces (card and flyout)
*   - N: toggle Acrylic grain
*   - M: toggle "default buttons" Acrylic fill
*
*   Example complexity rating: [★★★☆] 3/4
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2016-2026
*
********************************************************************************************/

#include "raylib.h"
#include <math.h>
#include <stddef.h> // NULL
#include <string.h>

// This repo uses RL* prefixed API. If you compile against upstream raylib,
// rename RL* -> * (e.g. RLBeginDrawing -> BeginDrawing).

// --------------------------------------------------------------------------------------
// Math helpers
// --------------------------------------------------------------------------------------

static float LerpF(float a, float b, float t) { return a + (b - a) * t; }
static float Clamp01(float x) { return (x < 0.0f) ? 0.0f : (x > 1.0f ? 1.0f : x); }

static float Smoothstep(float t)
{
    t = Clamp01(t);
    return t * t * (3.0f - 2.0f * t);
}

static float ApproachExp(float current, float target, float speed, float fDeltaTime)
{
    // Framerate-independent exponential smoothing.
    float k = 1.0f - expf(-speed * fDeltaTime);
    return LerpF(current, target, k);
}

static float EaseOutCubic(float t)
{
    t = Clamp01(t);
    float u = 1.0f - t;
    return 1.0f - u * u * u;
}

static RLColor LerpColor(RLColor a, RLColor b, float t)
{
    t = Clamp01(t);
    RLColor c;
    c.r = (unsigned char)((float)a.r + ((float)b.r - (float)a.r) * t);
    c.g = (unsigned char)((float)a.g + ((float)b.g - (float)a.g) * t);
    c.b = (unsigned char)((float)a.b + ((float)b.b - (float)a.b) * t);
    c.a = (unsigned char)((float)a.a + ((float)b.a - (float)a.a) * t);
    return c;
}

// --------------------------------------------------------------------------------------
// Scissor stack (raylib scissor is not nested; we emulate nesting by re-applying parent).
// This is required for ScrollViewer clipping to remain valid when buttons use inner scissor
// for reveal/ripple effects.
// --------------------------------------------------------------------------------------

typedef struct ScissorRectI { int x, y, w, h; } ScissorRectI;

#define SCISSOR_STACK_MAX 16
static ScissorRectI gScissorStack[SCISSOR_STACK_MAX];
static int gScissorDepth = 0;

#define UI_ID_SCRIM        9000
#define UI_ID_FLY_PANEL    9001
#define UI_ID_SCROLL_TRACK 9002
#define UI_ID_SCROLL_THUMB 9003

static int gUiActiveId = -1;

// Global acxAcrylic toggles (kept simple for the example).
static bool gAcrylicEnabled = true;
static bool gAcrylicNoise = true;
static bool gAcrylicButtons = false;

static bool gFlyoutConstrainToCard = false;

static const char* ACRYLIC_FS_330 =
"#version 330\n"
"in vec2 fragTexCoord;\n"
"in vec4 fragColor;\n"
"out vec4 finalColor;\n"
"uniform sampler2D texture0;\n"
"uniform vec4 colDiffuse;\n"
"uniform sampler2D u_backdrop;\n"
"uniform vec2 u_screen;\n"
"uniform vec4 u_rect;\n"       // x,y,w,h (top-left pixel coords)
"uniform float u_radius;\n"
"uniform vec4 u_tint;\n"       // rgb in 0..1, a = tint amount
"uniform float u_grain;\n"     // ~0..0.03
"uniform float u_time;\n"
"uniform float u_soften;\n"
"\n"
"float hash12(vec2 p) {\n"
"    // Cheap stable hash (no texture).\n"
"    vec3 p3 = fract(vec3(p.xyx) * 0.1031);\n"
"    p3 += dot(p3, p3.yzx + 33.33);\n"
"    return fract((p3.x + p3.y) * p3.z);\n"
"}\n"
"\n"
"float sdRoundRect(vec2 p, vec2 b, float r) {\n"
"    vec2 q = abs(p) - b;\n"
"    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;\n"
"}\n"
"\n"
"void main()\n"
"{\n"
"    // Convert gl_FragCoord to a top-left origin pixel coord (raylib drawing space).\n"
"    vec2 frag = vec2(gl_FragCoord.x, u_screen.y - gl_FragCoord.y);\n"
"\n"
"    // Bounds check (cheap).\n"
"    vec2 rel = frag - u_rect.xy;\n"
"    if (rel.x < 0.0 || rel.y < 0.0 || rel.x > u_rect.z || rel.y > u_rect.w) discard;\n"
"\n"
"    // Sample the down-sampled backdrop using window-relative UV.\n"
"    vec2 uv = vec2(frag.x / u_screen.x, 1.0 - (frag.y / u_screen.y));\n"
"    vec3 bg = texture(u_backdrop, uv).rgb;\n"
"    if (u_soften > 0.001) {\n"
"        vec2 texel = 1.0 / vec2(textureSize(u_backdrop, 0));\n"
"        float s = mix(0.75, 2.50, clamp(u_soften, 0.0, 1.0));\n"
"        vec3 sum = bg * 0.36;\n"
"        sum += texture(u_backdrop, uv + vec2(texel.x*s, 0.0)).rgb * 0.16;\n"
"        sum += texture(u_backdrop, uv - vec2(texel.x*s, 0.0)).rgb * 0.16;\n"
"        sum += texture(u_backdrop, uv + vec2(0.0, texel.y*s)).rgb * 0.16;\n"
"        sum += texture(u_backdrop, uv - vec2(0.0, texel.y*s)).rgb * 0.16;\n"
"        bg = sum;\n"
"    }\n"
"\n"
"    // Slight desaturation helps the acxAcrylic read as 'frosted'.\n"
"    float lum = dot(bg, vec3(0.299, 0.587, 0.114));\n"
"    bg = mix(bg, vec3(lum), 0.18);\n"
"\n"
"    // Tint blend (amount in u_tint.a).\n"
"    vec3 col = mix(bg, u_tint.rgb, clamp(u_tint.a, 0.0, 1.0));\n"
"\n"
"    // Subtle film grain (luma-ish). Using per-pixel hash in screen space avoids tiling artifacts.\n"
"    if (u_grain > 0.0001) {\n"
"        float g = hash12(floor(frag * 1.0 + vec2(17.0, 53.0)));\n"
"        g = (g - 0.5);\n"
"        col += g * u_grain;\n"
"    }\n"
"\n"
"    // Rounded-rect mask (SDF).\n"
"    vec2 size = u_rect.zw;\n"
"    vec2 p = rel - size * 0.5;\n"
"    vec2 b = size * 0.5 - vec2(u_radius);\n"
"    float d = sdRoundRect(p, b, u_radius);\n"
"\n"
"    // 1px-ish antialias for the edge.\n"
"    float aa = max(fwidth(d), 1.0);\n"
"    float mask = 1.0 - smoothstep(0.0, aa, d);\n"
"\n"
"    vec4 outCol = vec4(col, mask);\n"
"    finalColor = outCol * fragColor * colDiffuse;\n"
"}\n";

static ScissorRectI ScissorIntersectI(ScissorRectI a, ScissorRectI b)
{
    int x1 = (a.x > b.x) ? a.x : b.x;
    int y1 = (a.y > b.y) ? a.y : b.y;
    int x2 = ((a.x + a.w) < (b.x + b.w)) ? (a.x + a.w) : (b.x + b.w);
    int y2 = ((a.y + a.h) < (b.y + b.h)) ? (a.y + a.h) : (b.y + b.h);

    ScissorRectI scissorRect = (ScissorRectI){ x1, y1, x2 - x1, y2 - y1 };
    if (scissorRect.w <= 0 || scissorRect.h <= 0)
    {
        // Offscreen 1x1: avoids stray pixels when empty intersections occur.
        int iScreenWidth = RLGetScreenWidth();
        int iScreenHeight = RLGetScreenHeight();
        scissorRect.x = iScreenWidth + 16;
        scissorRect.y = iScreenHeight + 16;
        scissorRect.w = 1;
        scissorRect.h = 1;
    }
    return scissorRect;
}

static void ScissorReset(void)
{
    if (gScissorDepth > 0)
    {
        RLEndScissorMode();
        gScissorDepth = 0;
    }
}

static void PushScissorI(int x, int y, int w, int h)
{
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    ScissorRectI r = (ScissorRectI){ x, y, w, h };
    if (gScissorDepth > 0)
    {
        r = ScissorIntersectI(gScissorStack[gScissorDepth - 1], r);
    }

    if (gScissorDepth < SCISSOR_STACK_MAX)
    {
        gScissorStack[gScissorDepth++] = r;
    }
    // BeginScissorMode always sets the active scissor rectangle.
    RLBeginScissorMode(r.x, r.y, r.w, r.h);
}

static void PushScissorRect(RLRectangle r, int inset)
{
    int x = (int)(r.x + 0.5f) + inset;
    int y = (int)(r.y + 0.5f) + inset;
    int w = (int)(r.width + 0.5f) - inset * 2;
    int h = (int)(r.height + 0.5f) - inset * 2;
    PushScissorI(x, y, w, h);
}

static void PopScissor(void)
{
    if (gScissorDepth <= 0) return;
    gScissorDepth--;
    if (gScissorDepth == 0)
    {
        RLEndScissorMode();
        return;
    }
    ScissorRectI scissorRect = gScissorStack[gScissorDepth - 1];
    RLBeginScissorMode(scissorRect.x, scissorRect.y, scissorRect.w, scissorRect.h);
}


// --------------------------------------------------------------------------------------
// Theme tokens
// --------------------------------------------------------------------------------------

typedef struct ThemeTokens {
    RLColor bg;
    RLColor surface;
    RLColor surface2;
    RLColor text;
    RLColor textDim;
    RLColor disabledText;
    RLColor border;
    RLColor borderHot;
    RLColor shadow;
    RLColor accent;
    RLColor accent2;
} ThemeTokens;

static ThemeTokens LerpTheme(ThemeTokens a, ThemeTokens b, float t)
{
    ThemeTokens oThemeTokens;
    oThemeTokens.bg = LerpColor(a.bg, b.bg, t);
    oThemeTokens.surface = LerpColor(a.surface, b.surface, t);
    oThemeTokens.surface2 = LerpColor(a.surface2, b.surface2, t);
    oThemeTokens.text = LerpColor(a.text, b.text, t);
    oThemeTokens.textDim = LerpColor(a.textDim, b.textDim, t);
    oThemeTokens.disabledText = LerpColor(a.disabledText, b.disabledText, t);
    oThemeTokens.border = LerpColor(a.border, b.border, t);
    oThemeTokens.borderHot = LerpColor(a.borderHot, b.borderHot, t);
    oThemeTokens.shadow = LerpColor(a.shadow, b.shadow, t);
    oThemeTokens.accent = LerpColor(a.accent, b.accent, t);
    oThemeTokens.accent2 = LerpColor(a.accent2, b.accent2, t);
    return oThemeTokens;
}

static ThemeTokens ThemeDark(void)
{
    ThemeTokens oThemeTokens = { 0 };
    oThemeTokens.bg = (RLColor){ 15, 15, 17, 255 };
    oThemeTokens.surface = (RLColor){ 27, 27, 30, 255 };
    oThemeTokens.surface2 = (RLColor){ 34, 34, 38, 255 };
    oThemeTokens.text = (RLColor){ 235, 235, 245, 255 };
    oThemeTokens.textDim = (RLColor){ 180, 180, 190, 255 };
    oThemeTokens.disabledText = (RLColor){ 130, 130, 140, 255 };
    oThemeTokens.border = (RLColor){ 255, 255, 255, 35 };
    oThemeTokens.borderHot = (RLColor){ 255, 255, 255, 85 };
    oThemeTokens.shadow = (RLColor){ 0, 0, 0, 255 };
    oThemeTokens.accent = (RLColor){ 95, 168, 255, 255 };
    oThemeTokens.accent2 = (RLColor){ 130, 198, 255, 255 };
    return oThemeTokens;
}

static ThemeTokens ThemeLight(void)
{
    ThemeTokens oThemeTokens = { 0 };
    oThemeTokens.bg = (RLColor){ 246, 246, 248, 255 };
    oThemeTokens.surface = (RLColor){ 255, 255, 255, 255 };
    oThemeTokens.surface2 = (RLColor){ 246, 246, 248, 255 };
    oThemeTokens.text = (RLColor){ 20,  20,  22, 255 };
    oThemeTokens.textDim = (RLColor){ 90,  90,  98, 255 };
    oThemeTokens.disabledText = (RLColor){ 140, 140, 150, 255 };
    oThemeTokens.border = (RLColor){ 0, 0, 0, 25 };
    oThemeTokens.borderHot = (RLColor){ 0, 0, 0, 60 };
    oThemeTokens.shadow = (RLColor){ 0, 0, 0, 255 };
    oThemeTokens.accent = (RLColor){ 30, 108, 229, 255 };
    oThemeTokens.accent2 = (RLColor){ 60, 140, 245, 255 };
    return oThemeTokens;
}

// --------------------------------------------------------------------------------------
// Rounded geometry helpers
// --------------------------------------------------------------------------------------

static float RoundnessForRadius(float w, float h, float radius)
{
    float m = (w < h) ? w : h;
    if (m <= 0.0f) return 0.0f;
    // In raylib, corner radius = roundness * min(w,h) / 2.
    float roundness = (2.0f * radius) / m;
    return Clamp01(roundness);
}

static void DrawSoftShadowRounded(RLRectangle r, float radius, float strength, const ThemeTokens* ttTheme)
{
    strength = Clamp01(strength);
    if (strength <= 0.001f) return;

    float round = RoundnessForRadius(r.width, r.height, radius);
    int seg = 12;

    const int layers = 8;
    float spread = 10.0f + 18.0f * strength;
    float yoff = 1.5f + 3.0f * strength;
    float baseA = 26.0f + 22.0f * strength;

    for (int i = 0; i < layers; i++)
    {
        float t = (float)(i + 1) / (float)layers;
        float expand = spread * t;

        RLRectangle sr = r;
        sr.x -= expand * 0.5f;
        sr.width += expand;

        sr.y += yoff * t - expand * 0.18f;
        sr.height += expand;

        float w = (1.0f - t);
        float a = baseA * w * w;
        RLColor color = ttTheme->shadow;
        color.a = (unsigned char)(a);
        RLDrawRectangleRounded(sr, round, seg, color);
    }
}

static void DrawRevealGradientClipped(RLRectangle r, RLVector2 p, float intensity, float fThemeLightness)
{
    if (intensity <= 0.001f) return;

    float alpha = LerpF(35.0f, 60.0f, Clamp01(fThemeLightness)) * intensity;

    PushScissorRect(r, 0);
    {
        float rad = 140.0f;
        RLColor inner = (RLColor){ 255, 255, 255, 0 };
        inner.a = (unsigned char)((alpha < 0.0f) ? 0.0f : (alpha > 255.0f ? 255.0f : alpha));
        RLColor outerColor = (RLColor){ inner.r, inner.g, inner.b, 0 };
        RLDrawCircleGradient((int)p.x, (int)p.y, rad, inner, outerColor);
    }
    PopScissor();
}

static bool PointInAnyRect(RLVector2 p, const RLRectangle* rects, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (RLCheckCollisionPointRec(p, rects[i])) return true;
    }
    return false;
}

// --------------------------------------------------------------------------------------
// Backdrop helpers (photo cover and down-sample blur)
// --------------------------------------------------------------------------------------

static RLRectangle ComputeCoverSourceRect(float texW, float texH, float destW, float destH)
{
    float sx = destW / texW;
    float sy = destH / texH;
    float scale = (sx > sy) ? sx : sy;
    float srcW = destW / scale;
    float srcH = destH / scale;
    float srcX = (texW - srcW) * 0.5f;
    float srcY = (texH - srcH) * 0.5f;
    return (RLRectangle) { srcX, srcY, srcW, srcH };
}

static int BlurDownSampleFromLevel(int level)
{
    if (level < 1) level = 1;
    if (level > 5) level = 5;
    switch (level)
    {
    case 1: return 2;
    case 2: return 4;
    case 3: return 8;
    case 4: return 12;
    default: return 16;
    }
}

static void EnsureRT(RLRenderTexture2D* rt, int* curW, int* curH, int wantW, int wantH)
{
    if (wantW < 1) wantW = 1;
    if (wantH < 1) wantH = 1;

    if ((rt->id == 0) || (*curW != wantW) || (*curH != wantH))
    {
        if (rt->id != 0) RLUnloadRenderTexture(*rt);
        *rt = RLLoadRenderTexture(wantW, wantH);
        RLSetTextureFilter(rt->texture, TEXTURE_FILTER_BILINEAR);
        *curW = wantW;
        *curH = wantH;
    }
}

// Draw background into the *current* target (screen or RenderTexture).
static void DrawBackdropToCurrentTarget(int targetW, int targetH,
    const ThemeTokens* ttTheme, float fThemeLightness,
    bool useImage, bool imageLoaded, RLTexture2D texBackground,
    bool applyThemeTint)
{
    if (useImage && imageLoaded && (texBackground.id != 0))
    {
        RLRectangle dst = (RLRectangle){ 0, 0, (float)targetW, (float)targetH };
        RLRectangle src = ComputeCoverSourceRect((float)texBackground.width, (float)texBackground.height, dst.width, dst.height);
        RLDrawTexturePro(texBackground, src, dst, (RLVector2) { 0, 0 }, 0.0f, (RLColor) { 255, 255, 255, 255 });

        if (applyThemeTint)
        {
            RLColor tintColor = ttTheme->bg;
            tintColor.a = (unsigned char)(140.0f + 30.0f * Clamp01(fThemeLightness));
            RLDrawRectangle(0, 0, targetW, targetH, tintColor);
        }
    }
    else
    {
        RLClearBackground(ttTheme->bg);
    }

    // Accent blobs (positions are defined in window-relative space).
    // Using relative coords keeps them aligned between the screen and down-sampled RenderTexture.
    float c0x = 0.1333333f * (float)targetW;
    float c0y = 0.2692307f * (float)targetH;
    float c1x = 0.8444444f * (float)targetW;
    float c1y = 0.8076923f * (float)targetH;

    float rad0 = 0.2888889f * (float)targetW;
    float rad1 = 0.3555555f * (float)targetW;

    RLColor a0 = ttTheme->accent;  a0.a = (unsigned char)LerpF(22.0f, 16.0f, Clamp01(fThemeLightness));
    RLColor a1 = ttTheme->accent2; a1.a = (unsigned char)LerpF(18.0f, 14.0f, Clamp01(fThemeLightness));

    RLDrawCircleGradient((int)c0x, (int)c0y, rad0, a0, (RLColor) { ttTheme->bg.r, ttTheme->bg.g, ttTheme->bg.b, 0 });
    RLDrawCircleGradient((int)c1x, (int)c1y, rad1, a1, (RLColor) { ttTheme->bg.r, ttTheme->bg.g, ttTheme->bg.b, 0 });
}

static void DrawWindowBackdrop(const ThemeTokens* ttTheme, float fThemeLightness,
    bool useImage, bool imageLoaded, RLTexture2D texBackground,
    bool useBlur, int blurLevel,
    RLRenderTexture2D* blurRT, int* blurW, int* blurH)
{
    int iScreenWidth = RLGetScreenWidth();
    int iScreenHeight = RLGetScreenHeight();

    if (useImage && imageLoaded && (texBackground.id != 0) && useBlur)
    {
        int ds = BlurDownSampleFromLevel(blurLevel);
        int w = (iScreenWidth + ds - 1) / ds;
        int h = (iScreenHeight + ds - 1) / ds;
        EnsureRT(blurRT, blurW, blurH, w, h);

        RLBeginTextureMode(*blurRT);
        RLClearBackground((RLColor) { 0, 0, 0, 0 });
        DrawBackdropToCurrentTarget(w, h, ttTheme, fThemeLightness, useImage, imageLoaded, texBackground, true);
        RLEndTextureMode();

        RLRectangle srcRT = (RLRectangle){ 0, 0, (float)blurRT->texture.width, -(float)blurRT->texture.height };
        RLRectangle dst = (RLRectangle){ 0, 0, (float)iScreenWidth, (float)iScreenHeight };
        RLDrawTexturePro(blurRT->texture, srcRT, dst, (RLVector2) { 0, 0 }, 0.0f, (RLColor) { 255, 255, 255, 255 });
    }
    else
    {
        // No blur: draw directly to screen.
        DrawBackdropToCurrentTarget(iScreenWidth, iScreenHeight, ttTheme, fThemeLightness, useImage, imageLoaded, texBackground, true);
    }
}

static bool TryLoadBackgroundTexture(RLTexture2D* outTex, const char** outPath)
{
    static const char* candidates[] = {
        "resources/fluent_bg.png",
        "resources/fluent_bg.jpg",
        "resources/background.png",
        "resources/background.jpg",
        "../resources/fluent_bg.png",
        "../resources/fluent_bg.jpg",
        "../resources/background.png",
        "../resources/background.jpg",
    };

    for (int i = 0; i < (int)(sizeof(candidates) / sizeof(candidates[0])); i++)
    {
        const char* pCandidatePath = candidates[i];
        if (!RLFileExists(pCandidatePath)) continue;

        RLImage img = RLLoadImage(pCandidatePath);
        if (img.data == NULL) continue;

        RLTexture2D tex = RLLoadTextureFromImage(img);
        RLUnloadImage(img);

        if (tex.id == 0) continue;
        RLSetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);

        *outTex = tex;
        if (outPath) *outPath = pCandidatePath;
        return true;
    }

    if (outPath) *outPath = NULL;
    return false;
}

// --------------------------------------------------------------------------------------
// Acrylic surface (rounded rcClip via shader)
// --------------------------------------------------------------------------------------

typedef struct AcrylicCtx {
    RLShader shader;
    int locScreen;
    int locRect;
    int locRadius;
    int locTint;
    int locGrain;
    int locTime;
    int locSoften;
    int locBackdrop;
} AcrylicCtx;

static void AcrylicInit(AcrylicCtx* acrylicCtx)
{
    acrylicCtx->shader = RLLoadShaderFromMemory(0, ACRYLIC_FS_330);
    acrylicCtx->locScreen = RLGetShaderLocation(acrylicCtx->shader, "u_screen");
    acrylicCtx->locRect = RLGetShaderLocation(acrylicCtx->shader, "u_rect");
    acrylicCtx->locRadius = RLGetShaderLocation(acrylicCtx->shader, "u_radius");
    acrylicCtx->locTint = RLGetShaderLocation(acrylicCtx->shader, "u_tint");
    acrylicCtx->locGrain = RLGetShaderLocation(acrylicCtx->shader, "u_grain");
    acrylicCtx->locTime = RLGetShaderLocation(acrylicCtx->shader, "u_time");
    acrylicCtx->locSoften = RLGetShaderLocation(acrylicCtx->shader, "u_soften");
    acrylicCtx->locBackdrop = RLGetShaderLocation(acrylicCtx->shader, "u_backdrop");
}

static void AcrylicUnload(AcrylicCtx* acrylicCtx)
{
    if (acrylicCtx->shader.id != 0) RLUnloadShader(acrylicCtx->shader);
    acrylicCtx->shader = (RLShader){ 0 };
}

static void DrawAcrylicRounded(const AcrylicCtx* acrylicCtx, RLTexture2D backdrop,
    RLRectangle r, float radius,
    RLColor tintRgb, float tintAmount,
    float grainAmount,
    float fTimeSeconds, float soften)
{
    if (backdrop.id == 0) return;

    if (acrylicCtx->shader.id == 0) {
        float round = RoundnessForRadius(r.width, r.height, radius);
        RLColor c = tintRgb;
        c.a = (unsigned char)(Clamp01(tintAmount) * 255.0f);
        RLDrawRectangleRounded(r, round, 12, c);
        return;
    }

    float screen[2] = { (float)RLGetScreenWidth(), (float)RLGetScreenHeight() };
    float rect[4] = { r.x, r.y, r.width, r.height };
    float rad = radius;

    float tint[4] = {
        (float)tintRgb.r / 255.0f,
        (float)tintRgb.g / 255.0f,
        (float)tintRgb.b / 255.0f,
        Clamp01(tintAmount)
    };

    float grain = (grainAmount < 0.0f) ? 0.0f : grainAmount;

    RLSetShaderValue(acrylicCtx->shader, acrylicCtx->locScreen, screen, SHADER_UNIFORM_VEC2);
    RLSetShaderValue(acrylicCtx->shader, acrylicCtx->locRect, rect, SHADER_UNIFORM_VEC4);
    RLSetShaderValue(acrylicCtx->shader, acrylicCtx->locRadius, &rad, SHADER_UNIFORM_FLOAT);
    RLSetShaderValue(acrylicCtx->shader, acrylicCtx->locTint, tint, SHADER_UNIFORM_VEC4);
    RLSetShaderValue(acrylicCtx->shader, acrylicCtx->locGrain, &grain, SHADER_UNIFORM_FLOAT);
    RLSetShaderValue(acrylicCtx->shader, acrylicCtx->locTime, &fTimeSeconds, SHADER_UNIFORM_FLOAT);
    float soft = Clamp01(soften);
    RLSetShaderValue(acrylicCtx->shader, acrylicCtx->locSoften, &soft, SHADER_UNIFORM_FLOAT);

    // Bind backdrop to texture0.
    RLSetShaderValueTexture(acrylicCtx->shader, acrylicCtx->locBackdrop, backdrop);

    RLBeginShaderMode(acrylicCtx->shader);
    RLDrawRectangleRec(r, (RLColor) { 255, 255, 255, 255 });
    RLEndShaderMode();
}

// Update the down-sampled backdrop used by Acrylic fills.
static void UpdateAcrylicBackdropRT(RLRenderTexture2D* pRenderTexture, int* rtW, int* rtH,
    int downSample,
    const ThemeTokens* ttTheme, float fThemeLightness,
    bool useImage, bool imageLoaded, RLTexture2D texBackground)
{
    int iScreenWidth = RLGetScreenWidth();
    int iScreenHeight = RLGetScreenHeight();

    int w = (iScreenWidth + downSample - 1) / downSample;
    int h = (iScreenHeight + downSample - 1) / downSample;

    EnsureRT(pRenderTexture, rtW, rtH, w, h);

    RLBeginTextureMode(*pRenderTexture);
    RLClearBackground((RLColor) { 0, 0, 0, 0 });
    // For acxAcrylic sampling we always want the "blurred" view of the backdrop,
    // so we draw into a small RenderTexture (down-sample does the blur).
    DrawBackdropToCurrentTarget(w, h, ttTheme, fThemeLightness, useImage, imageLoaded, texBackground, true);
    RLEndTextureMode();
}


// --------------------------------------------------------------------------------------
// Simple UI routing (virtual Z-order and pointer capture)
// --------------------------------------------------------------------------------------

typedef struct UIHitItem {
    int id;
    RLRectangle rect;
    int z;
} UIHitItem;

typedef struct UIInput {
    RLVector2 mouse;
    bool down;
    bool pressed;
    bool released;
    int hotId;
    int activeId;
} UIInput;

static int UIHitTest(const UIHitItem* items, int count, RLVector2 p)
{
    int bestId = -1;
    int bestZ = -2147483647;
    for (int i = 0; i < count; i++)
    {
        if (items[i].id < 0) continue;
        if (!RLCheckCollisionPointRec(p, items[i].rect)) continue;
        if (items[i].z >= bestZ)
        {
            bestZ = items[i].z;
            bestId = items[i].id;
        }
    }
    return bestId;
}

static void UIInputBegin(UIInput* uiInput, const UIHitItem* items, int count)
{
    uiInput->mouse = RLGetMousePosition();
    uiInput->down = RLIsMouseButtonDown(MOUSE_BUTTON_LEFT);
    uiInput->pressed = RLIsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    uiInput->released = RLIsMouseButtonReleased(MOUSE_BUTTON_LEFT);

    if (!RLIsWindowFocused()) gUiActiveId = -1;

    uiInput->hotId = UIHitTest(items, count, uiInput->mouse);

    if (uiInput->pressed) gUiActiveId = uiInput->hotId;
    uiInput->activeId = gUiActiveId;

    // Clear capture after release is evaluated this frame.
    if (uiInput->released) gUiActiveId = -1;
    if (!uiInput->down && !uiInput->released) gUiActiveId = -1;
}

// --------------------------------------------------------------------------------------
// Responsive layout
// --------------------------------------------------------------------------------------

typedef struct UILayout {
    RLRectangle rcCard;
    RLRectangle rcContentClip;   // Scroll viewport inside rcCard (excludes footer)
    RLRectangle rcFooter;    // Fixed footer area inside rcCard (does not scroll)
    float fContentHeight;            // Scrollable content height (unscrolled, screen space)
    float fScrollMax;

    RLVector2 v2VariantsTitlePos;
    RLVector2 v2ThemeTitlePos;
    RLVector2 v2ThemeStatePos;

    // Main controls
    RLRectangle rcButtonOpen;
    RLRectangle rcButtonPrimary;
    RLRectangle rcButtonSubtle;
    RLRectangle rcButtonDisabled;
    RLRectangle rcButtonTheme;

    // Flyout geometry (screen space)
    RLRectangle rcFlyoutPanel;
    RLRectangle rcFlyoutButton1;
    RLRectangle rcFlyoutButton2;

    int iFlyoutDirection;          // +1 = down, -1 = up

    // Layout hints
    float fLabelX;
    float fLabelWidth;
    float fLabelYOffset;
    bool bLabelsAbove;
    bool bNarrowLayout;
} UILayout;
static float ClampF(float x, float a, float b) { return (x < a) ? a : ((x > b) ? b : x); }
static float SnapPixel(float x) { return floorf(x + 0.5f); }
static int   RoundI(float x) { return (int)floorf(x + 0.5f); }

static float SmoothDamp(float fCurrent, float fTarget, float* pfCurrentVelocity, float fSmoothTime, float fMaxSpeed, float fDeltaTime)
{
    // Unity-style critically damped smoothing (good for ScrollViewer-like motion).
    if (fSmoothTime < 0.0001f) fSmoothTime = 0.0001f;
    float omega = 2.0f / fSmoothTime;
    float x = omega * fDeltaTime;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

    float change = fCurrent - fTarget;
    float originalTo = fTarget;

    // Clamp maximum change
    float maxChange = fMaxSpeed * fSmoothTime;
    change = ClampF(change, -maxChange, maxChange);
    fTarget = fCurrent - change;

    float temp = (*pfCurrentVelocity + omega * change) * fDeltaTime;
    *pfCurrentVelocity = (*pfCurrentVelocity - omega * temp) * exp;

    float output = fTarget + (change + temp) * exp;

    // Prevent overshooting
    if ((originalTo - fCurrent > 0.0f) == (output > originalTo))
    {
        output = originalTo;
        *pfCurrentVelocity = 0.0f;
    }

    return output;
}

// Text helpers (simple ASCII-safe utilities; good enough for demo UI strings)
static void RLStrCopy(char* dst, size_t cap, const char* src)
{
    if (!dst || cap == 0) return;
    if (!src) { dst[0] = 0; return; }
#if defined(_MSC_VER)
    strncpy_s(dst, cap, src, _TRUNCATE);
#else
    strncpy(dst, src, cap - 1);
    dst[cap - 1] = 0;
#endif
}

static void RLStrCopyCount(char* dst, size_t cap, const char* src, size_t count)
{
    if (!dst || cap == 0) return;
    if (!src) { dst[0] = 0; return; }
#if defined(_MSC_VER)
    strncpy_s(dst, cap, src, count);
#else
    if (count >= cap) count = cap - 1;
    memcpy(dst, src, count);
    dst[count] = 0;
#endif
}

static void RLStrCat(char* dst, size_t cap, const char* src)
{
    if (!dst || cap == 0 || !src) return;
    size_t len = strlen(dst);
    if (len >= cap - 1) return;
#if defined(_MSC_VER)
    strncat_s(dst, cap, src, _TRUNCATE);
#else
    strncat(dst, src, cap - len - 1);
    dst[cap - 1] = 0;
#endif
}
static void TextEllipsizeToFit(char* out, int outCap, const char* in, int fontSize, float maxW)
{
    if (!out || outCap <= 0) return;
    out[0] = 0;
    if (!in) return;

    if ((float)RLMeasureText(in, fontSize) <= maxW)
    {
        RLStrCopy(out, (size_t)outCap, in);
        out[outCap - 1] = 0;
        return;
    }

    const char* ell = "...";
    int ellW = RLMeasureText(ell, fontSize);
    if ((float)ellW >= maxW)
    {
        out[0] = 0;
        return;
    }

    int n = (int)strlen(in);
    int lo = 0, hi = n;

    while (lo < hi)
    {
        int mid = (lo + hi + 1) / 2;
        char tmp[256];
        if (mid > 240) mid = 240;
        RLStrCopyCount(tmp, sizeof(tmp), in, (size_t)mid);
        tmp[mid] = 0;
        RLStrCat(tmp, sizeof(tmp), ell);
        if ((float)RLMeasureText(tmp, fontSize) <= maxW) lo = mid;
        else hi = mid - 1;
    }

    int keep = lo;
    if (keep < 0) keep = 0;
    if (keep > outCap - 4) keep = outCap - 4;

    RLStrCopyCount(out, (size_t)outCap, in, (size_t)keep);
    out[keep] = 0;
    RLStrCat(out, (size_t)outCap, ell);
    out[outCap - 1] = 0;
}

static void AppendWord(char* dst, int cap, const char* word)
{
    if (!dst || cap <= 0 || !word) return;
    if (dst[0] == 0)
    {
        RLStrCopy(dst, (size_t)cap, word);
        dst[cap - 1] = 0;
    }
    else
    {
        size_t len = strlen(dst);
        if (len + 1 < (size_t)cap) { dst[len] = ' '; dst[len + 1] = 0; }
        RLStrCat(dst, (size_t)cap, word);
    }
}

static int WrapText2Lines(char line1[256], char line2[256], const char* text, int fontSize, float maxW)
{
    if (!line1 || !line2) return 0;
    line1[0] = 0;
    line2[0] = 0;
    if (!text) return 0;

    if ((float)RLMeasureText(text, fontSize) <= maxW)
    {
        RLStrCopy(line1, 256, text);
        line1[255] = 0;
        return 1;
    }

    int n = (int)strlen(text);
    int i = 0;
    int onLine = 1;
    char word[128];

    while (i < n)
    {
        while (i < n && text[i] == ' ') i++;
        if (i >= n) break;

        int wlen = 0;
        while (i < n && text[i] != ' ' && wlen < (int)sizeof(word) - 1)
        {
            word[wlen++] = text[i++];
        }
        word[wlen] = 0;

        if (onLine == 1)
        {
            char cand[256];
            RLStrCopy(cand, 256, line1);
            AppendWord(cand, 256, word);

            if ((float)RLMeasureText(cand, fontSize) <= maxW)
            {
                AppendWord(line1, 256, word);
            }
            else
            {
                onLine = 2;
                AppendWord(line2, 256, word);
            }
        }
        else
        {
            char cand[256];
            RLStrCopy(cand, 256, line2);
            AppendWord(cand, 256, word);

            if ((float)RLMeasureText(cand, fontSize) <= maxW)
            {
                AppendWord(line2, 256, word);
            }
            else
            {
                // Not enough room; ellipsize line2 with the extra word and stop.
                char tmp[256];
                RLStrCopy(tmp, 256, line2);
                AppendWord(tmp, 256, word);
                TextEllipsizeToFit(line2, 256, tmp, fontSize, maxW);
                return 2;
            }
        }
    }

    if (line2[0] != 0 && (float)RLMeasureText(line2, fontSize) > maxW)
    {
        char tmp[256];
        RLStrCopy(tmp, 256, line2);
        TextEllipsizeToFit(line2, 256, tmp, fontSize, maxW);
    }

    return (line2[0] != 0) ? 2 : 1;
}


static UILayout ComputeLayout(int iScreenWidth, int iScreenHeight, float fTopY)
{
    UILayout lytLayout = { 0 };

    // Page metrics
    const float pageMarginX = 28.0f;
    const float pageMarginBottom = 28.0f;

    // Responsive container sizing (WinUI-like: grows with the window, capped for readability).
    float availW = (float)iScreenWidth - pageMarginX * 2.0f;
    if (availW < 160.0f) availW = 160.0f;
    float cardMaxW = fminf(1280.0f, availW);
    float cardMinW = fminf(420.0f, cardMaxW);
    float cardW = ClampF(availW, cardMinW, cardMaxW);

    float availH = (float)iScreenHeight - fTopY - pageMarginBottom;
    if (availH < 140.0f) availH = 140.0f;
    float cardMaxH = fminf(760.0f, availH);
    float cardMinH = fminf(280.0f, cardMaxH);
    float cardH = ClampF(availH, cardMinH, cardMaxH);

    // Pixel-snap the container rect to avoid scissor/round-rect shimmer when resizing.
    cardW = SnapPixel(cardW);
    cardH = SnapPixel(cardH);

    // Align the card with the header text grid (left margin), like Fluent pages.
    float cardX = SnapPixel(pageMarginX);
    float cardY = SnapPixel(fTopY);
    lytLayout.rcCard = (RLRectangle){ cardX, cardY, cardW, cardH };

    // In-card geometry
    const float insetL = 18.0f;
    const float insetR = 22.0f;    // Reserve space for scrollbar
    const float insetT = 16.0f;
    const float insetB = 16.0f;

    const float footerReserved = 66.0f; // Fixed footer height inside the card
    const float footerPadT = 10.0f;
    const float footerPadB = 10.0f;
    float clipX = SnapPixel(cardX + insetL);
    float clipY = SnapPixel(cardY + insetT);
    float clipW = cardW - insetL - insetR;
    float clipH = cardH - insetT - insetB - footerReserved;
    if (clipW < 80.0f) clipW = 80.0f;
    if (clipH < 80.0f) clipH = 80.0f;
    clipW = SnapPixel(clipW);
    clipH = SnapPixel(clipH);

    lytLayout.rcContentClip = (RLRectangle){ clipX, clipY, clipW, clipH };

    float footH = footerReserved - footerPadT - footerPadB;
    if (footH < 28.0f) footH = 28.0f;
    float footY = SnapPixel(cardY + cardH - footerReserved + footerPadT);
    lytLayout.rcFooter = (RLRectangle){ clipX, footY, clipW, SnapPixel(footH) };

    // Inner padding inside the scroll viewport
    const float innerPadX = 10.0f;
    const float innerPadY = 10.0f;
    float x0 = lytLayout.rcContentClip.x + innerPadX;
    float y0 = lytLayout.rcContentClip.y + innerPadY;
    float w0 = lytLayout.rcContentClip.width - innerPadX * 2.0f;
    if (w0 < 120.0f) w0 = 120.0f;

    const float btnH = 54.0f;
    const float rowGap = 18.0f;
    const float labelFontSize = 14.0f;

    // Default title positions (unscrolled)
    lytLayout.v2VariantsTitlePos = (RLVector2){ x0, y0 };

    // Decide layout mode
    bool useColumns = (w0 >= 640.0f);

    float colGap = 32.0f;
    float rightW = ClampF(w0 * 0.40f, 260.0f, 320.0f);
    float leftW = w0 - colGap - rightW;

    if (useColumns && leftW < 360.0f) useColumns = false;

    float contentBottom = y0;

    if (useColumns)
    {
        lytLayout.bNarrowLayout = false;
        lytLayout.bLabelsAbove = false;

        float leftX = x0;
        float rightX = x0 + leftW + colGap;

        // Theme header in the same top row
        lytLayout.v2ThemeTitlePos = (RLVector2){ rightX, y0 };
        lytLayout.v2ThemeStatePos = (RLVector2){ rightX, y0 + 30.0f };

        // Left column: labels and buttons
        float labelW = ClampF(leftW * 0.30f, 92.0f, 130.0f);
        float gap = 18.0f;
        float btnX = leftX + labelW + gap;
        float btnW = leftW - labelW - gap;
        if (btnW < 180.0f) btnW = 180.0f;

        float y = y0 + 38.0f;

        lytLayout.fLabelX = leftX;
        lytLayout.fLabelWidth = labelW;
        lytLayout.fLabelYOffset = (btnH - labelFontSize) * 0.5f - 1.0f;

        lytLayout.rcButtonOpen = (RLRectangle){ btnX, y, btnW, btnH }; y += btnH + rowGap;
        lytLayout.rcButtonPrimary = (RLRectangle){ btnX, y, btnW, btnH }; y += btnH + rowGap;
        lytLayout.rcButtonSubtle = (RLRectangle){ btnX, y, btnW, btnH }; y += btnH + rowGap;
        lytLayout.rcButtonDisabled = (RLRectangle){ btnX, y, btnW, btnH };

        float leftBottom = lytLayout.rcButtonDisabled.y + lytLayout.rcButtonDisabled.height;

        // Theme button in right column
        float themeBtnY = y0 + 54.0f;
        float themeBtnW = ClampF(rightW, 240.0f, 320.0f);
        lytLayout.rcButtonTheme = (RLRectangle){ rightX, themeBtnY, themeBtnW, btnH };
        float rightBottom = lytLayout.rcButtonTheme.y + lytLayout.rcButtonTheme.height;

        contentBottom = fmaxf(leftBottom, rightBottom) + 22.0f;
    }
    else
    {
        lytLayout.bNarrowLayout = true;
        lytLayout.bLabelsAbove = true;

        float y = y0 + 30.0f;

        float btnW = w0;
        float btnX = x0;
        lytLayout.fLabelX = x0;
        lytLayout.fLabelWidth = 0.0f;
        lytLayout.fLabelYOffset = -22.0f;

        // Default
        lytLayout.rcButtonOpen = (RLRectangle){ btnX, y + 22.0f, btnW, btnH };
        y = lytLayout.rcButtonOpen.y + btnH + rowGap;

        // Primary
        lytLayout.rcButtonPrimary = (RLRectangle){ btnX, y + 22.0f, btnW, btnH };
        y = lytLayout.rcButtonPrimary.y + btnH + rowGap;

        // Subtle
        lytLayout.rcButtonSubtle = (RLRectangle){ btnX, y + 22.0f, btnW, btnH };
        y = lytLayout.rcButtonSubtle.y + btnH + rowGap;

        // Disabled
        lytLayout.rcButtonDisabled = (RLRectangle){ btnX, y + 22.0f, btnW, btnH };
        y = lytLayout.rcButtonDisabled.y + btnH + 24.0f;

        // Theme section
        lytLayout.v2ThemeTitlePos = (RLVector2){ x0, y };
        y += 30.0f;
        lytLayout.v2ThemeStatePos = (RLVector2){ x0, y };
        y += 22.0f;
        lytLayout.rcButtonTheme = (RLRectangle){ btnX, y, btnW, btnH };
        y = lytLayout.rcButtonTheme.y + btnH;

        contentBottom = y + 22.0f;
    }

    // Scroll metrics
    lytLayout.fContentHeight = contentBottom - lytLayout.rcContentClip.y;
    float viewBottom = lytLayout.rcContentClip.y + lytLayout.rcContentClip.height;
    lytLayout.fScrollMax = (contentBottom > viewBottom) ? (contentBottom - viewBottom) : 0.0f;
    // Flyout: anchored to Open Menu button (initial estimate; final placement is recomputed after scrolling).
    float flyW = 300.0f;
    float flyH = 150.0f;
    float flyX = lytLayout.rcButtonOpen.x + (lytLayout.rcButtonOpen.width - flyW) * 0.5f;
    float flyY = lytLayout.rcButtonOpen.y + lytLayout.rcButtonOpen.height + 10.0f;
    int flyDir = +1;
    if (flyX < lytLayout.rcCard.x + 18.0f) flyX = lytLayout.rcCard.x + 18.0f;
    if (flyX + flyW > lytLayout.rcCard.x + lytLayout.rcCard.width - 18.0f) flyX = lytLayout.rcCard.x + lytLayout.rcCard.width - 18.0f - flyW;
    if (flyY + flyH > lytLayout.rcCard.y + lytLayout.rcCard.height - 18.0f) { flyY = lytLayout.rcButtonOpen.y - 10.0f - flyH; flyDir = -1; }

    lytLayout.rcFlyoutPanel = (RLRectangle){ flyX, flyY, flyW, flyH };
    lytLayout.rcFlyoutButton1 = (RLRectangle){ flyX + 18.0f, flyY + flyH - 56.0f, flyW - 36.0f, 44.0f };
    lytLayout.rcFlyoutButton2 = (RLRectangle){ flyX + 18.0f, flyY + 18.0f,          flyW - 36.0f, 44.0f };
    lytLayout.iFlyoutDirection = flyDir;

    return lytLayout;
}


static RLRectangle RectIntersect(RLRectangle rcA, RLRectangle rcB)
{
    float x1 = fmaxf(rcA.x, rcB.x);
    float y1 = fmaxf(rcA.y, rcB.y);
    float x2 = fminf(rcA.x + rcA.width, rcB.x + rcB.width);
    float y2 = fminf(rcA.y + rcA.height, rcB.y + rcB.height);
    RLRectangle rcRect = { x1, y1, x2 - x1, y2 - y1 };
    if (rcRect.width <= 0.0f || rcRect.height <= 0.0f) return (RLRectangle) { 0 };
    return rcRect;
}

static void ApplyScrollToLayout(UILayout* lytLayout, float fScrollY)
{
    lytLayout->v2VariantsTitlePos.y -= fScrollY;
    lytLayout->v2ThemeTitlePos.y -= fScrollY;
    lytLayout->v2ThemeStatePos.y -= fScrollY;

    lytLayout->rcButtonOpen.y -= fScrollY;
    lytLayout->rcButtonPrimary.y -= fScrollY;
    lytLayout->rcButtonSubtle.y -= fScrollY;
    lytLayout->rcButtonDisabled.y -= fScrollY;
    lytLayout->rcButtonTheme.y -= fScrollY;
}


static void RecomputeFlyoutLayout(UILayout* lytLayout, int iScreenWidth, int iScreenHeight)
{
    // Flyout anchored to Open button (recomputed after scrolling so the anchor stays correct).
    // Placement: prefer opening upward when it fits. Bounds can be constrained to the card
    // or allowed to escape to the root/window bounds (WinUI-like).
    float flyW = ClampF(lytLayout->rcCard.width * 0.42f, 280.0f, 380.0f);
    float flyH = 196.0f;
    float gap = 10.0f;
    float pad = 16.0f;
    float m = 8.0f;

    RLRectangle cardInner = (RLRectangle){
        lytLayout->rcCard.x + pad,
        lytLayout->rcCard.y + pad,
        lytLayout->rcCard.width - pad * 2.0f,
        lytLayout->rcCard.height - pad * 2.0f
    };

    RLRectangle rootInner = (RLRectangle){
        m, m,
        (float)iScreenWidth - m * 2.0f,
        (float)iScreenHeight - m * 2.0f
    };

    RLRectangle bounds = gFlyoutConstrainToCard ? cardInner : rootInner;

    // Anchor center horizontally.
    float flyX = lytLayout->rcButtonOpen.x + (lytLayout->rcButtonOpen.width - flyW) * 0.5f;

    float anchorTop = lytLayout->rcButtonOpen.y;
    float anchorBottom = lytLayout->rcButtonOpen.y + lytLayout->rcButtonOpen.height;
    float spaceAbove = anchorTop - bounds.y;
    float spaceBelow = (bounds.y + bounds.height) - anchorBottom;

    int dir = +1;
    float flyY = anchorBottom + gap;

    // Prefer up if it fits.
    if (spaceAbove >= flyH + gap)
    {
        dir = -1;
        flyY = anchorTop - gap - flyH;
    }
    else if (spaceBelow >= flyH + gap)
    {
        dir = +1;
        flyY = anchorBottom + gap;
    }
    else
    {
        // Neither fits fully: choose the side with more room and clamp.
        if (spaceAbove >= spaceBelow)
        {
            dir = -1;
            flyY = bounds.y;
        }
        else
        {
            dir = +1;
            flyY = bounds.y + bounds.height - flyH;
        }
    }

    // Clamp inside placement bounds.
    if (flyX < bounds.x) flyX = bounds.x;
    if (flyX + flyW > bounds.x + bounds.width) flyX = bounds.x + bounds.width - flyW;
    if (flyY < bounds.y) flyY = bounds.y;
    if (flyY + flyH > bounds.y + bounds.height) flyY = bounds.y + bounds.height - flyH;

    // Final clamp to window (safety for extreme resize).
    if (flyX < m) flyX = m;
    if (flyX + flyW > (float)iScreenWidth - m) flyX = (float)iScreenWidth - m - flyW;
    if (flyY < m) flyY = m;
    if (flyY + flyH > (float)iScreenHeight - m) flyY = (float)iScreenHeight - m - flyH;

    lytLayout->iFlyoutDirection = dir;
    lytLayout->rcFlyoutPanel = (RLRectangle){ flyX, flyY, flyW, flyH };

    float innerPad = 16.0f;
    float flyBtnH = 48.0f;
    lytLayout->rcFlyoutButton1 = (RLRectangle){ flyX + innerPad, flyY + 78.0f, flyW - innerPad * 2.0f, flyBtnH };
    lytLayout->rcFlyoutButton2 = (RLRectangle){ flyX + innerPad, flyY + 134.0f, flyW - innerPad * 2.0f, flyBtnH };
}


static void AddHitClipped(UIHitItem* aHitItems, int* piHitItemCount, int iId, RLRectangle rcItem, int iZOrder, RLRectangle rcClip)
{
    if (iId < 0) return;
    RLRectangle rcRect = RectIntersect(rcItem, rcClip);
    if (rcRect.width <= 0.0f || rcRect.height <= 0.0f) return;
    aHitItems[(*piHitItemCount)++] = (UIHitItem){ iId, rcRect, iZOrder };
}

// Time-based smoothing (Unity-style SmoothDamp) – great for UI motion.
static float SmoothDampF(float fCurrent, float fTarget, float* pfCurrentVelocity, float fSmoothTime, float fMaxSpeed, float fDeltaTime)
{
    if (fSmoothTime < 0.0001f) fSmoothTime = 0.0001f;
    float omega = 2.0f / fSmoothTime;

    float x = omega * fDeltaTime;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

    float change = fCurrent - fTarget;
    float originalTo = fTarget;

    float maxChange = fMaxSpeed * fSmoothTime;
    change = ClampF(change, -maxChange, maxChange);
    fTarget = fCurrent - change;

    float temp = (*pfCurrentVelocity + omega * change) * fDeltaTime;
    *pfCurrentVelocity = (*pfCurrentVelocity - omega * temp) * exp;

    float output = fTarget + (change + temp) * exp;

    // Prevent overshoot.
    if ((originalTo - fCurrent > 0.0f) == (output > originalTo))
    {
        output = originalTo;
        *pfCurrentVelocity = (output - originalTo) / fmaxf(fDeltaTime, 0.0001f);
    }

    return output;
}

// --------------------------------------------------------------------------------------
// Buttons
// --------------------------------------------------------------------------------------

typedef struct FluentButtonState {
    float hover;   // 0..1
    float press;   // 0..1
    float focus;   // 0..1
    float rippleT; // 0..1
    RLVector2 ripplePos;
    bool rippleActive;
} FluentButtonState;

typedef enum FluentButtonKind {
    FLUENT_BTN_DEFAULT = 0,
    FLUENT_BTN_PRIMARY,
    FLUENT_BTN_SUBTLE,
} FluentButtonKind;

// Returns true when activated (mouse click or keyboard action).
static bool DrawFluentButtonEx(int id, RLRectangle rcRect, const char* text,
    FluentButtonKind kind, bool enabled,
    FluentButtonState* st, int* iFocusId,
    const ThemeTokens* ttTheme, float fThemeLightness,
    const AcrylicCtx* ac, RLTexture2D acrylicBackdrop,
    float fTimeSeconds, float acrylicSoften,
    const UIInput* uiInput)
{
    RLVector2 v2MousePos = uiInput->mouse;
    bool hot = (uiInput->hotId == id);

    // Focus handling: click to focus.
    if (enabled && hot && uiInput->pressed) *iFocusId = id;
    bool focused = (*iFocusId == id);

    bool down = enabled && (uiInput->activeId == id) && uiInput->down;
    bool clickedMouse = enabled && uiInput->released && (uiInput->activeId == id) && hot;

    bool clickedKey = false;
    if (enabled && focused)
    {
        if (RLIsKeyPressed(KEY_ENTER) || RLIsKeyPressed(KEY_KP_ENTER) || RLIsKeyPressed(KEY_SPACE))
        {
            clickedKey = true;
            st->ripplePos = (RLVector2){ rcRect.x + rcRect.width * 0.5f, rcRect.y + rcRect.height * 0.5f };
            st->rippleActive = true;
            st->rippleT = 0.0f;
        }
    }

    if (enabled && hot && uiInput->pressed)
    {
        st->ripplePos = v2MousePos;
        st->rippleActive = true;
        st->rippleT = 0.0f;
    }

    float fDeltaTime = RLGetFrameTime();
    float hoverTarget = (enabled && hot) ? 1.0f : 0.0f;
    float pressTarget = down ? 1.0f : 0.0f;
    float focusTarget = focused ? 1.0f : 0.0f;

    st->hover = ApproachExp(st->hover, hoverTarget, 14.0f, fDeltaTime);
    st->press = ApproachExp(st->press, pressTarget, 22.0f, fDeltaTime);
    st->focus = ApproachExp(st->focus, focusTarget, 18.0f, fDeltaTime);

    float h = Smoothstep(st->hover);
    float p = EaseOutCubic(st->press);
    float f = Smoothstep(st->focus);

    if (st->rippleActive)
    {
        st->rippleT += fDeltaTime / 0.28f;
        if (st->rippleT >= 1.0f) { st->rippleT = 1.0f; st->rippleActive = false; }
    }

    // Layout micro-interactions
    float radius = 8.0f;
    float scale = 1.0f + 0.015f * h - 0.010f * p;
    RLRectangle rr = rcRect;
    rr.x = rcRect.x + (rcRect.width - rcRect.width * scale) * 0.5f;
    rr.y = rcRect.y + (rcRect.height - rcRect.height * scale) * 0.5f + 1.0f * p;
    rr.width *= scale;
    rr.height *= scale;

    // Colors by variant
    RLColor bg = ttTheme->surface;
    RLColor border = ttTheme->border;
    RLColor label = ttTheme->text;
    RLColor overlay = (RLColor){ 255, 255, 255, 0 };

    if (!enabled)
    {
        label = ttTheme->disabledText;
        border.a = (unsigned char)(border.a * 0.6f);
        if (kind == FLUENT_BTN_PRIMARY) bg = ttTheme->surface2;
    }
    else if (kind == FLUENT_BTN_PRIMARY)
    {
        bg = ttTheme->accent;
        label = (RLColor){ 255, 255, 255, 255 };
        overlay = (RLColor){ 255, 255, 255, (unsigned char)(20 * h) };
        border = (RLColor){ 255, 255, 255, (unsigned char)(50 + 60 * h + 80 * f) };
    }
    else if (kind == FLUENT_BTN_SUBTLE)
    {
        // Transparent-ish base.
        bg = (RLColor){ ttTheme->surface.r, ttTheme->surface.g, ttTheme->surface.b, (unsigned char)(0 + 180 * h + 140 * f) };
        border = (RLColor){ ttTheme->borderHot.r, ttTheme->borderHot.g, ttTheme->borderHot.b, (unsigned char)(20 + 70 * h + 90 * f) };
    }
    else
    {
        bg = ttTheme->surface;
        RLColor ovDark = (RLColor){ 255, 255, 255, (unsigned char)(10 * h) };
        RLColor ovLight = (RLColor){ 0, 0, 0, (unsigned char)(12 * h) };
        overlay = LerpColor(ovDark, ovLight, Clamp01(fThemeLightness));
        border = (RLColor){ ttTheme->border.r, ttTheme->border.g, ttTheme->border.b, (unsigned char)(ttTheme->border.a + 40 * h + 90 * f) };
    }

    // Elevation
    float elev = 0.0f;
    if (enabled)
    {
        elev = (kind == FLUENT_BTN_SUBTLE) ? 0.0f : Clamp01(0.10f + 0.55f * h - 0.40f * p);
    }
    else
    {
        elev = (kind == FLUENT_BTN_SUBTLE) ? 0.0f : 0.05f;
    }

    if (elev > 0.01f) DrawSoftShadowRounded(rr, radius, elev, ttTheme);

    float round = RoundnessForRadius(rr.width, rr.height, radius);
    int seg = 12;

    // --- Fill ---
    bool canAcrylicBtn = enabled && gAcrylicEnabled && gAcrylicButtons && (kind == FLUENT_BTN_DEFAULT);
    if (canAcrylicBtn)
    {
        // Use theme surface2 as tint color; amount is tuned per theme.
        RLColor tintRgb = ttTheme->surface2;
        float tintAmount = LerpF(0.52f, 0.64f, Clamp01(fThemeLightness));
        float grain = gAcrylicNoise ? LerpF(0.014f, 0.022f, Clamp01(fThemeLightness)) : 0.0f;
        DrawAcrylicRounded(ac, acrylicBackdrop, rr, radius, tintRgb, tintAmount, grain, fTimeSeconds, acrylicSoften);
    }
    else
    {
        RLDrawRectangleRounded(rr, round, seg, bg);
    }

    if (overlay.a > 0) RLDrawRectangleRounded(rr, round, seg, overlay);
    RLDrawRectangleRoundedLines(rr, round, seg, border);

    // Reveal
    if (enabled) DrawRevealGradientClipped(rr, v2MousePos, Clamp01(h * 0.9f + f * 0.25f), fThemeLightness);

    // Ripple
    if (enabled && (st->rippleActive || st->rippleT > 0.0f))
    {
        float t = Clamp01(st->rippleT);
        float e = EaseOutCubic(t);
        float maxRad = sqrtf(rr.width * rr.width + rr.height * rr.height);
        float rad = 10.0f + maxRad * e;
        RLColor c = (kind == FLUENT_BTN_PRIMARY) ? (RLColor) { 255, 255, 255, 60 } : ttTheme->accent2;
        c.a = (unsigned char)(55.0f * (1.0f - t));
        PushScissorRect(rr, 0);
        RLDrawCircleV(st->ripplePos, rad, c);
        PopScissor();
    }

    // Focus ring
    if (enabled && f > 0.001f)
    {
        RLRectangle rcFocusRect = rr;
        rcFocusRect.x -= 2; rcFocusRect.y -= 2; 
        rcFocusRect.width += 4; rcFocusRect.height += 4;
        RLColor ring = ttTheme->accent2;
        ring.a = (unsigned char)(160.0f * f);
        RLDrawRectangleRoundedLines(rcFocusRect, RoundnessForRadius(rcFocusRect.width, rcFocusRect.height, radius + 2.0f), seg, ring);
    }

    // Label
    int fontSize = 18;
    float maxLabelW = rr.width - 24.0f;
    if (maxLabelW < 10.0f) maxLabelW = 10.0f;

    char labelText[128];
    TextEllipsizeToFit(labelText, (int)sizeof(labelText), text, fontSize, maxLabelW);

    int tw = RLMeasureText(labelText, fontSize);
    int tx = (int)(rr.x + rr.width * 0.5f - tw * 0.5f);
    int ty = (int)(rr.y + rr.height * 0.5f - fontSize * 0.6f);

    if (!enabled) label.a = (unsigned char)(label.a * 0.90f);
    RLDrawText(labelText, tx, ty, fontSize, label);
    return clickedMouse || clickedKey;
}

// --------------------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------------------

static void FocusCycle(const int* piFocusOrder, int iCount, int iDirection, int* piFocusId)
{
    if (iCount <= 0) return;

    int cur = -1;
    for (int i = 0; i < iCount; i++) if (piFocusOrder[i] == *piFocusId) { cur = i; break; }

    if (cur < 0) cur = (iDirection > 0) ? 0 : (iCount - 1);
    else cur = (cur + iDirection + iCount) % iCount;

    *piFocusId = piFocusOrder[cur];
}


typedef struct ScrollState {
    float pos;
    float target;
    float vel;
    float dragGrabY;
    bool dragging;
} ScrollState;

static void ComputeScrollbarRects(const UILayout* pLayoutBase, float fScrollPosition,
    RLRectangle* prcOutTrack, RLRectangle* prcOutThumb)
{
    *prcOutTrack = (RLRectangle){ 0 };
    *prcOutThumb = (RLRectangle){ 0 };
    if (pLayoutBase->fScrollMax <= 0.01f) return;

    float fTrackX = pLayoutBase->rcCard.x + pLayoutBase->rcCard.width - 14.0f;
    float fTrackY = pLayoutBase->rcContentClip.y;
    float fTrackHeight = pLayoutBase->rcContentClip.height;
    float fTrackWidth = 10.0f;

    float fT = (pLayoutBase->fScrollMax > 0.0f) ? ClampF(fScrollPosition / pLayoutBase->fScrollMax, 0.0f, 1.0f) : 0.0f;
    float fThumbHeight = fmaxf(34.0f, fTrackHeight * (fTrackHeight / (fTrackHeight + pLayoutBase->fScrollMax)));
    if (fThumbHeight > fTrackHeight) fThumbHeight = fTrackHeight;
    float fThumbY = fTrackY + (fTrackHeight - fThumbHeight) * fT;

    *prcOutTrack = (RLRectangle){ fTrackX, fTrackY, fTrackWidth, fTrackHeight };
    *prcOutThumb = (RLRectangle){ fTrackX, fThumbY, fTrackWidth, fThumbHeight };
}


int main(void)
{
    RLSetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_EVENT_THREAD);
    RLInitWindow(900, 540, "raylib [core] example - fluent button");
    RLSetTargetFPS(144);

    // Theme transition state
    ThemeTokens ttThemeDark = ThemeDark();
    ThemeTokens ttThemeLight = ThemeLight();
    bool bThemeTargetLight = false;
    float fThemeMix = 0.0f; // 0=dark, 1=light

    // Background image and blur state
    bool bBackgroundUseImage = false;
    bool bBackgroundUseBlur = true;
    int iBackgroundBlurLevel = 3; // 1..5

    RLTexture2D texBackground = (RLTexture2D){ 0 };
    const char* pszBackgroundPath = NULL;
    bool bBackgroundLoaded = TryLoadBackgroundTexture(&texBackground, &pszBackgroundPath);

    RLRenderTexture2D rtBackgroundBlur = (RLRenderTexture2D){ 0 };
    int iBackgroundBlurWidth = 0, iBackgroundBlurHeight = 0;

    // Acrylic
    AcrylicCtx acxAcrylic = { 0 };
    AcrylicInit(&acxAcrylic);

    RLRenderTexture2D rtAcrylicBackdrop = (RLRenderTexture2D){ 0 };
    int iAcrylicWidth = 0, iAcrylicHeight = 0;

    // Buttons
    FluentButtonState fbsDefault = { 0 };
    FluentButtonState fbsPrimary = { 0 };
    FluentButtonState fbsSubtle = { 0 };
    FluentButtonState fbsDisabled = { 0 };
    FluentButtonState fbsThemeToggle = { 0 };

    // Flyout
    bool bFlyoutWanted = false;
    float fFlyoutAnimT = 0.0f;        // 0..1
    float fFlyoutAnimVel = 0.0f;      // smoothing velocity
    int iFocusBeforeFlyout = -1;
    FluentButtonState fbsFlyoutAction1 = { 0 };
    FluentButtonState fbsFlyoutAction2 = { 0 };

    int iClicks = 0;
    int iFocusId = -1;
    ScrollState ssCardScroll = { 0 }; // scroll state for card content
    float fScrollbarVisibility = 0.0f;    // 0..1 scrollbar visibility
    float fScrollbarWakeSeconds = 0.0f;   // seconds to keep visible after interaction
    float fScrollbarThumbWidth = 6.0f; // animated thumb width
    float fTimeSeconds = 0.0f;

    while (!RLWindowShouldClose())
    {
        float fDeltaTime = RLGetFrameTime();
        fTimeSeconds += fDeltaTime;

        // Global toggles
        if (RLIsKeyPressed(KEY_T)) bThemeTargetLight = !bThemeTargetLight;
        if (RLIsKeyPressed(KEY_B)) bBackgroundUseImage = !bBackgroundUseImage;
        if (RLIsKeyPressed(KEY_V)) bBackgroundUseBlur = !bBackgroundUseBlur;
        if (RLIsKeyPressed(KEY_A)) gAcrylicEnabled = !gAcrylicEnabled;
        if (RLIsKeyPressed(KEY_N)) gAcrylicNoise = !gAcrylicNoise;
        if (RLIsKeyPressed(KEY_M)) gAcrylicButtons = !gAcrylicButtons;

        if (RLIsKeyPressed(KEY_C)) gFlyoutConstrainToCard = !gFlyoutConstrainToCard;
        if (RLIsKeyPressed(KEY_LEFT_BRACKET))
        {
            iBackgroundBlurLevel--;
            if (iBackgroundBlurLevel < 1) iBackgroundBlurLevel = 1;
            iBackgroundBlurWidth = 0; iBackgroundBlurHeight = 0;
            iAcrylicWidth = 0; iAcrylicHeight = 0;
        }
        if (RLIsKeyPressed(KEY_RIGHT_BRACKET))
        {
            iBackgroundBlurLevel++;
            if (iBackgroundBlurLevel > 5) iBackgroundBlurLevel = 5;
            iBackgroundBlurWidth = 0; iBackgroundBlurHeight = 0;
            iAcrylicWidth = 0; iAcrylicHeight = 0;
        }

        // Smooth theme
        fThemeMix = ApproachExp(fThemeMix, bThemeTargetLight ? 1.0f : 0.0f, 7.0f, fDeltaTime);
        float fThemeLightness = Smoothstep(fThemeMix);
        ThemeTokens ttTheme = LerpTheme(ttThemeDark, ttThemeLight, fThemeLightness);

        // Close flyout on ESC
        if ((bFlyoutWanted || fFlyoutAnimT > 0.01f) && RLIsKeyPressed(KEY_ESCAPE))
        {
            bFlyoutWanted = false;
        }
        // Layout (responsive to current window size)
        int iScreenWidth = RLGetScreenWidth();
        int iScreenHeight = RLGetScreenHeight();

        // Compute a safe card top offset based on the header and status block.
        // Those lines are intentionally single-line (ellipsized), so this stays stable and avoids overlap.
        float fHeaderBottomY = 112.0f + 14.0f;
        if (bBackgroundUseImage && !bBackgroundLoaded) fHeaderBottomY = 130.0f + 14.0f;
        float fCardTopY = fHeaderBottomY + 24.0f;
        UILayout lytLayoutBase = ComputeLayout(iScreenWidth, iScreenHeight, fCardTopY);

        // Determine flyout visibility from previous state (for hit-testing this frame)
        bool bFlyoutVisible = bFlyoutWanted || (fFlyoutAnimT > 0.01f);

        // ScrollViewer (wheel inertia and scrollbar).
        RLVector2 v2MousePos = RLGetMousePosition();
        RLRectangle rcScrollbarHotZone = (RLRectangle){ lytLayoutBase.rcCard.x + lytLayoutBase.rcCard.width - 18.0f, lytLayoutBase.rcContentClip.y, 18.0f, lytLayoutBase.rcContentClip.height };
        float fWheelDelta = 0.0f;
        bool bScrolledThisFrame = false;

        float fScrollMax = lytLayoutBase.fScrollMax;
        if (fScrollMax <= 0.01f)
        {
            ssCardScroll.pos = 0.0f;
            ssCardScroll.target = 0.0f;
            ssCardScroll.vel = 0.0f;
            ssCardScroll.dragging = false;
        }
        else
        {
            ssCardScroll.pos = ClampF(ssCardScroll.pos, 0.0f, fScrollMax);
            ssCardScroll.target = ClampF(ssCardScroll.target, 0.0f, fScrollMax);

            if (!bFlyoutVisible)
            {
                // Accept wheel scrolling when the cursor is over the viewport or near the scrollbar edge.
                if (RLCheckCollisionPointRec(v2MousePos, lytLayoutBase.rcContentClip) || RLCheckCollisionPointRec(v2MousePos, rcScrollbarHotZone))
                {
                    float wheel = RLGetMouseWheelMove();
                    if (wheel != 0.0f)
                    {
                        fWheelDelta = wheel; // wheel notches (positive = up)
                        bScrolledThisFrame = true;
                    }
                }
            }
        }

        float fScrollPositionDraw = ssCardScroll.pos;

        UILayout lytLayout = lytLayoutBase;
        ApplyScrollToLayout(&lytLayout, fScrollPositionDraw);
        RecomputeFlyoutLayout(&lytLayout, iScreenWidth, iScreenHeight);

        // Scrollbar geometry (for hit-testing and drawing)
        RLRectangle rcScrollbarTrack = (RLRectangle){ 0 };
        RLRectangle rcScrollbarThumb = (RLRectangle){ 0 };
        ComputeScrollbarRects(&lytLayoutBase, fScrollPositionDraw, &rcScrollbarTrack, &rcScrollbarThumb);

        float fFlyoutAlphaPrev = Smoothstep(fFlyoutAnimT);
        float fFlyoutLiftPrev = (1.0f - fFlyoutAlphaPrev) * 10.0f;
        int iFlyoutDirPrev = (lytLayout.iFlyoutDirection == 0) ? +1 : lytLayout.iFlyoutDirection;
        float fFlyoutOffsetPrev = -fFlyoutLiftPrev * (float)iFlyoutDirPrev;

        RLRectangle rcFlyoutPanelDrawPrev = lytLayout.rcFlyoutPanel;
        rcFlyoutPanelDrawPrev.y += fFlyoutOffsetPrev;

        RLRectangle rcFlyoutButton1DrawPrev = lytLayout.rcFlyoutButton1;
        RLRectangle rcFlyoutButton2DrawPrev = lytLayout.rcFlyoutButton2;
        rcFlyoutButton1DrawPrev.y += fFlyoutOffsetPrev;
        rcFlyoutButton2DrawPrev.y += fFlyoutOffsetPrev;

        // Build hit-test list (virtual Z)
        UIHitItem aHitItems[16];
        int iHitItemCount = 0;

        // Base controls (clipped to card content viewport)
        RLRectangle rcClip = lytLayout.rcContentClip;
        int iZOpen = bFlyoutVisible ? 96 : 20; // allow clicking anchor to close when flyout is open
        AddHitClipped(aHitItems, &iHitItemCount, 0, lytLayout.rcButtonOpen, iZOpen, rcClip);
        AddHitClipped(aHitItems, &iHitItemCount, 1, lytLayout.rcButtonPrimary, 20, rcClip);
        AddHitClipped(aHitItems, &iHitItemCount, 2, lytLayout.rcButtonSubtle, 20, rcClip);
        // Disabled button: omit from hit-test
        AddHitClipped(aHitItems, &iHitItemCount, 4, lytLayout.rcButtonTheme, 20, rcClip);
        if (!bFlyoutVisible && lytLayoutBase.fScrollMax > 0.01f)
        {
            bool bScrollbarHitEnable = (fScrollbarVisibility > 0.04f) || ssCardScroll.dragging || RLCheckCollisionPointRec(v2MousePos, rcScrollbarHotZone);
            if (bScrollbarHitEnable)
            {
                aHitItems[iHitItemCount++] = (UIHitItem){ UI_ID_SCROLL_TRACK, rcScrollbarTrack, 30 };
                aHitItems[iHitItemCount++] = (UIHitItem){ UI_ID_SCROLL_THUMB, rcScrollbarThumb, 31 };
            }
        }
        if (bFlyoutVisible)
        {
            aHitItems[iHitItemCount++] = (UIHitItem){ UI_ID_SCRIM, (RLRectangle) { 0, 0, (float)iScreenWidth, (float)iScreenHeight }, 90 };
            aHitItems[iHitItemCount++] = (UIHitItem){ UI_ID_FLY_PANEL, rcFlyoutPanelDrawPrev, 100 };
            aHitItems[iHitItemCount++] = (UIHitItem){ 100, rcFlyoutButton1DrawPrev, 110 };
            aHitItems[iHitItemCount++] = (UIHitItem){ 101, rcFlyoutButton2DrawPrev, 110 };
        }

        UIInput uiInput = { 0 };
        UIInputBegin(&uiInput, aHitItems, iHitItemCount);

        // Scrollbar interactions and wheel inertia
        if (!bFlyoutVisible && lytLayoutBase.fScrollMax > 0.01f)
        {
            bool scrollTouched = bScrolledThisFrame;

            // Start dragging from the thumb
            if (uiInput.pressed && uiInput.hotId == UI_ID_SCROLL_THUMB)
            {
                ssCardScroll.dragging = true;
                ssCardScroll.dragGrabY = uiInput.mouse.y - rcScrollbarThumb.y;
                ssCardScroll.target = ssCardScroll.pos;
                ssCardScroll.vel = 0.0f;
                scrollTouched = true;
            }

            // Click on the track = page up/down
            if (uiInput.pressed && uiInput.hotId == UI_ID_SCROLL_TRACK)
            {
                float page = lytLayoutBase.rcContentClip.height * 0.85f;
                if (uiInput.mouse.y < rcScrollbarThumb.y) ssCardScroll.target = ClampF(ssCardScroll.target - page, 0.0f, lytLayoutBase.fScrollMax);
                else if (uiInput.mouse.y > (rcScrollbarThumb.y + rcScrollbarThumb.height)) ssCardScroll.target = ClampF(ssCardScroll.target + page, 0.0f, lytLayoutBase.fScrollMax);
                ssCardScroll.vel = 0.0f;
                scrollTouched = true;
            }

            // Drag updates scroll directly (no latency)
            if (ssCardScroll.dragging)
            {
                if (uiInput.down && uiInput.activeId == UI_ID_SCROLL_THUMB)
                {
                    float trackY = rcScrollbarTrack.y;
                    float trackH = rcScrollbarTrack.height;
                    float thumbH = rcScrollbarThumb.height;
                    float newY = uiInput.mouse.y - ssCardScroll.dragGrabY;
                    newY = ClampF(newY, trackY, trackY + trackH - thumbH);
                    float t = (trackH > thumbH + 0.001f) ? ((newY - trackY) / (trackH - thumbH)) : 0.0f;
                    ssCardScroll.pos = ClampF(t * lytLayoutBase.fScrollMax, 0.0f, lytLayoutBase.fScrollMax);
                    ssCardScroll.vel = 0.0f;
                    scrollTouched = true;
                }

                if (!uiInput.down || uiInput.released || (uiInput.activeId != UI_ID_SCROLL_THUMB))
                {
                    ssCardScroll.dragging = false;
                }
            }

            // Scroll motion (WinUI-like): wheel changes the target, then we smooth toward it.
            if (!ssCardScroll.dragging)
            {
                if (fWheelDelta != 0.0f)
                {
                    // Step size tuned to feel like Fluent ScrollViewer (slightly larger on taller viewports).
                    float wheelStep = LerpF(78.0f, 104.0f, Clamp01(lytLayoutBase.rcContentClip.height / 420.0f));
                    if (RLIsKeyDown(KEY_LEFT_CONTROL) || RLIsKeyDown(KEY_RIGHT_CONTROL)) wheelStep *= 1.55f;

                    ssCardScroll.target = ClampF(ssCardScroll.target + (-fWheelDelta) * wheelStep, 0.0f, lytLayoutBase.fScrollMax);
                    scrollTouched = true;
                }

                // Smooth to target (critically damped).
                float prevPos = ssCardScroll.pos;
                float smoothTime = (fWheelDelta != 0.0f) ? 0.085f : 0.125f;
                ssCardScroll.pos = SmoothDamp(ssCardScroll.pos, ssCardScroll.target, &ssCardScroll.vel, smoothTime, 12000.0f, fDeltaTime);

                // Hard clamp (no overscroll). Keep target consistent too.
                if (ssCardScroll.pos < 0.0f)
                {
                    ssCardScroll.pos = 0.0f;
                    ssCardScroll.target = 0.0f;
                    ssCardScroll.vel = 0.0f;
                }
                else if (ssCardScroll.pos > lytLayoutBase.fScrollMax)
                {
                    ssCardScroll.pos = lytLayoutBase.fScrollMax;
                    ssCardScroll.target = lytLayoutBase.fScrollMax;
                    ssCardScroll.vel = 0.0f;
                }

                if (fabsf(ssCardScroll.pos - prevPos) > 0.01f) scrollTouched = true;
                if (fabsf(ssCardScroll.vel) < 0.05f && fabsf(ssCardScroll.pos - ssCardScroll.target) < 0.05f) ssCardScroll.vel = 0.0f;
            }

            // Scrollbar auto-hide
            {
                bool sbHover = (uiInput.hotId == UI_ID_SCROLL_THUMB) || (uiInput.hotId == UI_ID_SCROLL_TRACK) || RLCheckCollisionPointRec(v2MousePos, rcScrollbarHotZone);
                bool sbDrag = ssCardScroll.dragging || ((uiInput.activeId == UI_ID_SCROLL_THUMB) && uiInput.down);
                if (scrollTouched || sbHover || sbDrag) fScrollbarWakeSeconds = 0.9f;
                else fScrollbarWakeSeconds = fmaxf(0.0f, fScrollbarWakeSeconds - fDeltaTime);

                bool sbWant = sbDrag || sbHover || RLCheckCollisionPointRec(v2MousePos, lytLayoutBase.rcContentClip) || (fabsf(ssCardScroll.vel) > 2.0f) || (fabsf(ssCardScroll.target - ssCardScroll.pos) > 0.35f);
                float sbTarget = (sbWant || fScrollbarWakeSeconds > 0.0f) ? 1.0f : 0.0f;
                fScrollbarVisibility = ApproachExp(fScrollbarVisibility, sbTarget, 10.0f, fDeltaTime);
            }

            // If scroll changed after input (track click / drag / inertia), update layout for drawing this frame.
            fScrollPositionDraw = ssCardScroll.pos;
            lytLayout = lytLayoutBase;
            ApplyScrollToLayout(&lytLayout, fScrollPositionDraw);
            RecomputeFlyoutLayout(&lytLayout, iScreenWidth, iScreenHeight);
            ComputeScrollbarRects(&lytLayoutBase, fScrollPositionDraw, &rcScrollbarTrack, &rcScrollbarThumb);
        }
        else
        {
            fScrollbarWakeSeconds = 0.0f;
            fScrollbarVisibility = ApproachExp(fScrollbarVisibility, 0.0f, 12.0f, fDeltaTime);
        }

        // Clear focus on background click (not when flyout is open; scrim handles dismissal).
        if (!bFlyoutVisible && uiInput.pressed && uiInput.hotId < 0) iFocusId = -1;
        if (!RLIsWindowFocused()) iFocusId = -1;

        // Light-dismiss by scrim
        if (bFlyoutVisible && uiInput.pressed && uiInput.hotId == UI_ID_SCRIM)
        {
            bFlyoutWanted = false;
        }

        // Keyboard focus cycling within the current scope.
        if (RLIsKeyPressed(KEY_TAB))
        {
            int dir = (RLIsKeyDown(KEY_LEFT_SHIFT) || RLIsKeyDown(KEY_RIGHT_SHIFT)) ? -1 : 1;
            if (bFlyoutVisible)
            {
                const int orderFly[] = { 100, 101 };
                FocusCycle(orderFly, 2, dir, &iFocusId);
            }
            else
            {
                const int orderMain[] = { 0, 1, 2, 4 };
                FocusCycle(orderMain, 4, dir, &iFocusId);
            }
        }

        // Update flyout motion (after processing events for this frame)
        {
            float target = bFlyoutWanted ? 1.0f : 0.0f;
            float smooth = bFlyoutWanted ? 0.18f : 0.12f;
            fFlyoutAnimT = SmoothDampF(fFlyoutAnimT, target, &fFlyoutAnimVel, smooth, 6.0f, fDeltaTime);
            fFlyoutAnimT = Clamp01(fFlyoutAnimT);
        }

        float flyAlpha = Smoothstep(fFlyoutAnimT);
        float flyLift = (1.0f - flyAlpha) * 10.0f;
        int flyDirDraw = (lytLayout.iFlyoutDirection == 0) ? +1 : lytLayout.iFlyoutDirection;
        float flyOffset = -flyLift * (float)flyDirDraw;

        RLRectangle flyPanelDraw = lytLayout.rcFlyoutPanel;
        flyPanelDraw.y += flyOffset;

        RLRectangle flyBtn1Draw = lytLayout.rcFlyoutButton1;
        RLRectangle flyBtn2Draw = lytLayout.rcFlyoutButton2;
        flyBtn1Draw.y += flyOffset;
        flyBtn2Draw.y += flyOffset;

        RLBeginDrawing();

        // Safety: ensure no scissor leaks between frames.
        ScissorReset();

        // Acrylic blur should track the same blur level control.
        int acrylicDownSample = BlurDownSampleFromLevel(iBackgroundBlurLevel);
        float acrylicSoften = Clamp01(((float)acrylicDownSample - 2.0f) / 14.0f);

        // Update acxAcrylic RenderTexture (down-sampled backdrop used for acxAcrylic fills)
        UpdateAcrylicBackdropRT(&rtAcrylicBackdrop, &iAcrylicWidth, &iAcrylicHeight, acrylicDownSample, &ttTheme, fThemeLightness,
            bBackgroundUseImage, bBackgroundLoaded, texBackground);

        // Window background
        DrawWindowBackdrop(&ttTheme, fThemeLightness, bBackgroundUseImage, bBackgroundLoaded, texBackground, bBackgroundUseBlur, iBackgroundBlurLevel, &rtBackgroundBlur, &iBackgroundBlurWidth, &iBackgroundBlurHeight);

        // Header
        RLDrawText("Fluent buttons (raylib)", 28, 22, 22, ttTheme.text);
        {
            char line[512];
            TextEllipsizeToFit(line, sizeof(line), u8"Tab focus · Enter/Space activate · T theme · B image · V blur · [ ] blur · A acxAcrylic · N noise · M btn-acxAcrylic · C flyout-bounds",
                16, (float)iScreenWidth - 56.0f);
            RLDrawText(line, 28, 52, 16, ttTheme.textDim);
        }
        RLDrawText(RLTextFormat("Clicks: %i", iClicks), 28, 74, 16, ttTheme.textDim);

        // Status row
        {
            const char* onOff = bBackgroundUseImage ? "ON" : "OFF";
            const char* blurOnOff = bBackgroundUseBlur ? "ON" : "OFF";
            const char* acOnOff = gAcrylicEnabled ? "ON" : "OFF";
            const char* nOnOff = gAcrylicNoise ? "ON" : "OFF";
            const char* btnAc = gAcrylicButtons ? "ON" : "OFF";
            const char* flyBound = gFlyoutConstrainToCard ? "CARD" : "ROOT";
            {
                const char* s = RLTextFormat("BG Image: %s  Blur: %s  Level: %d", onOff, blurOnOff, iBackgroundBlurLevel);
                char line[256];
                TextEllipsizeToFit(line, sizeof(line), s, 14, (float)iScreenWidth - 56.0f);
                RLDrawText(line, 28, 94, 14, ttTheme.textDim);
            }
            {
                const char* s = RLTextFormat("Acrylic: %s  Grain: %s  BtnAcrylic: %s  Flyout: %s", acOnOff, nOnOff, btnAc, flyBound);
                char line[256];
                TextEllipsizeToFit(line, sizeof(line), s, 14, (float)iScreenWidth - 56.0f);
                RLDrawText(line, 28, 112, 14, ttTheme.textDim);
            }
            if (bBackgroundUseImage && !bBackgroundLoaded)
            {
                RLDrawText("(No image found: put a file in examples/resources/fluent_bg.(png|jpg))", 28, 130, 14, ttTheme.textDim);
            }
        }

        // Card
        float cardShadow = LerpF(0.55f, 0.35f, fThemeLightness);
        DrawSoftShadowRounded(lytLayout.rcCard, 16.0f, cardShadow, &ttTheme);

        if (gAcrylicEnabled)
        {
            RLColor tintRgb = ttTheme.surface2;
            float tintAmount = LerpF(0.54f, 0.66f, Clamp01(fThemeLightness));
            float grain = gAcrylicNoise ? LerpF(0.014f, 0.022f, Clamp01(fThemeLightness)) : 0.0f;
            DrawAcrylicRounded(&acxAcrylic, rtAcrylicBackdrop.texture, lytLayout.rcCard, 16.0f, tintRgb, tintAmount, grain, fTimeSeconds, acrylicSoften);
        }
        else
        {
            RLDrawRectangleRounded(lytLayout.rcCard, RoundnessForRadius(lytLayout.rcCard.width, lytLayout.rcCard.height, 16.0f), 12, ttTheme.surface2);
        }

        RLDrawRectangleRoundedLines(lytLayout.rcCard, RoundnessForRadius(lytLayout.rcCard.width, lytLayout.rcCard.height, 16.0f), 12, ttTheme.border);
        // Clip scrollable card content (prevents overflow in stacked layout)
        {
            int scX = (int)(lytLayout.rcContentClip.x + 1);
            int scY = (int)(lytLayout.rcContentClip.y + 1);
            int scW = (int)(lytLayout.rcContentClip.width - 2);
            int scH = (int)(lytLayout.rcContentClip.height - 2);
            if (scW < 1) scW = 1;
            if (scH < 1) scH = 1;
            PushScissorI(scX, scY, scW, scH);
        }

        // Card content headings
        RLDrawText("Variants", (int)lytLayout.v2VariantsTitlePos.x, (int)lytLayout.v2VariantsTitlePos.y, 18, ttTheme.text);

        // Labels (aligned to rows)
        if (!lytLayout.bLabelsAbove)
        {
            RLDrawText("Default", (int)lytLayout.fLabelX, (int)(lytLayout.rcButtonOpen.y + lytLayout.fLabelYOffset), 14, ttTheme.textDim);
            RLDrawText("Primary", (int)lytLayout.fLabelX, (int)(lytLayout.rcButtonPrimary.y + lytLayout.fLabelYOffset), 14, ttTheme.textDim);
            RLDrawText("Subtle", (int)lytLayout.fLabelX, (int)(lytLayout.rcButtonSubtle.y + lytLayout.fLabelYOffset), 14, ttTheme.textDim);
            RLDrawText("Disabled", (int)lytLayout.fLabelX, (int)(lytLayout.rcButtonDisabled.y + lytLayout.fLabelYOffset), 14, ttTheme.textDim);
        }
        else
        {
            RLDrawText("Default", (int)lytLayout.rcButtonOpen.x, (int)(lytLayout.rcButtonOpen.y - 22), 14, ttTheme.textDim);
            RLDrawText("Primary", (int)lytLayout.rcButtonPrimary.x, (int)(lytLayout.rcButtonPrimary.y - 22), 14, ttTheme.textDim);
            RLDrawText("Subtle", (int)lytLayout.rcButtonSubtle.x, (int)(lytLayout.rcButtonSubtle.y - 22), 14, ttTheme.textDim);
            RLDrawText("Disabled", (int)lytLayout.rcButtonDisabled.x, (int)(lytLayout.rcButtonDisabled.y - 22), 14, ttTheme.textDim);
        }

        // Theme section label
        {
            bool nowLight = (fThemeMix >= 0.5f);
            if (lytLayout.bNarrowLayout)
            {
                float sepY = lytLayout.v2ThemeTitlePos.y - 14.0f;
                if (sepY > lytLayout.rcContentClip.y + 8.0f)
                {
                    // Subtle separator: derive from existing border token (no extra theme field).
                    RLColor sepCol = ttTheme.border;
                    sepCol.a = (unsigned char)((float)sepCol.a * 0.65f);
                    RLDrawLine((int)(lytLayout.rcContentClip.x + 10.0f), (int)sepY,
                        (int)(lytLayout.rcContentClip.x + lytLayout.rcContentClip.width - 10.0f), (int)sepY, sepCol);
                }
            }
            RLDrawText("Theme", (int)lytLayout.v2ThemeTitlePos.x, (int)lytLayout.v2ThemeTitlePos.y, 18, ttTheme.text);
            RLDrawText(nowLight ? "Light" : "Dark", (int)(lytLayout.v2ThemeStatePos.x + 2.0f), (int)lytLayout.v2ThemeStatePos.y, 14, ttTheme.textDim);
        }

        // Buttons (use routed UI input)
        if (DrawFluentButtonEx(0, lytLayout.rcButtonOpen, "Open Menu", FLUENT_BTN_DEFAULT, true, &fbsDefault, &iFocusId, &ttTheme, fThemeLightness,
            &acxAcrylic, rtAcrylicBackdrop.texture, fTimeSeconds, acrylicSoften, &uiInput))
        {
            iClicks++;
            bFlyoutWanted = !bFlyoutWanted;
            if (bFlyoutWanted)
            {
                iFocusBeforeFlyout = iFocusId;
                iFocusId = 100;
            }
        }

        if (DrawFluentButtonEx(1, lytLayout.rcButtonPrimary, "Primary Action", FLUENT_BTN_PRIMARY, true, &fbsPrimary, &iFocusId, &ttTheme, fThemeLightness,
            &acxAcrylic, rtAcrylicBackdrop.texture, fTimeSeconds, acrylicSoften, &uiInput))
        {
            iClicks++;
        }

        if (DrawFluentButtonEx(2, lytLayout.rcButtonSubtle, "More Options", FLUENT_BTN_SUBTLE, true, &fbsSubtle, &iFocusId, &ttTheme, fThemeLightness,
            &acxAcrylic, rtAcrylicBackdrop.texture, fTimeSeconds, acrylicSoften, &uiInput))
        {
            iClicks++;
        }

        (void)DrawFluentButtonEx(3, lytLayout.rcButtonDisabled, "Disabled", FLUENT_BTN_DEFAULT, false, &fbsDisabled, &iFocusId, &ttTheme, fThemeLightness,
            &acxAcrylic, rtAcrylicBackdrop.texture, fTimeSeconds, acrylicSoften, &uiInput);

        // Theme toggle
        {
            bool nowLight = (fThemeMix >= 0.5f);
            if (DrawFluentButtonEx(4, lytLayout.rcButtonTheme, nowLight ? "Switch to Dark" : "Switch to Light",
                FLUENT_BTN_DEFAULT, true, &fbsThemeToggle, &iFocusId, &ttTheme, fThemeLightness,
                &acxAcrylic, rtAcrylicBackdrop.texture, fTimeSeconds, acrylicSoften, &uiInput))
            {
                bThemeTargetLight = !bThemeTargetLight;
            }
        }


        PopScissor();

        // Edge fades help hide hard rcClip edges in a ScrollViewer-like way.
        if (lytLayoutBase.fScrollMax > 0.01f)
        {
            float fadeH = 22.0f;
            float fadeW = lytLayout.rcContentClip.width;

            // Only show fades when there is actually clipped content.
            if (ssCardScroll.pos > 0.10f)
            {
                RLColor fc = ttTheme.surface2; fc.a = (unsigned char)(gAcrylicEnabled ? 210 : 240);
                RLColor fc0 = fc; fc0.a = 0;
                RLDrawRectangleGradientV((int)lytLayout.rcContentClip.x, (int)lytLayout.rcContentClip.y, (int)fadeW, (int)fadeH, fc, fc0);
            }
            if (ssCardScroll.pos < (lytLayoutBase.fScrollMax - 0.10f))
            {
                RLColor fc = ttTheme.surface2; fc.a = (unsigned char)(gAcrylicEnabled ? 210 : 240);
                RLColor fc0 = fc; fc0.a = 0;
                RLDrawRectangleGradientV((int)lytLayout.rcContentClip.x, (int)(lytLayout.rcContentClip.y + lytLayout.rcContentClip.height - fadeH), (int)fadeW, (int)fadeH, fc0, fc);
            }
        }

        // Scrollbar (auto-hide and hover widen)
        if (!bFlyoutVisible && lytLayoutBase.fScrollMax > 0.01f && fScrollbarVisibility > 0.02f)
        {
            float vis = Smoothstep(fScrollbarVisibility);
            bool sbHover = (uiInput.hotId == UI_ID_SCROLL_THUMB) || (uiInput.hotId == UI_ID_SCROLL_TRACK) || RLCheckCollisionPointRec(uiInput.mouse, rcScrollbarHotZone);
            bool sbDrag = ssCardScroll.dragging || ((uiInput.activeId == UI_ID_SCROLL_THUMB) && uiInput.down);

            float wTarget = sbDrag ? 10.5f : (sbHover ? 8.5f : 6.8f);
            fScrollbarThumbWidth = ApproachExp(fScrollbarThumbWidth, wTarget, 22.0f, fDeltaTime);

            RLColor track = ttTheme.border;
            track.a = (unsigned char)((sbHover ? 90.0f : 65.0f) * vis);
            float trackW = LerpF(2.0f, 4.0f, vis);
            RLDrawRectangleRounded((RLRectangle) { rcScrollbarTrack.x + 3.0f, rcScrollbarTrack.y, trackW, rcScrollbarTrack.height }, 0.9f, 8, track);

            float thumbW = fScrollbarThumbWidth;
            float thumbX = rcScrollbarTrack.x + (rcScrollbarTrack.width - thumbW) * 0.5f;
            RLColor thumb = ttTheme.textDim;
            thumb.a = (unsigned char)((sbDrag ? 235.0f : (sbHover ? 185.0f : 135.0f)) * vis);
            RLDrawRectangleRounded((RLRectangle) { thumbX, rcScrollbarThumb.y, thumbW, rcScrollbarThumb.height }, 0.9f, 8, thumb);
        }
        else
        {
            fScrollbarThumbWidth = ApproachExp(fScrollbarThumbWidth, 6.8f, 14.0f, fDeltaTime);
        }

        // Footer (fixed; does not scroll).
        {
            RLRectangle foot = lytLayout.rcFooter;
            if (foot.width < 80.0f) foot.width = 80.0f;
            if (foot.height < 24.0f) foot.height = 24.0f;

            // Clip to footer rect
            PushScissorRect(foot, 0);

            char l1[256], l2[256];
            int lines = WrapText2Lines(l1, l2,
                "Tip: real Acrylic/Mica is OS-level composition; here we approximate with down-sampled sampling + tint + grain.",
                14, foot.width);

            float yText = foot.y + (foot.height - (lines > 1 ? 36.0f : 16.0f)) * 0.5f;
            RLDrawText(l1, (int)(foot.x + 0.5f), (int)(yText + 0.5f), 14, ttTheme.textDim);
            if (lines > 1) RLDrawText(l2, (int)(foot.x + 0.5f), (int)(yText + 20.0f + 0.5f), 14, ttTheme.textDim);

            PopScissor();
        }

        // Flyout overlay (virtual z-index: scrim < flyout)
        if (flyAlpha > 0.001f)
        {
            // Scrim
            RLColor scrim = (RLColor){ 0, 0, 0, (unsigned char)(LerpF(10.0f, 26.0f, 1.0f - fThemeLightness) * flyAlpha) };
            RLDrawRectangle(0, 0, iScreenWidth, iScreenHeight, scrim);

            // Flyout shadow + fill
            DrawSoftShadowRounded(flyPanelDraw, 14.0f, 0.72f * flyAlpha, &ttTheme);

            if (gAcrylicEnabled)
            {
                RLColor tintRgb = ttTheme.surface2;
                float tintAmount = LerpF(0.56f, 0.68f, Clamp01(fThemeLightness));
                float grain = gAcrylicNoise ? LerpF(0.014f, 0.022f, Clamp01(fThemeLightness)) : 0.0f;
                DrawAcrylicRounded(&acxAcrylic, rtAcrylicBackdrop.texture, flyPanelDraw, 14.0f, tintRgb, tintAmount, grain, fTimeSeconds, acrylicSoften);
            }
            else
            {
                RLDrawRectangleRounded(flyPanelDraw, RoundnessForRadius(flyPanelDraw.width, flyPanelDraw.height, 14.0f), 12, ttTheme.surface2);
            }

            RLDrawRectangleRoundedLines(flyPanelDraw, RoundnessForRadius(flyPanelDraw.width, flyPanelDraw.height, 14.0f), 12, ttTheme.border);

            // Content
            RLColor tText = ttTheme.text;
            RLColor tDim = ttTheme.textDim;
            tText.a = (unsigned char)(255.0f * flyAlpha);
            tDim.a = (unsigned char)(255.0f * flyAlpha);

            RLDrawText("Menu", (int)flyPanelDraw.x + 16, (int)flyPanelDraw.y + 14, 18, tText);
            RLDrawText("This is a simple flyout.", (int)flyPanelDraw.x + 16, (int)flyPanelDraw.y + 40, 14, tDim);

            if (DrawFluentButtonEx(100, flyBtn1Draw, "Action 1", FLUENT_BTN_DEFAULT, true, &fbsFlyoutAction1, &iFocusId, &ttTheme, fThemeLightness,
                &acxAcrylic, rtAcrylicBackdrop.texture, fTimeSeconds, acrylicSoften, &uiInput))
            {
                iClicks++;
                bFlyoutWanted = false;
            }
            if (DrawFluentButtonEx(101, flyBtn2Draw, "Close", FLUENT_BTN_SUBTLE, true, &fbsFlyoutAction2, &iFocusId, &ttTheme, fThemeLightness,
                &acxAcrylic, rtAcrylicBackdrop.texture, fTimeSeconds, acrylicSoften, &uiInput))
            {
                bFlyoutWanted = false;
            }
        }

        // Restore focus after flyout finishes closing
        if (!bFlyoutWanted && fFlyoutAnimT < 0.01f && iFocusBeforeFlyout >= 0)
        {
            iFocusId = iFocusBeforeFlyout;
            iFocusBeforeFlyout = -1;
        }

        RLEndDrawing();
    }

    // Cleanup
    if (rtAcrylicBackdrop.id != 0) RLUnloadRenderTexture(rtAcrylicBackdrop);
    if (rtBackgroundBlur.id != 0) RLUnloadRenderTexture(rtBackgroundBlur);
    if (texBackground.id != 0) RLUnloadTexture(texBackground);
    AcrylicUnload(&acxAcrylic);

    RLCloseWindow();
    return 0;
}
