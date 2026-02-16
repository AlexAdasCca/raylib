/**********************************************************************************************
*
*   raylib v5.6-dev - A simple and easy-to-use library to enjoy videogames programming (www.raylib.com)
*
*   FEATURES:
*       - NO external dependencies, all required libraries included with raylib
*       - Multiplatform: Windows, Linux, macOS, FreeBSD, Web, Android, Raspberry Pi, DRM native...
*       - Written in plain C code (C99) in PascalCase/camelCase notation
*       - Hardware accelerated with OpenGL (1.1, 2.1, 3.3, 4.3, ES2, ES3 - choose at compile)
*       - Custom OpenGL abstraction layer (usable as standalone module): [rlgl]
*       - Multiple Fonts formats supported (TTF, OTF, FNT, BDF, Sprite fonts)
*       - Many texture formats supportted, including compressed formats (DXT, ETC, ASTC)
*       - Full 3d support for 3d Shapes, Models, Billboards, Heightmaps and more!
*       - Flexible Materials system, supporting classic maps and PBR maps
*       - Animated 3D models supported (skeletal bones animation) (IQM, M3D, GLTF)
*       - Shaders support, including Model shaders and Postprocessing shaders
*       - Powerful math module for Vector, Matrix and Quaternion operations: [raymath]
*       - Audio loading and playing with streaming support (WAV, OGG, MP3, FLAC, QOA, XM, MOD)
*       - VR stereo rendering with configurable HMD device parameters
*       - Bindings to multiple programming languages available!
*
*   NOTES:
*       - One default Font is loaded on InitWindow()->LoadFontDefault() [core, text]
*       - One default Texture2D is loaded on rlglInit(), 1x1 white pixel R8G8B8A8 [rlgl] (OpenGL 3.3 or ES2)
*       - One default Shader is loaded on rlglInit()->rlLoadShaderDefault() [rlgl] (OpenGL 3.3 or ES2)
*       - One default RenderBatch is loaded on rlglInit()->rlLoadRenderBatch() [rlgl] (OpenGL 3.3 or ES2)
*
*   DEPENDENCIES:
*       [rcore] Depends on the selected platform backend, check rcore.c header for details 
*       [rlgl] glad/glad_gles2 (David Herberth - github.com/Dav1dde/glad) for OpenGL extensions loading
*       [raudio] miniaudio (David Reid - github.com/mackron/miniaudio) for audio device/context management
*
*   OPTIONAL DEPENDENCIES (included):
*       [rcore] sinfl (Micha Mettke) for DEFLATE decompression algorithm
*       [rcore] sdefl (Micha Mettke) for DEFLATE compression algorithm
*       [rcore] rprand (Ramon Santamaria) for pseudo-random numbers generation
*       [rtextures] qoi (Dominic Szablewski - https://phoboslab.org) for QOI image manage
*       [rtextures] stb_image (Sean Barret) for images loading (BMP, TGA, PNG, JPEG, HDR...)
*       [rtextures] stb_image_write (Sean Barret) for image writing (BMP, TGA, PNG, JPG)
*       [rtextures] stb_image_resize2 (Sean Barret) for image resizing algorithms
*       [rtextures] stb_perlin (Sean Barret) for Perlin Noise image generation
*       [rtextures] rl_gputex (Ramon Santamaria) for GPU-compressed texture formats 
*       [rtext] stb_truetype (Sean Barret) for ttf fonts loading
*       [rtext] stb_rect_pack (Sean Barret) for rectangles packing
*       [rmodels] par_shapes (Philip Rideout) for parametric 3d shapes generation
*       [rmodels] tinyobj_loader_c (Syoyo Fujita) for models loading (OBJ, MTL)
*       [rmodels] cgltf (Johannes Kuhlmann) for models loading (glTF)
*       [rmodels] m3d (bzt) for models loading (M3D, https://bztsrc.gitlab.io/model3d)
*       [rmodels] vox_loader (Johann Nadalutti) for models loading (VOX)
*       [raudio] dr_wav (David Reid) for WAV audio file loading
*       [raudio] dr_flac (David Reid) for FLAC audio file loading
*       [raudio] dr_mp3 (David Reid) for MP3 audio file loading
*       [raudio] stb_vorbis (Sean Barret) for OGG audio loading
*       [raudio] jar_xm (Joshua Reisenauer) for XM audio module loading
*       [raudio] jar_mod (Joshua Reisenauer) for MOD audio module loading
*       [raudio] qoa (Dominic Szablewski - https://phoboslab.org) for QOA audio manage
*
*
*   LICENSE: zlib/libpng
*
*   raylib is licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software:
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

#ifndef RAYLIB_H
#define RAYLIB_H

#include <stdarg.h>
#include <stdint.h>     // Required for: va_list - Only used by TraceLogCallback

#define RAYLIB_VERSION_MAJOR 5
#define RAYLIB_VERSION_MINOR 6
#define RAYLIB_VERSION_PATCH 0
#define RAYLIB_VERSION  "5.6-dev"

// Function specifiers in case library is build/used as a shared library
// NOTE: Microsoft specifiers to tell compiler that symbols are imported/exported from a .dll
// NOTE: visibility("default") attribute makes symbols "visible" when compiled with -fvisibility=hidden
#if defined(_WIN32)
    #if defined(__TINYC__)
        #define __declspec(x) __attribute__((x))
    #endif
    #if defined(BUILD_LIBTYPE_SHARED)
        #define RLAPI __declspec(dllexport)     // Building the library as a Win32 shared library (.dll)
    #elif defined(USE_LIBTYPE_SHARED)
        #define RLAPI __declspec(dllimport)     // Using the library as a Win32 shared library (.dll)
    #endif
#else
    #if defined(BUILD_LIBTYPE_SHARED)
        #define RLAPI __attribute__((visibility("default"))) // Building as a Unix shared library (.so/.dylib)
    #endif
#endif

#ifndef RLAPI
    #define RLAPI       // Functions defined as 'extern' by default (implicit specifiers)
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#ifndef PI
    #define PI 3.14159265358979323846f
#endif
#ifndef DEG2RAD
    #define DEG2RAD (PI/180.0f)
#endif
#ifndef RAD2DEG
    #define RAD2DEG (180.0f/PI)
#endif

// Allow custom memory allocators
// NOTE: Require recompiling raylib sources
#ifndef RL_MALLOC
    #define RL_MALLOC(sz)       malloc(sz)
#endif
#ifndef RL_CALLOC
    #define RL_CALLOC(n,sz)     calloc(n,sz)
#endif
#ifndef RL_REALLOC
    #define RL_REALLOC(ptr,sz)  realloc(ptr,sz)
#endif
#ifndef RL_FREE
    #define RL_FREE(ptr)        free(ptr)
#endif

// NOTE: MSVC C++ compiler does not support compound literals (C99 feature)
// Plain structures in C++ (without constructors) can be initialized with { }
// This is called aggregate initialization (C++11 feature)
#if defined(__cplusplus)
    #define CLITERAL(type)      type
#else
    #define CLITERAL(type)      (type)
#endif

// Some compilers (mostly macos clang) default to C++98,
// where aggregate initialization can't be used
// So, give a more clear error stating how to fix this
#if !defined(_MSC_VER) && (defined(__cplusplus) && __cplusplus < 201103L)
    #error "C++11 or later is required. Add -std=c++11"
#endif

// NOTE: Set some defines with some data types declared by raylib
// Other modules (raymath, rlgl) also require some of those types, so,
// to be able to use those other modules as standalone (not depending on raylib)
// this defines are very useful for internal check and avoid type (re)definitions
#define RL_COLOR_TYPE
#define RL_RECTANGLE_TYPE
#define RL_VECTOR2_TYPE
#define RL_VECTOR3_TYPE
#define RL_VECTOR4_TYPE
#define RL_QUATERNION_TYPE
#define RL_MATRIX_TYPE

// Some Basic Colors
// NOTE: Custom raylib color palette for amazing visuals on WHITE background
#define LIGHTGRAY  CLITERAL(RLColor){ 200, 200, 200, 255 }   // Light Gray
#define GRAY       CLITERAL(RLColor){ 130, 130, 130, 255 }   // Gray
#define DARKGRAY   CLITERAL(RLColor){ 80, 80, 80, 255 }      // Dark Gray
#define YELLOW     CLITERAL(RLColor){ 253, 249, 0, 255 }     // Yellow
#define GOLD       CLITERAL(RLColor){ 255, 203, 0, 255 }     // Gold
#define ORANGE     CLITERAL(RLColor){ 255, 161, 0, 255 }     // Orange
#define PINK       CLITERAL(RLColor){ 255, 109, 194, 255 }   // Pink
#define RED        CLITERAL(RLColor){ 230, 41, 55, 255 }     // Red
#define MAROON     CLITERAL(RLColor){ 190, 33, 55, 255 }     // Maroon
#define GREEN      CLITERAL(RLColor){ 0, 228, 48, 255 }      // Green
#define LIME       CLITERAL(RLColor){ 0, 158, 47, 255 }      // Lime
#define DARKGREEN  CLITERAL(RLColor){ 0, 117, 44, 255 }      // Dark Green
#define SKYBLUE    CLITERAL(RLColor){ 102, 191, 255, 255 }   // Sky Blue
#define BLUE       CLITERAL(RLColor){ 0, 121, 241, 255 }     // Blue
#define DARKBLUE   CLITERAL(RLColor){ 0, 82, 172, 255 }      // Dark Blue
#define PURPLE     CLITERAL(RLColor){ 200, 122, 255, 255 }   // Purple
#define VIOLET     CLITERAL(RLColor){ 135, 60, 190, 255 }    // Violet
#define DARKPURPLE CLITERAL(RLColor){ 112, 31, 126, 255 }    // Dark Purple
#define BEIGE      CLITERAL(RLColor){ 211, 176, 131, 255 }   // Beige
#define BROWN      CLITERAL(RLColor){ 127, 106, 79, 255 }    // Brown
#define DARKBROWN  CLITERAL(RLColor){ 76, 63, 47, 255 }      // Dark Brown

#define WHITE      CLITERAL(RLColor){ 255, 255, 255, 255 }   // White
#define BLACK      CLITERAL(RLColor){ 0, 0, 0, 255 }         // Black
#define BLANK      CLITERAL(RLColor){ 0, 0, 0, 0 }           // Blank (Transparent)
#define MAGENTA    CLITERAL(RLColor){ 255, 0, 255, 255 }     // Magenta
#define RAYWHITE   CLITERAL(RLColor){ 245, 245, 245, 255 }   // My own White (raylib logo)

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Boolean type
#if (defined(__STDC__) && __STDC_VERSION__ >= 199901L) || (defined(_MSC_VER) && _MSC_VER >= 1800)
    #include <stdbool.h>
#elif !defined(__cplusplus) && !defined(bool)
    typedef enum bool { false = 0, true = !false } bool;
    #define RL_BOOL_TYPE
#endif

// Vector2, 2 components
typedef struct RLVector2 {
    float x;                // Vector x component
    float y;                // Vector y component
} RLVector2;

// Vector3, 3 components
typedef struct RLVector3 {
    float x;                // Vector x component
    float y;                // Vector y component
    float z;                // Vector z component
} RLVector3;

// Vector4, 4 components
typedef struct RLVector4 {
    float x;                // Vector x component
    float y;                // Vector y component
    float z;                // Vector z component
    float w;                // Vector w component
} RLVector4;

// Quaternion, 4 components (Vector4 alias)
typedef RLVector4 RLQuaternion;

// Matrix, 4x4 components, column major, OpenGL style, right-handed
typedef struct RLMatrix {
    float m0, m4, m8, m12;  // Matrix first row (4 components)
    float m1, m5, m9, m13;  // Matrix second row (4 components)
    float m2, m6, m10, m14; // Matrix third row (4 components)
    float m3, m7, m11, m15; // Matrix fourth row (4 components)
} RLMatrix;

// Color, 4 components, R8G8B8A8 (32bit)
typedef struct RLColor {
    unsigned char r;        // Color red value
    unsigned char g;        // Color green value
    unsigned char b;        // Color blue value
    unsigned char a;        // Color alpha value
} RLColor;

// Rectangle, 4 components
typedef struct RLRectangle {
    float x;                // Rectangle top-left corner position x
    float y;                // Rectangle top-left corner position y
    float width;            // Rectangle width
    float height;           // Rectangle height
} RLRectangle;

// Image, pixel data stored in CPU memory (RAM)
typedef struct RLImage {
    void *data;             // Image raw data
    int width;              // Image base width
    int height;             // Image base height
    int mipmaps;            // Mipmap levels, 1 by default
    int format;             // Data format (PixelFormat type)
} RLImage;

// Texture, tex data stored in GPU memory (VRAM)
typedef struct RLTexture {
    unsigned int id;        // OpenGL texture id
    int width;              // Texture base width
    int height;             // Texture base height
    int mipmaps;            // Mipmap levels, 1 by default
    int format;             // Data format (PixelFormat type)
} RLTexture;

// Texture2D, same as Texture
typedef RLTexture RLTexture2D;

// TextureCubemap, same as Texture
typedef RLTexture RLTextureCubemap;

// RenderTexture, fbo for texture rendering
typedef struct RLRenderTexture {
    unsigned int id;        // OpenGL framebuffer object id
    RLTexture texture;        // Color buffer attachment texture
    RLTexture depth;          // Depth buffer attachment texture
} RLRenderTexture;

// RenderTexture2D, same as RenderTexture
typedef RLRenderTexture RLRenderTexture2D;

// NPatchInfo, n-patch layout info
typedef struct RLNPatchInfo {
    RLRectangle source;       // Texture source rectangle
    int left;               // Left border offset
    int top;                // Top border offset
    int right;              // Right border offset
    int bottom;             // Bottom border offset
    int layout;             // Layout of the n-patch: 3x3, 1x3 or 3x1
} RLNPatchInfo;

// GlyphInfo, font characters glyphs info
typedef struct RLGlyphInfo {
    int value;              // Character value (Unicode)
    int offsetX;            // Character offset X when drawing
    int offsetY;            // Character offset Y when drawing
    int advanceX;           // Character advance position X
    RLImage image;            // Character image data
} RLGlyphInfo;

// Font, font texture and GlyphInfo array data
typedef struct RLFont {
    int baseSize;           // Base size (default chars height)
    int glyphCount;         // Number of glyph characters
    int glyphPadding;       // Padding around the glyph characters
    RLTexture2D texture;      // Texture atlas containing the glyphs
    RLRectangle *recs;        // Rectangles in texture for the glyphs
    RLGlyphInfo *glyphs;      // Glyphs info data
} RLFont;

// Camera, defines position/orientation in 3d space
typedef struct RLCamera3D {
    RLVector3 position;       // Camera position
    RLVector3 target;         // Camera target it looks-at
    RLVector3 up;             // Camera up vector (rotation over its axis)
    float fovy;             // Camera field-of-view aperture in Y (degrees) in perspective, used as near plane height in world units in orthographic
    int projection;         // Camera projection: CAMERA_PERSPECTIVE or CAMERA_ORTHOGRAPHIC
} RLCamera3D;

typedef RLCamera3D RLCamera;    // Camera type fallback, defaults to Camera3D

// Camera2D, defines position/orientation in 2d space
typedef struct RLCamera2D {
    RLVector2 offset;         // Camera offset (screen space offset from window origin)
    RLVector2 target;         // Camera target (world space target point that is mapped to screen space offset)
    float rotation;         // Camera rotation in degrees (pivots around target)
    float zoom;             // Camera zoom (scaling around target), must not be set to 0, set to 1.0f for no scale
} RLCamera2D;

// Mesh, vertex data and vao/vbo
typedef struct RLMesh {
    int vertexCount;        // Number of vertices stored in arrays
    int triangleCount;      // Number of triangles stored (indexed or not)

    // Vertex attributes data
    float *vertices;        // Vertex position (XYZ - 3 components per vertex) (shader-location = 0)
    float *texcoords;       // Vertex texture coordinates (UV - 2 components per vertex) (shader-location = 1)
    float *texcoords2;      // Vertex texture second coordinates (UV - 2 components per vertex) (shader-location = 5)
    float *normals;         // Vertex normals (XYZ - 3 components per vertex) (shader-location = 2)
    float *tangents;        // Vertex tangents (XYZW - 4 components per vertex) (shader-location = 4)
    unsigned char *colors;      // Vertex colors (RGBA - 4 components per vertex) (shader-location = 3)
    unsigned short *indices;    // Vertex indices (in case vertex data comes indexed)

    // Animation vertex data
    float *animVertices;    // Animated vertex positions (after bones transformations)
    float *animNormals;     // Animated normals (after bones transformations)
    unsigned char *boneIds; // Vertex bone ids, max 255 bone ids, up to 4 bones influence by vertex (skinning) (shader-location = 6)
    float *boneWeights;     // Vertex bone weight, up to 4 bones influence by vertex (skinning) (shader-location = 7)
    RLMatrix *boneMatrices;   // Bones animated transformation matrices
    int boneCount;          // Number of bones

    // OpenGL identifiers
    unsigned int vaoId;     // OpenGL Vertex Array Object id
    unsigned int *vboId;    // OpenGL Vertex Buffer Objects id (default vertex data)
} RLMesh;

// Shader
typedef struct RLShader {
    unsigned int id;        // Shader program id
    int *locs;              // Shader locations array (RL_MAX_SHADER_LOCATIONS)
} RLShader;

// MaterialMap
typedef struct RLMaterialMap {
    RLTexture2D texture;      // Material map texture
    RLColor color;            // Material map color
    float value;            // Material map value
} RLMaterialMap;

// Material, includes shader and maps
typedef struct RLMaterial {
    RLShader shader;          // Material shader
    RLMaterialMap *maps;      // Material maps array (MAX_MATERIAL_MAPS)
    float params[4];        // Material generic parameters (if required)
} RLMaterial;

// Transform, vertex transformation data
typedef struct RLTransform {
    RLVector3 translation;    // Translation
    RLQuaternion rotation;    // Rotation
    RLVector3 scale;          // Scale
} RLTransform;

// Bone, skeletal animation bone
typedef struct RLBoneInfo {
    char name[32];          // Bone name
    int parent;             // Bone parent
} RLBoneInfo;

// Model, meshes, materials and animation data
typedef struct RLModel {
    RLMatrix transform;       // Local transform matrix

    int meshCount;          // Number of meshes
    int materialCount;      // Number of materials
    RLMesh *meshes;           // Meshes array
    RLMaterial *materials;    // Materials array
    int *meshMaterial;      // Mesh material number

    // Animation data
    int boneCount;          // Number of bones
    RLBoneInfo *bones;        // Bones information (skeleton)
    RLTransform *bindPose;    // Bones base transformation (pose)
} RLModel;

// ModelAnimation
typedef struct RLModelAnimation {
    char name[32];          // Animation name
    int boneCount;          // Number of bones
    int frameCount;         // Number of animation frames
    RLBoneInfo *bones;        // Bones information (skeleton)
    RLTransform **framePoses; // Poses array by frame
} RLModelAnimation;

// Ray, ray for raycasting
typedef struct RLRay {
    RLVector3 position;       // Ray position (origin)
    RLVector3 direction;      // Ray direction (normalized)
} RLRay;

// RayCollision, ray hit information
typedef struct RLRayCollision {
    bool hit;               // Did the ray hit something?
    float distance;         // Distance to the nearest hit
    RLVector3 point;          // Point of the nearest hit
    RLVector3 normal;         // Surface normal of hit
} RLRayCollision;

// BoundingBox
typedef struct RLBoundingBox {
    RLVector3 min;            // Minimum vertex box-corner
    RLVector3 max;            // Maximum vertex box-corner
} RLBoundingBox;

// Wave, audio wave data
typedef struct RLWave {
    unsigned int frameCount;    // Total number of frames (considering channels)
    unsigned int sampleRate;    // Frequency (samples per second)
    unsigned int sampleSize;    // Bit depth (bits per sample): 8, 16, 32 (24 not supported)
    unsigned int channels;      // Number of channels (1-mono, 2-stereo, ...)
    void *data;                 // Buffer data pointer
} RLWave;

// Opaque structs declaration
// NOTE: Actual structs are defined internally in raudio module
typedef struct rAudioBuffer rAudioBuffer;
typedef struct rAudioProcessor rAudioProcessor;

// AudioStream, custom audio stream
typedef struct RLAudioStream {
    rAudioBuffer *buffer;       // Pointer to internal data used by the audio system
    rAudioProcessor *processor; // Pointer to internal data processor, useful for audio effects

    unsigned int sampleRate;    // Frequency (samples per second)
    unsigned int sampleSize;    // Bit depth (bits per sample): 8, 16, 32 (24 not supported)
    unsigned int channels;      // Number of channels (1-mono, 2-stereo, ...)
} RLAudioStream;

// Sound
typedef struct RLSound {
    RLAudioStream stream;         // Audio stream
    unsigned int frameCount;    // Total number of frames (considering channels)
} RLSound;

// Music, audio stream, anything longer than ~10 seconds should be streamed
typedef struct RLMusic {
    RLAudioStream stream;         // Audio stream
    unsigned int frameCount;    // Total number of frames (considering channels)
    bool looping;               // Music looping enable

    int ctxType;                // Type of music context (audio filetype)
    void *ctxData;              // Audio context data, depends on type
} RLMusic;

// VrDeviceInfo, Head-Mounted-Display device parameters
typedef struct RLVrDeviceInfo {
    int hResolution;                // Horizontal resolution in pixels
    int vResolution;                // Vertical resolution in pixels
    float hScreenSize;              // Horizontal size in meters
    float vScreenSize;              // Vertical size in meters
    float eyeToScreenDistance;      // Distance between eye and display in meters
    float lensSeparationDistance;   // Lens separation distance in meters
    float interpupillaryDistance;   // IPD (distance between pupils) in meters
    float lensDistortionValues[4];  // Lens distortion constant parameters
    float chromaAbCorrection[4];    // Chromatic aberration correction parameters
} RLVrDeviceInfo;

// VrStereoConfig, VR stereo rendering configuration for simulator
typedef struct RLVrStereoConfig {
    RLMatrix projection[2];           // VR projection matrices (per eye)
    RLMatrix viewOffset[2];           // VR view offset matrices (per eye)
    float leftLensCenter[2];        // VR left lens center
    float rightLensCenter[2];       // VR right lens center
    float leftScreenCenter[2];      // VR left screen center
    float rightScreenCenter[2];     // VR right screen center
    float scale[2];                 // VR distortion scale
    float scaleIn[2];               // VR distortion scale in
} RLVrStereoConfig;

// File path list
typedef struct RLFilePathList {
    unsigned int count;             // Filepaths entries count
    char **paths;                   // Filepaths entries
} RLFilePathList;

// Automation event
typedef struct RLAutomationEvent {
    unsigned int frame;             // Event frame
    unsigned int type;              // Event type (AutomationEventType)
    int params[4];                  // Event parameters (if required)
} RLAutomationEvent;

// Automation event list
typedef struct RLAutomationEventList {
    unsigned int capacity;          // Events max entries (MAX_AUTOMATION_EVENTS)
    unsigned int count;             // Events entries count
    RLAutomationEvent *events;        // Events entries
} RLAutomationEventList;

//----------------------------------------------------------------------------------
// Enumerators Definition
//----------------------------------------------------------------------------------
// System/Window config flags
// NOTE: Every bit registers one state (use it with bit masks)
// By default all flags are set to 0
typedef enum {
    FLAG_VSYNC_HINT         = 0x00000040,   // Set to try enabling V-Sync on GPU
    FLAG_FULLSCREEN_MODE    = 0x00000002,   // Set to run program in fullscreen
    FLAG_WINDOW_RESIZABLE   = 0x00000004,   // Set to allow resizable window
    FLAG_WINDOW_UNDECORATED = 0x00000008,   // Set to disable window decoration (frame and buttons)
    FLAG_WINDOW_HIDDEN      = 0x00000080,   // Set to hide window
    FLAG_WINDOW_MINIMIZED   = 0x00000200,   // Set to minimize window (iconify)
    FLAG_WINDOW_MAXIMIZED   = 0x00000400,   // Set to maximize window (expanded to monitor)
    FLAG_WINDOW_UNFOCUSED   = 0x00000800,   // Set to window non focused
    FLAG_WINDOW_TOPMOST     = 0x00001000,   // Set to window always on top
    FLAG_WINDOW_ALWAYS_RUN  = 0x00000100,   // Set to allow windows running while minimized
    FLAG_WINDOW_TRANSPARENT = 0x00000010,   // Set to allow transparent framebuffer
    FLAG_WINDOW_HIGHDPI     = 0x00002000,   // Set to support HighDPI
    FLAG_WINDOW_MOUSE_PASSTHROUGH = 0x00004000, // Set to support mouse passthrough, only supported when FLAG_WINDOW_UNDECORATED
    FLAG_BORDERLESS_WINDOWED_MODE = 0x00008000, // Set to run program in borderless windowed mode
    FLAG_MSAA_4X_HINT       = 0x00000020,   // Set to try enabling MSAA 4X
    FLAG_INTERLACED_HINT    = 0x00010000,   // Set to try enabling interlaced video format (for V3D)
    FLAG_WINDOW_EVENT_THREAD        = 0x00020000,   // [GLFW/Win32] Create a dedicated message/event thread for this window (render thread separated)
    FLAG_WINDOW_REFRESH_CALLBACK   = 0x00040000,   // [GLFW/Win32] Enable OS-driven refresh ticks during Win32 modal loops (move/size/menu); use with RLSetWindowRefreshCallback()
    FLAG_WINDOW_BROADCAST_WAKE    = 0x00080000    // [GLFW/Win32] Broadcast wake to all windows' render threads on shutdown/close (optional)
} RLConfigFlags;

// Trace log level
// NOTE: Organized by priority level
typedef enum {
    LOG_ALL = 0,        // Display all logs
    LOG_TRACE,          // Trace logging, intended for internal use only
    LOG_DEBUG,          // Debug logging, used for internal debugging, it should be disabled on release builds
    LOG_INFO,           // Info logging, used for program execution info
    LOG_WARNING,        // Warning logging, used on recoverable failures
    LOG_ERROR,          // Error logging, used on unrecoverable failures
    LOG_FATAL,          // Fatal logging, used to abort program: exit(EXIT_FAILURE)
    LOG_NONE            // Disable logging
} RLTraceLogLevel;

// Keyboard keys (US keyboard layout)
// NOTE: Use GetKeyPressed() to allow redefining required keys for alternative layouts
typedef enum {
    KEY_NULL            = 0,        // Key: NULL, used for no key pressed
    // Alphanumeric keys
    KEY_APOSTROPHE      = 39,       // Key: '
    KEY_COMMA           = 44,       // Key: ,
    KEY_MINUS           = 45,       // Key: -
    KEY_PERIOD          = 46,       // Key: .
    KEY_SLASH           = 47,       // Key: /
    KEY_ZERO            = 48,       // Key: 0
    KEY_ONE             = 49,       // Key: 1
    KEY_TWO             = 50,       // Key: 2
    KEY_THREE           = 51,       // Key: 3
    KEY_FOUR            = 52,       // Key: 4
    KEY_FIVE            = 53,       // Key: 5
    KEY_SIX             = 54,       // Key: 6
    KEY_SEVEN           = 55,       // Key: 7
    KEY_EIGHT           = 56,       // Key: 8
    KEY_NINE            = 57,       // Key: 9
    KEY_SEMICOLON       = 59,       // Key: ;
    KEY_EQUAL           = 61,       // Key: =
    KEY_A               = 65,       // Key: A | a
    KEY_B               = 66,       // Key: B | b
    KEY_C               = 67,       // Key: C | c
    KEY_D               = 68,       // Key: D | d
    KEY_E               = 69,       // Key: E | e
    KEY_F               = 70,       // Key: F | f
    KEY_G               = 71,       // Key: G | g
    KEY_H               = 72,       // Key: H | h
    KEY_I               = 73,       // Key: I | i
    KEY_J               = 74,       // Key: J | j
    KEY_K               = 75,       // Key: K | k
    KEY_L               = 76,       // Key: L | l
    KEY_M               = 77,       // Key: M | m
    KEY_N               = 78,       // Key: N | n
    KEY_O               = 79,       // Key: O | o
    KEY_P               = 80,       // Key: P | p
    KEY_Q               = 81,       // Key: Q | q
    KEY_R               = 82,       // Key: R | r
    KEY_S               = 83,       // Key: S | s
    KEY_T               = 84,       // Key: T | t
    KEY_U               = 85,       // Key: U | u
    KEY_V               = 86,       // Key: V | v
    KEY_W               = 87,       // Key: W | w
    KEY_X               = 88,       // Key: X | x
    KEY_Y               = 89,       // Key: Y | y
    KEY_Z               = 90,       // Key: Z | z
    KEY_LEFT_BRACKET    = 91,       // Key: [
    KEY_BACKSLASH       = 92,       // Key: '\'
    KEY_RIGHT_BRACKET   = 93,       // Key: ]
    KEY_GRAVE           = 96,       // Key: `
    // Function keys
    KEY_SPACE           = 32,       // Key: Space
    KEY_ESCAPE          = 256,      // Key: Esc
    KEY_ENTER           = 257,      // Key: Enter
    KEY_TAB             = 258,      // Key: Tab
    KEY_BACKSPACE       = 259,      // Key: Backspace
    KEY_INSERT          = 260,      // Key: Ins
    KEY_DELETE          = 261,      // Key: Del
    KEY_RIGHT           = 262,      // Key: Cursor right
    KEY_LEFT            = 263,      // Key: Cursor left
    KEY_DOWN            = 264,      // Key: Cursor down
    KEY_UP              = 265,      // Key: Cursor up
    KEY_PAGE_UP         = 266,      // Key: Page up
    KEY_PAGE_DOWN       = 267,      // Key: Page down
    KEY_HOME            = 268,      // Key: Home
    KEY_END             = 269,      // Key: End
    KEY_CAPS_LOCK       = 280,      // Key: Caps lock
    KEY_SCROLL_LOCK     = 281,      // Key: Scroll down
    KEY_NUM_LOCK        = 282,      // Key: Num lock
    KEY_PRINT_SCREEN    = 283,      // Key: Print screen
    KEY_PAUSE           = 284,      // Key: Pause
    KEY_F1              = 290,      // Key: F1
    KEY_F2              = 291,      // Key: F2
    KEY_F3              = 292,      // Key: F3
    KEY_F4              = 293,      // Key: F4
    KEY_F5              = 294,      // Key: F5
    KEY_F6              = 295,      // Key: F6
    KEY_F7              = 296,      // Key: F7
    KEY_F8              = 297,      // Key: F8
    KEY_F9              = 298,      // Key: F9
    KEY_F10             = 299,      // Key: F10
    KEY_F11             = 300,      // Key: F11
    KEY_F12             = 301,      // Key: F12
    KEY_LEFT_SHIFT      = 340,      // Key: Shift left
    KEY_LEFT_CONTROL    = 341,      // Key: Control left
    KEY_LEFT_ALT        = 342,      // Key: Alt left
    KEY_LEFT_SUPER      = 343,      // Key: Super left
    KEY_RIGHT_SHIFT     = 344,      // Key: Shift right
    KEY_RIGHT_CONTROL   = 345,      // Key: Control right
    KEY_RIGHT_ALT       = 346,      // Key: Alt right
    KEY_RIGHT_SUPER     = 347,      // Key: Super right
    KEY_KB_MENU         = 348,      // Key: KB menu
    // Keypad keys
    KEY_KP_0            = 320,      // Key: Keypad 0
    KEY_KP_1            = 321,      // Key: Keypad 1
    KEY_KP_2            = 322,      // Key: Keypad 2
    KEY_KP_3            = 323,      // Key: Keypad 3
    KEY_KP_4            = 324,      // Key: Keypad 4
    KEY_KP_5            = 325,      // Key: Keypad 5
    KEY_KP_6            = 326,      // Key: Keypad 6
    KEY_KP_7            = 327,      // Key: Keypad 7
    KEY_KP_8            = 328,      // Key: Keypad 8
    KEY_KP_9            = 329,      // Key: Keypad 9
    KEY_KP_DECIMAL      = 330,      // Key: Keypad .
    KEY_KP_DIVIDE       = 331,      // Key: Keypad /
    KEY_KP_MULTIPLY     = 332,      // Key: Keypad *
    KEY_KP_SUBTRACT     = 333,      // Key: Keypad -
    KEY_KP_ADD          = 334,      // Key: Keypad +
    KEY_KP_ENTER        = 335,      // Key: Keypad Enter
    KEY_KP_EQUAL        = 336,      // Key: Keypad =
    // Android key buttons
    KEY_BACK            = 4,        // Key: Android back button
    KEY_MENU            = 5,        // Key: Android menu button
    KEY_VOLUME_UP       = 24,       // Key: Android volume up button
    KEY_VOLUME_DOWN     = 25        // Key: Android volume down button
} RLKeyboardKey;

// Add backwards compatibility support for deprecated names
#define MOUSE_LEFT_BUTTON   MOUSE_BUTTON_LEFT
#define MOUSE_RIGHT_BUTTON  MOUSE_BUTTON_RIGHT
#define MOUSE_MIDDLE_BUTTON MOUSE_BUTTON_MIDDLE

// Mouse buttons
typedef enum {
    MOUSE_BUTTON_LEFT    = 0,       // Mouse button left
    MOUSE_BUTTON_RIGHT   = 1,       // Mouse button right
    MOUSE_BUTTON_MIDDLE  = 2,       // Mouse button middle (pressed wheel)
    MOUSE_BUTTON_SIDE    = 3,       // Mouse button side (advanced mouse device)
    MOUSE_BUTTON_EXTRA   = 4,       // Mouse button extra (advanced mouse device)
    MOUSE_BUTTON_FORWARD = 5,       // Mouse button forward (advanced mouse device)
    MOUSE_BUTTON_BACK    = 6,       // Mouse button back (advanced mouse device)
} RLMouseButton;

// Mouse cursor
typedef enum {
    MOUSE_CURSOR_DEFAULT       = 0,     // Default pointer shape
    MOUSE_CURSOR_ARROW         = 1,     // Arrow shape
    MOUSE_CURSOR_IBEAM         = 2,     // Text writing cursor shape
    MOUSE_CURSOR_CROSSHAIR     = 3,     // Cross shape
    MOUSE_CURSOR_POINTING_HAND = 4,     // Pointing hand cursor
    MOUSE_CURSOR_RESIZE_EW     = 5,     // Horizontal resize/move arrow shape
    MOUSE_CURSOR_RESIZE_NS     = 6,     // Vertical resize/move arrow shape
    MOUSE_CURSOR_RESIZE_NWSE   = 7,     // Top-left to bottom-right diagonal resize/move arrow shape
    MOUSE_CURSOR_RESIZE_NESW   = 8,     // The top-right to bottom-left diagonal resize/move arrow shape
    MOUSE_CURSOR_RESIZE_ALL    = 9,     // The omnidirectional resize/move cursor shape
    MOUSE_CURSOR_NOT_ALLOWED   = 10     // The operation-not-allowed shape
} RLMouseCursor;

// Gamepad buttons
typedef enum {
    GAMEPAD_BUTTON_UNKNOWN = 0,         // Unknown button, just for error checking
    GAMEPAD_BUTTON_LEFT_FACE_UP,        // Gamepad left DPAD up button
    GAMEPAD_BUTTON_LEFT_FACE_RIGHT,     // Gamepad left DPAD right button
    GAMEPAD_BUTTON_LEFT_FACE_DOWN,      // Gamepad left DPAD down button
    GAMEPAD_BUTTON_LEFT_FACE_LEFT,      // Gamepad left DPAD left button
    GAMEPAD_BUTTON_RIGHT_FACE_UP,       // Gamepad right button up (i.e. PS3: Triangle, Xbox: Y)
    GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,    // Gamepad right button right (i.e. PS3: Circle, Xbox: B)
    GAMEPAD_BUTTON_RIGHT_FACE_DOWN,     // Gamepad right button down (i.e. PS3: Cross, Xbox: A)
    GAMEPAD_BUTTON_RIGHT_FACE_LEFT,     // Gamepad right button left (i.e. PS3: Square, Xbox: X)
    GAMEPAD_BUTTON_LEFT_TRIGGER_1,      // Gamepad top/back trigger left (first), it could be a trailing button
    GAMEPAD_BUTTON_LEFT_TRIGGER_2,      // Gamepad top/back trigger left (second), it could be a trailing button
    GAMEPAD_BUTTON_RIGHT_TRIGGER_1,     // Gamepad top/back trigger right (first), it could be a trailing button
    GAMEPAD_BUTTON_RIGHT_TRIGGER_2,     // Gamepad top/back trigger right (second), it could be a trailing button
    GAMEPAD_BUTTON_MIDDLE_LEFT,         // Gamepad center buttons, left one (i.e. PS3: Select)
    GAMEPAD_BUTTON_MIDDLE,              // Gamepad center buttons, middle one (i.e. PS3: PS, Xbox: XBOX)
    GAMEPAD_BUTTON_MIDDLE_RIGHT,        // Gamepad center buttons, right one (i.e. PS3: Start)
    GAMEPAD_BUTTON_LEFT_THUMB,          // Gamepad joystick pressed button left
    GAMEPAD_BUTTON_RIGHT_THUMB          // Gamepad joystick pressed button right
} RLGamepadButton;

// Gamepad axes
typedef enum {
    GAMEPAD_AXIS_LEFT_X        = 0,     // Gamepad left stick X axis
    GAMEPAD_AXIS_LEFT_Y        = 1,     // Gamepad left stick Y axis
    GAMEPAD_AXIS_RIGHT_X       = 2,     // Gamepad right stick X axis
    GAMEPAD_AXIS_RIGHT_Y       = 3,     // Gamepad right stick Y axis
    GAMEPAD_AXIS_LEFT_TRIGGER  = 4,     // Gamepad back trigger left, pressure level: [1..-1]
    GAMEPAD_AXIS_RIGHT_TRIGGER = 5      // Gamepad back trigger right, pressure level: [1..-1]
} RLGamepadAxis;

// Material map index
typedef enum {
    MATERIAL_MAP_ALBEDO = 0,        // Albedo material (same as: MATERIAL_MAP_DIFFUSE)
    MATERIAL_MAP_METALNESS,         // Metalness material (same as: MATERIAL_MAP_SPECULAR)
    MATERIAL_MAP_NORMAL,            // Normal material
    MATERIAL_MAP_ROUGHNESS,         // Roughness material
    MATERIAL_MAP_OCCLUSION,         // Ambient occlusion material
    MATERIAL_MAP_EMISSION,          // Emission material
    MATERIAL_MAP_HEIGHT,            // Heightmap material
    MATERIAL_MAP_CUBEMAP,           // Cubemap material (NOTE: Uses GL_TEXTURE_CUBE_MAP)
    MATERIAL_MAP_IRRADIANCE,        // Irradiance material (NOTE: Uses GL_TEXTURE_CUBE_MAP)
    MATERIAL_MAP_PREFILTER,         // Prefilter material (NOTE: Uses GL_TEXTURE_CUBE_MAP)
    MATERIAL_MAP_BRDF               // Brdf material
} RLMaterialMapIndex;

#define MATERIAL_MAP_DIFFUSE      MATERIAL_MAP_ALBEDO
#define MATERIAL_MAP_SPECULAR     MATERIAL_MAP_METALNESS

// Shader location index
typedef enum {
    SHADER_LOC_VERTEX_POSITION = 0, // Shader location: vertex attribute: position
    SHADER_LOC_VERTEX_TEXCOORD01,   // Shader location: vertex attribute: texcoord01
    SHADER_LOC_VERTEX_TEXCOORD02,   // Shader location: vertex attribute: texcoord02
    SHADER_LOC_VERTEX_NORMAL,       // Shader location: vertex attribute: normal
    SHADER_LOC_VERTEX_TANGENT,      // Shader location: vertex attribute: tangent
    SHADER_LOC_VERTEX_COLOR,        // Shader location: vertex attribute: color
    SHADER_LOC_MATRIX_MVP,          // Shader location: matrix uniform: model-view-projection
    SHADER_LOC_MATRIX_VIEW,         // Shader location: matrix uniform: view (camera transform)
    SHADER_LOC_MATRIX_PROJECTION,   // Shader location: matrix uniform: projection
    SHADER_LOC_MATRIX_MODEL,        // Shader location: matrix uniform: model (transform)
    SHADER_LOC_MATRIX_NORMAL,       // Shader location: matrix uniform: normal
    SHADER_LOC_VECTOR_VIEW,         // Shader location: vector uniform: view
    SHADER_LOC_COLOR_DIFFUSE,       // Shader location: vector uniform: diffuse color
    SHADER_LOC_COLOR_SPECULAR,      // Shader location: vector uniform: specular color
    SHADER_LOC_COLOR_AMBIENT,       // Shader location: vector uniform: ambient color
    SHADER_LOC_MAP_ALBEDO,          // Shader location: sampler2d texture: albedo (same as: SHADER_LOC_MAP_DIFFUSE)
    SHADER_LOC_MAP_METALNESS,       // Shader location: sampler2d texture: metalness (same as: SHADER_LOC_MAP_SPECULAR)
    SHADER_LOC_MAP_NORMAL,          // Shader location: sampler2d texture: normal
    SHADER_LOC_MAP_ROUGHNESS,       // Shader location: sampler2d texture: roughness
    SHADER_LOC_MAP_OCCLUSION,       // Shader location: sampler2d texture: occlusion
    SHADER_LOC_MAP_EMISSION,        // Shader location: sampler2d texture: emission
    SHADER_LOC_MAP_HEIGHT,          // Shader location: sampler2d texture: height
    SHADER_LOC_MAP_CUBEMAP,         // Shader location: samplerCube texture: cubemap
    SHADER_LOC_MAP_IRRADIANCE,      // Shader location: samplerCube texture: irradiance
    SHADER_LOC_MAP_PREFILTER,       // Shader location: samplerCube texture: prefilter
    SHADER_LOC_MAP_BRDF,            // Shader location: sampler2d texture: brdf
    SHADER_LOC_VERTEX_BONEIDS,      // Shader location: vertex attribute: boneIds
    SHADER_LOC_VERTEX_BONEWEIGHTS,  // Shader location: vertex attribute: boneWeights
    SHADER_LOC_BONE_MATRICES,       // Shader location: array of matrices uniform: boneMatrices
    SHADER_LOC_VERTEX_INSTANCE_TX   // Shader location: vertex attribute: instanceTransform
} RLShaderLocationIndex;

#define SHADER_LOC_MAP_DIFFUSE      SHADER_LOC_MAP_ALBEDO
#define SHADER_LOC_MAP_SPECULAR     SHADER_LOC_MAP_METALNESS

// Shader uniform data type
typedef enum {
    SHADER_UNIFORM_FLOAT = 0,       // Shader uniform type: float
    SHADER_UNIFORM_VEC2,            // Shader uniform type: vec2 (2 float)
    SHADER_UNIFORM_VEC3,            // Shader uniform type: vec3 (3 float)
    SHADER_UNIFORM_VEC4,            // Shader uniform type: vec4 (4 float)
    SHADER_UNIFORM_INT,             // Shader uniform type: int
    SHADER_UNIFORM_IVEC2,           // Shader uniform type: ivec2 (2 int)
    SHADER_UNIFORM_IVEC3,           // Shader uniform type: ivec3 (3 int)
    SHADER_UNIFORM_IVEC4,           // Shader uniform type: ivec4 (4 int)
    SHADER_UNIFORM_UINT,            // Shader uniform type: unsigned int
    SHADER_UNIFORM_UIVEC2,          // Shader uniform type: uivec2 (2 unsigned int)
    SHADER_UNIFORM_UIVEC3,          // Shader uniform type: uivec3 (3 unsigned int)
    SHADER_UNIFORM_UIVEC4,          // Shader uniform type: uivec4 (4 unsigned int)
    SHADER_UNIFORM_SAMPLER2D        // Shader uniform type: sampler2d
} RLShaderUniformDataType;

// Shader attribute data types
typedef enum {
    SHADER_ATTRIB_FLOAT = 0,        // Shader attribute type: float
    SHADER_ATTRIB_VEC2,             // Shader attribute type: vec2 (2 float)
    SHADER_ATTRIB_VEC3,             // Shader attribute type: vec3 (3 float)
    SHADER_ATTRIB_VEC4              // Shader attribute type: vec4 (4 float)
} RLShaderAttributeDataType;

// Pixel formats
// NOTE: Support depends on OpenGL version and platform
typedef enum {
    PIXELFORMAT_UNCOMPRESSED_GRAYSCALE = 1, // 8 bit per pixel (no alpha)
    PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA,    // 8*2 bpp (2 channels)
    PIXELFORMAT_UNCOMPRESSED_R5G6B5,        // 16 bpp
    PIXELFORMAT_UNCOMPRESSED_R8G8B8,        // 24 bpp
    PIXELFORMAT_UNCOMPRESSED_R5G5B5A1,      // 16 bpp (1 bit alpha)
    PIXELFORMAT_UNCOMPRESSED_R4G4B4A4,      // 16 bpp (4 bit alpha)
    PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,      // 32 bpp
    PIXELFORMAT_UNCOMPRESSED_R32,           // 32 bpp (1 channel - float)
    PIXELFORMAT_UNCOMPRESSED_R32G32B32,     // 32*3 bpp (3 channels - float)
    PIXELFORMAT_UNCOMPRESSED_R32G32B32A32,  // 32*4 bpp (4 channels - float)
    PIXELFORMAT_UNCOMPRESSED_R16,           // 16 bpp (1 channel - half float)
    PIXELFORMAT_UNCOMPRESSED_R16G16B16,     // 16*3 bpp (3 channels - half float)
    PIXELFORMAT_UNCOMPRESSED_R16G16B16A16,  // 16*4 bpp (4 channels - half float)
    PIXELFORMAT_COMPRESSED_DXT1_RGB,        // 4 bpp (no alpha)
    PIXELFORMAT_COMPRESSED_DXT1_RGBA,       // 4 bpp (1 bit alpha)
    PIXELFORMAT_COMPRESSED_DXT3_RGBA,       // 8 bpp
    PIXELFORMAT_COMPRESSED_DXT5_RGBA,       // 8 bpp
    PIXELFORMAT_COMPRESSED_ETC1_RGB,        // 4 bpp
    PIXELFORMAT_COMPRESSED_ETC2_RGB,        // 4 bpp
    PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA,   // 8 bpp
    PIXELFORMAT_COMPRESSED_PVRT_RGB,        // 4 bpp
    PIXELFORMAT_COMPRESSED_PVRT_RGBA,       // 4 bpp
    PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA,   // 8 bpp
    PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA    // 2 bpp
} RLPixelFormat;

// Texture parameters: filter mode
// NOTE 1: Filtering considers mipmaps if available in the texture
// NOTE 2: Filter is accordingly set for minification and magnification
typedef enum {
    TEXTURE_FILTER_POINT = 0,               // No filter, just pixel approximation
    TEXTURE_FILTER_BILINEAR,                // Linear filtering
    TEXTURE_FILTER_TRILINEAR,               // Trilinear filtering (linear with mipmaps)
    TEXTURE_FILTER_ANISOTROPIC_4X,          // Anisotropic filtering 4x
    TEXTURE_FILTER_ANISOTROPIC_8X,          // Anisotropic filtering 8x
    TEXTURE_FILTER_ANISOTROPIC_16X,         // Anisotropic filtering 16x
} RLTextureFilter;

// Texture parameters: wrap mode
typedef enum {
    TEXTURE_WRAP_REPEAT = 0,                // Repeats texture in tiled mode
    TEXTURE_WRAP_CLAMP,                     // Clamps texture to edge pixel in tiled mode
    TEXTURE_WRAP_MIRROR_REPEAT,             // Mirrors and repeats the texture in tiled mode
    TEXTURE_WRAP_MIRROR_CLAMP               // Mirrors and clamps to border the texture in tiled mode
} RLTextureWrap;

// Cubemap layouts
typedef enum {
    CUBEMAP_LAYOUT_AUTO_DETECT = 0,         // Automatically detect layout type
    CUBEMAP_LAYOUT_LINE_VERTICAL,           // Layout is defined by a vertical line with faces
    CUBEMAP_LAYOUT_LINE_HORIZONTAL,         // Layout is defined by a horizontal line with faces
    CUBEMAP_LAYOUT_CROSS_THREE_BY_FOUR,     // Layout is defined by a 3x4 cross with cubemap faces
    CUBEMAP_LAYOUT_CROSS_FOUR_BY_THREE     // Layout is defined by a 4x3 cross with cubemap faces
} RLCubemapLayout;

// Font type, defines generation method
typedef enum {
    FONT_DEFAULT = 0,               // Default font generation, anti-aliased
    FONT_BITMAP,                    // Bitmap font generation, no anti-aliasing
    FONT_SDF                        // SDF font generation, requires external shader
} RLFontType;

// Color blending modes (pre-defined)
typedef enum {
    BLEND_ALPHA = 0,                // Blend textures considering alpha (default)
    BLEND_ADDITIVE,                 // Blend textures adding colors
    BLEND_MULTIPLIED,               // Blend textures multiplying colors
    BLEND_ADD_COLORS,               // Blend textures adding colors (alternative)
    BLEND_SUBTRACT_COLORS,          // Blend textures subtracting colors (alternative)
    BLEND_ALPHA_PREMULTIPLY,        // Blend premultiplied textures considering alpha
    BLEND_CUSTOM,                   // Blend textures using custom src/dst factors (use rlSetBlendFactors())
    BLEND_CUSTOM_SEPARATE           // Blend textures using custom rgb/alpha separate src/dst factors (use rlSetBlendFactorsSeparate())
} RLBlendMode;

// Gesture
// NOTE: Provided as bit-wise flags to enable only desired gestures
typedef enum {
    GESTURE_NONE        = 0,        // No gesture
    GESTURE_TAP         = 1,        // Tap gesture
    GESTURE_DOUBLETAP   = 2,        // Double tap gesture
    GESTURE_HOLD        = 4,        // Hold gesture
    GESTURE_DRAG        = 8,        // Drag gesture
    GESTURE_SWIPE_RIGHT = 16,       // Swipe right gesture
    GESTURE_SWIPE_LEFT  = 32,       // Swipe left gesture
    GESTURE_SWIPE_UP    = 64,       // Swipe up gesture
    GESTURE_SWIPE_DOWN  = 128,      // Swipe down gesture
    GESTURE_PINCH_IN    = 256,      // Pinch in gesture
    GESTURE_PINCH_OUT   = 512       // Pinch out gesture
} RLGesture;

// Camera system modes
typedef enum {
    CAMERA_CUSTOM = 0,              // Camera custom, controlled by user (UpdateCamera() does nothing)
    CAMERA_FREE,                    // Camera free mode
    CAMERA_ORBITAL,                 // Camera orbital, around target, zoom supported
    CAMERA_FIRST_PERSON,            // Camera first person
    CAMERA_THIRD_PERSON             // Camera third person
} RLCameraMode;

// Camera projection
typedef enum {
    CAMERA_PERSPECTIVE = 0,         // Perspective projection
    CAMERA_ORTHOGRAPHIC             // Orthographic projection
} RLCameraProjection;

// N-patch layout
typedef enum {
    NPATCH_NINE_PATCH = 0,          // Npatch layout: 3x3 tiles
    NPATCH_THREE_PATCH_VERTICAL,    // Npatch layout: 1x3 tiles
    NPATCH_THREE_PATCH_HORIZONTAL   // Npatch layout: 3x1 tiles
} RLNPatchLayout;

// Callbacks to hook some internal functions
// WARNING: These callbacks are intended for advanced users
typedef void (*RLTraceLogCallback)(int logLevel, const char *text, va_list args);  // Logging: Redirect trace log messages
typedef unsigned char *(*RLLoadFileDataCallback)(const char *fileName, int *dataSize);    // FileIO: Load binary data
typedef bool (*RLSaveFileDataCallback)(const char *fileName, void *data, int dataSize);   // FileIO: Save binary data
typedef char *(*RLLoadFileTextCallback)(const char *fileName);            // FileIO: Load text data
typedef bool (*RLSaveFileTextCallback)(const char *fileName, const char *text); // FileIO: Save text data


// Window refresh callback
// NOTE: When FLAG_WINDOW_REFRESH_CALLBACK is enabled (Win32/GLFW), this callback can be invoked while the OS is in a modal loop
// (e.g. interactive move/size or menu tracking) to allow the application to redraw without using a separate event thread.
// The callback is executed with a valid OpenGL context current on the calling thread.
typedef void (*RLWindowRefreshCallback)(void);

//------------------------------------------------------------------------------------
// Global Variables Definition
//------------------------------------------------------------------------------------
// It's lonely here...

//------------------------------------------------------------------------------------
// Window and Graphics Device Functions (Module: core)
//------------------------------------------------------------------------------------

// Context management (Route2): opaque context handle.
// By default, a per-thread default context is created on first use.
typedef struct RLContext RLContext;

#if defined(__cplusplus)
extern "C" {            // Prevents name mangling of functions
#endif

// Context-related functions
RLAPI RLContext *RLCreateContext(void);
RLAPI void RLDestroyContext(RLContext *ctx);
RLAPI void RLSetCurrentContext(RLContext *ctx);
RLAPI RLContext *RLGetCurrentContext(void);

// Window-related functions
RLAPI void RLInitWindow(int width, int height, const char *title);  // Initialize window and OpenGL context
RLAPI void RLInitWindowEx(int width, int height, const char *title, const char *win32ClassName); // Initialize window with optional Win32 class name (desktop+GLFW on Windows)
RLAPI void RLCloseWindow(void);                                     // Close window and unload OpenGL context
RLAPI bool RLWindowShouldClose(void);                               // Check if application should close (KEY_ESCAPE pressed or windows close icon clicked)
RLAPI bool RLIsWindowReady(void);                                   // Check if window has been initialized successfully
RLAPI bool RLIsWindowFullscreen(void);                              // Check if window is currently fullscreen
RLAPI bool RLIsWindowHidden(void);                                  // Check if window is currently hidden
RLAPI bool RLIsWindowMinimized(void);                               // Check if window is currently minimized
RLAPI bool RLIsWindowMaximized(void);                               // Check if window is currently maximized
RLAPI bool RLIsWindowFocused(void);                                 // Check if window is currently focused
RLAPI bool RLIsWindowResized(void);                                 // Check if window has been resized last frame
RLAPI bool RLIsWindowState(unsigned int flag);                      // Check if one specific window flag is enabled
RLAPI void RLSetWindowState(unsigned int flags);                    // Set window configuration state using flags
RLAPI void RLClearWindowState(unsigned int flags);                  // Clear window configuration state flags
RLAPI void RLToggleFullscreen(void);                                // Toggle window state: fullscreen/windowed, resizes monitor to match window resolution
RLAPI void RLToggleBorderlessWindowed(void);                        // Toggle window state: borderless windowed, resizes window to match monitor resolution
RLAPI void RLMaximizeWindow(void);                                  // Set window state: maximized, if resizable
RLAPI void RLMinimizeWindow(void);                                  // Set window state: minimized, if resizable
RLAPI void RLRestoreWindow(void);                                   // Restore window from being minimized/maximized
RLAPI void RLSetWindowIcon(RLImage image);                            // Set icon for window (single image, RGBA 32bit)
RLAPI void RLSetWindowIcons(RLImage *images, int count);              // Set icon for window (multiple images, RGBA 32bit)
RLAPI void RLSetWindowTitle(const char *title);                     // Set title for window
RLAPI void RLSetWindowPosition(int x, int y);                       // Set window position on screen
RLAPI void RLSetWindowMonitor(int monitor);                         // Set monitor for the current window
RLAPI void RLSetWindowMinSize(int width, int height);               // Set window minimum dimensions (for FLAG_WINDOW_RESIZABLE)
RLAPI void RLSetWindowMaxSize(int width, int height);               // Set window maximum dimensions (for FLAG_WINDOW_RESIZABLE)
RLAPI void RLSetWindowSize(int width, int height);                  // Set window dimensions
RLAPI void RLSetWindowOpacity(float opacity);                       // Set window opacity [0.0f..1.0f]
RLAPI void RLSetWindowFocused(void);                                // Set window focused
RLAPI void *RLGetWindowHandle(void);                                // Get native window handle

#if defined(_WIN32)
// Win32 helpers (property bag + message hooks)
// NOTE: Available only when using the GLFW desktop backend on Windows.
//       Property values are stored with SetProp/GetProp/RemoveProp on the underlying HWND.
//
// Return non-zero to mark the message as handled. If handled, *result will be returned by the window proc.
typedef int (*RLWin32MessageHook)(void* hwnd, unsigned int uMsg, uintptr_t wParam, intptr_t lParam, intptr_t* result, void* user);

RLAPI int RLWin32SetWindowProp(const char* name, void* value);
RLAPI void* RLWin32GetWindowProp(const char* name);
RLAPI void* RLWin32RemoveWindowProp(const char* name);

// Returns an opaque token to be passed to RLWin32RemoveMessageHook.
RLAPI void* RLWin32AddMessageHook(RLWin32MessageHook hook, void* user);
RLAPI int RLWin32RemoveMessageHook(void* token);

// Global window management (process-wide)
// If outHwnds is NULL or maxCount <= 0, returns the number of currently tracked raylib windows.
RLAPI int RLWin32GetAllWindowHandles(void** outHwnds, int maxCount);
RLAPI void* RLWin32GetPrimaryWindowHandle(void);
RLAPI int RLWin32IsKnownWindowHandle(void* hwnd);

// Cross-thread helpers (operate on a specific raylib window by HWND)
RLAPI int RLWin32SetWindowPropByHandle(void* hwnd, const char* name, void* value);
RLAPI void* RLWin32GetWindowPropByHandle(void* hwnd, const char* name);
RLAPI void* RLWin32RemoveWindowPropByHandle(void* hwnd, const char* name);

RLAPI void* RLWin32AddMessageHookByHandle(void* hwnd, RLWin32MessageHook hook, void* user);
RLAPI int RLWin32RemoveMessageHookByHandle(void* hwnd, void* token);
// Generic cross-thread dispatch primitives (advanced)
// NOTE: Window-thread invoke runs on the Win32 thread that owns the HWND (safe for Win32 UI ops).
#ifndef RL_WIN32_WINDOW_THREAD_INVOKE_DEFINED
#define RL_WIN32_WINDOW_THREAD_INVOKE_DEFINED
typedef intptr_t (*RLWin32WindowThreadInvoke)(void* hwnd, void* user);
#endif
RLAPI intptr_t RLWin32InvokeOnWindowThreadByHandle(void* hwnd, RLWin32WindowThreadInvoke fn, void* user, int wait);

// NOTE: Render-thread invoke runs on the render thread of the target window (safe for raylib drawing/GL for that window).
// In non-event-thread mode, this only works when called from the same thread that owns the target OpenGL context.
#ifndef RL_WINDOW_RENDER_THREAD_INVOKE_DEFINED
#define RL_WINDOW_RENDER_THREAD_INVOKE_DEFINED
typedef intptr_t (*RLWindowRenderThreadInvoke)(void* hwnd, void* user);
#endif
RLAPI intptr_t RLInvokeOnWindowRenderThreadByHandle(void* hwnd, RLWindowRenderThreadInvoke fn, void* user, int wait);

#endif

RLAPI void RLSetWindowWin32ClassName(const char *win32ClassName);   // Set one-shot Win32 class name for next window creation (desktop+GLFW on Windows)
RLAPI int RLGetScreenWidth(void);                                   // Get current screen width
RLAPI int RLGetScreenHeight(void);                                  // Get current screen height
RLAPI int RLGetRenderWidth(void);                                   // Get current render width (it considers HiDPI)
RLAPI int RLGetRenderHeight(void);                                  // Get current render height (it considers HiDPI)
RLAPI int RLGetMonitorCount(void);                                  // Get number of connected monitors
RLAPI int RLGetCurrentMonitor(void);                                // Get current monitor where window is placed
RLAPI RLVector2 RLGetMonitorPosition(int monitor);                    // Get specified monitor position
RLAPI int RLGetMonitorWidth(int monitor);                           // Get specified monitor width (current video mode used by monitor)
RLAPI int RLGetMonitorHeight(int monitor);                          // Get specified monitor height (current video mode used by monitor)
RLAPI int RLGetMonitorPhysicalWidth(int monitor);                   // Get specified monitor physical width in millimetres
RLAPI int RLGetMonitorPhysicalHeight(int monitor);                  // Get specified monitor physical height in millimetres
RLAPI int RLGetMonitorRefreshRate(int monitor);                     // Get specified monitor refresh rate
RLAPI RLVector2 RLGetWindowPosition(void);                            // Get window position XY on monitor
RLAPI RLVector2 RLGetWindowScaleDPI(void);                            // Get window scale DPI factor
RLAPI const char *RLGetMonitorName(int monitor);                    // Get the human-readable, UTF-8 encoded name of the specified monitor
RLAPI void RLSetClipboardText(const char *text);                    // Set clipboard text content
RLAPI const char *RLGetClipboardText(void);                         // Get clipboard text content
RLAPI RLImage RLGetClipboardImage(void);                              // Get clipboard image content
RLAPI void RLEnableEventWaiting(void);                              // Enable waiting for events on EndDrawing(), no automatic event polling
RLAPI void RLDisableEventWaiting(void);                             // Disable waiting for events on EndDrawing(), automatic events polling

// Cursor-related functions
RLAPI void RLShowCursor(void);                                      // Shows cursor
RLAPI void RLHideCursor(void);                                      // Hides cursor
RLAPI bool RLIsCursorHidden(void);                                  // Check if cursor is not visible
RLAPI void RLEnableCursor(void);                                    // Enables cursor (unlock cursor)
RLAPI void RLDisableCursor(void);                                   // Disables cursor (lock cursor)
RLAPI bool RLIsCursorOnScreen(void);                                // Check if cursor is on the screen

// Drawing-related functions
RLAPI void RLClearBackground(RLColor color);                          // Set background color (framebuffer clear color)
RLAPI void RLBeginDrawing(void);                                    // Setup canvas (framebuffer) to start drawing
RLAPI void RLEndDrawing(void);                                      // End canvas drawing and swap buffers (double buffering)
RLAPI void RLBeginMode2D(RLCamera2D camera);                          // Begin 2D mode with custom camera (2D)
RLAPI void RLEndMode2D(void);                                       // Ends 2D mode with custom camera
RLAPI void RLBeginMode3D(RLCamera3D camera);                          // Begin 3D mode with custom camera (3D)
RLAPI void RLEndMode3D(void);                                       // Ends 3D mode and returns to default 2D orthographic mode
RLAPI void RLBeginTextureMode(RLRenderTexture2D target);              // Begin drawing to render texture
RLAPI void RLEndTextureMode(void);                                  // Ends drawing to render texture
RLAPI void RLBeginShaderMode(RLShader shader);                        // Begin custom shader drawing
RLAPI void RLEndShaderMode(void);                                   // End custom shader drawing (use default shader)
RLAPI void RLBeginBlendMode(int mode);                              // Begin blending mode (alpha, additive, multiplied, subtract, custom)
RLAPI void RLEndBlendMode(void);                                    // End blending mode (reset to default: alpha blending)
RLAPI void RLBeginScissorMode(int x, int y, int width, int height); // Begin scissor mode (define screen area for following drawing)
RLAPI void RLEndScissorMode(void);                                  // End scissor mode
RLAPI void RLBeginVrStereoMode(RLVrStereoConfig config);              // Begin stereo rendering (requires VR simulator)
RLAPI void RLEndVrStereoMode(void);                                 // End stereo rendering (requires VR simulator)

// VR stereo config functions for VR simulator
RLAPI RLVrStereoConfig RLLoadVrStereoConfig(RLVrDeviceInfo device);     // Load VR stereo config for VR simulator device parameters
RLAPI void RLUnloadVrStereoConfig(RLVrStereoConfig config);           // Unload VR stereo config

// Shader management functions
// NOTE: Shader functionality is not available on OpenGL 1.1
RLAPI RLShader RLLoadShader(const char *vsFileName, const char *fsFileName);   // Load shader from files and bind default locations
RLAPI RLShader RLLoadShaderFromMemory(const char *vsCode, const char *fsCode); // Load shader from code strings and bind default locations
RLAPI bool RLIsShaderValid(RLShader shader);                                   // Check if a shader is valid (loaded on GPU)
RLAPI int RLGetShaderLocation(RLShader shader, const char *uniformName);       // Get shader uniform location
RLAPI int RLGetShaderLocationAttrib(RLShader shader, const char *attribName);  // Get shader attribute location
RLAPI void RLSetShaderValue(RLShader shader, int locIndex, const void *value, int uniformType);               // Set shader uniform value
RLAPI void RLSetShaderValueV(RLShader shader, int locIndex, const void *value, int uniformType, int count);   // Set shader uniform value vector
RLAPI void RLSetShaderValueMatrix(RLShader shader, int locIndex, RLMatrix mat);         // Set shader uniform value (matrix 4x4)
RLAPI void RLSetShaderValueTexture(RLShader shader, int locIndex, RLTexture2D texture); // Set shader uniform value and bind the texture (sampler2d)
RLAPI void RLUnloadShader(RLShader shader);                                    // Unload shader from GPU memory (VRAM)

// Screen-space-related functions
#define GetMouseRay RLGetScreenToWorldRay     // Compatibility hack for previous raylib versions
RLAPI RLRay RLGetScreenToWorldRay(RLVector2 position, RLCamera camera);         // Get a ray trace from screen position (i.e mouse)
RLAPI RLRay RLGetScreenToWorldRayEx(RLVector2 position, RLCamera camera, int width, int height); // Get a ray trace from screen position (i.e mouse) in a viewport
RLAPI RLVector2 RLGetWorldToScreen(RLVector3 position, RLCamera camera);        // Get the screen space position for a 3d world space position
RLAPI RLVector2 RLGetWorldToScreenEx(RLVector3 position, RLCamera camera, int width, int height); // Get size position for a 3d world space position
RLAPI RLVector2 RLGetWorldToScreen2D(RLVector2 position, RLCamera2D camera);    // Get the screen space position for a 2d camera world space position
RLAPI RLVector2 RLGetScreenToWorld2D(RLVector2 position, RLCamera2D camera);    // Get the world space position for a 2d camera screen space position
RLAPI RLMatrix RLGetCameraMatrix(RLCamera camera);                            // Get camera transform matrix (view matrix)
RLAPI RLMatrix RLGetCameraMatrix2D(RLCamera2D camera);                        // Get camera 2d transform matrix

// Timing-related functions
RLAPI void RLSetTargetFPS(int fps);                       // Set target FPS (maximum)
RLAPI float RLGetFrameTime(void);                         // Get time in seconds for last frame drawn (delta time)
RLAPI double RLGetTime(void);                             // Get elapsed time in seconds since InitWindow()
RLAPI int RLGetFPS(void);                                 // Get current FPS

// Custom frame control functions
// NOTE: Those functions are intended for advanced users that want full control over the frame processing
// By default EndDrawing() does this job: draws everything + SwapScreenBuffer() + manage frame timing + PollInputEvents()
// To avoid that behaviour and control frame processes manually, enable in config.h: SUPPORT_CUSTOM_FRAME_CONTROL
RLAPI void RLSwapScreenBuffer(void);                      // Swap back buffer with front buffer (screen drawing)
RLAPI void RLPollInputEvents(void);                       // Register all input events
RLAPI void RLWaitTime(double seconds);                    // Wait for some time (halt program execution)

// Random values generation functions
RLAPI void RLSetRandomSeed(unsigned int seed);            // Set the seed for the random number generator
RLAPI int RLGetRandomValue(int min, int max);             // Get a random value between min and max (both included)
RLAPI int *RLLoadRandomSequence(unsigned int count, int min, int max); // Load random values sequence, no values repeated
RLAPI void RLUnloadRandomSequence(int *sequence);         // Unload random values sequence

// Misc. functions
RLAPI void RLTakeScreenshot(const char *fileName);                // Takes a screenshot of current screen (filename extension defines format)
RLAPI void RLSetConfigFlags(unsigned int flags);                  // Setup init configuration flags (view FLAGS)
RLAPI void RLSetWindowRefreshCallback(RLWindowRefreshCallback callback); // Set user window refresh callback (used by FLAG_WINDOW_REFRESH_CALLBACK)
RLAPI void RLOpenURL(const char *url);                            // Open URL with default system browser (if available)

// Logging system
RLAPI void RLSetTraceLogLevel(int logLevel);                      // Set the current threshold (minimum) log level
RLAPI void RLTraceLog(int logLevel, const char *text, ...);       // Show trace log messages (LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR...)
RLAPI void RLSetTraceLogCallback(RLTraceLogCallback callback);      // Set custom trace log

// Memory management, using internal allocators
RLAPI void *RLMemAlloc(unsigned int size);                        // Internal memory allocator
RLAPI void *RLMemRealloc(void *ptr, unsigned int size);           // Internal memory reallocator
RLAPI void RLMemFree(void *ptr);                                  // Internal memory free

// File system management functions
RLAPI unsigned char *RLLoadFileData(const char *fileName, int *dataSize); // Load file data as byte array (read)
RLAPI void RLUnloadFileData(unsigned char *data);                     // Unload file data allocated by LoadFileData()
RLAPI bool RLSaveFileData(const char *fileName, void *data, int dataSize); // Save data to file from byte array (write), returns true on success
RLAPI bool RLExportDataAsCode(const unsigned char *data, int dataSize, const char *fileName); // Export data to code (.h), returns true on success
RLAPI char *RLLoadFileText(const char *fileName);                     // Load text data from file (read), returns a '\0' terminated string
RLAPI void RLUnloadFileText(char *text);                              // Unload file text data allocated by LoadFileText()
RLAPI bool RLSaveFileText(const char *fileName, const char *text);    // Save text data to file (write), string must be '\0' terminated, returns true on success

// File access custom callbacks
// WARNING: Callbacks setup is intended for advanced users
RLAPI void RLSetLoadFileDataCallback(RLLoadFileDataCallback callback);  // Set custom file binary data loader
RLAPI void RLSetSaveFileDataCallback(RLSaveFileDataCallback callback);  // Set custom file binary data saver
RLAPI void RLSetLoadFileTextCallback(RLLoadFileTextCallback callback);  // Set custom file text data loader
RLAPI void RLSetSaveFileTextCallback(RLSaveFileTextCallback callback);  // Set custom file text data saver

RLAPI int RLFileRename(const char *fileName, const char *fileRename); // Rename file (if exists)
RLAPI int RLFileRemove(const char *fileName);                         // Remove file (if exists)
RLAPI int RLFileCopy(const char *srcPath, const char *dstPath);       // Copy file from one path to another, dstPath created if it doesn't exist
RLAPI int RLFileMove(const char *srcPath, const char *dstPath);       // Move file from one directory to another, dstPath created if it doesn't exist
RLAPI int RLFileTextReplace(const char *fileName, const char *search, const char *replacement); // Replace text in an existing file
RLAPI int RLFileTextFindIndex(const char *fileName, const char *search); // Find text in existing file
RLAPI bool RLFileExists(const char *fileName);                        // Check if file exists
RLAPI bool RLDirectoryExists(const char *dirPath);                    // Check if a directory path exists
RLAPI bool RLIsFileExtension(const char *fileName, const char *ext);  // Check file extension (recommended include point: .png, .wav)
RLAPI int RLGetFileLength(const char *fileName);                      // Get file length in bytes (NOTE: GetFileSize() conflicts with windows.h)
RLAPI long RLGetFileModTime(const char *fileName);                    // Get file modification time (last write time)
RLAPI const char *RLGetFileExtension(const char *fileName);           // Get pointer to extension for a filename string (includes dot: '.png')
RLAPI const char *RLGetFileName(const char *filePath);                // Get pointer to filename for a path string
RLAPI const char *RLGetFileNameWithoutExt(const char *filePath);      // Get filename string without extension (uses static string)
RLAPI const char *RLGetDirectoryPath(const char *filePath);           // Get full path for a given fileName with path (uses static string)
RLAPI const char *RLGetPrevDirectoryPath(const char *dirPath);        // Get previous directory path for a given path (uses static string)
RLAPI const char *RLGetWorkingDirectory(void);                        // Get current working directory (uses static string)
RLAPI const char *RLGetApplicationDirectory(void);                    // Get the directory of the running application (uses static string)
RLAPI int RLMakeDirectory(const char *dirPath);                       // Create directories (including full path requested), returns 0 on success
RLAPI bool RLChangeDirectory(const char *dirPath);                    // Change working directory, return true on success
RLAPI bool RLIsPathFile(const char *path);                            // Check if a given path is a file or a directory
RLAPI bool RLIsFileNameValid(const char *fileName);                   // Check if fileName is valid for the platform/OS
RLAPI RLFilePathList RLLoadDirectoryFiles(const char *dirPath);         // Load directory filepaths
RLAPI RLFilePathList RLLoadDirectoryFilesEx(const char *basePath, const char *filter, bool scanSubdirs); // Load directory filepaths with extension filtering and recursive directory scan. Use 'DIR' in the filter string to include directories in the result
RLAPI void RLUnloadDirectoryFiles(RLFilePathList files);                // Unload filepaths
RLAPI bool RLIsFileDropped(void);                                     // Check if a file has been dropped into window
RLAPI RLFilePathList RLLoadDroppedFiles(void);                          // Load dropped filepaths
RLAPI void RLUnloadDroppedFiles(RLFilePathList files);                  // Unload dropped filepaths
RLAPI unsigned int RLGetDirectoryFileCount(const char *dirPath);      // Get the file count in a directory
RLAPI unsigned int RLGetDirectoryFileCountEx(const char *basePath, const char *filter, bool scanSubdirs);// Get the file count in a directory with extension filtering and recursive directory scan. Use 'DIR' in the filter string to include directories in the result

// Compression/Encoding functionality
RLAPI unsigned char *RLCompressData(const unsigned char *data, int dataSize, int *compDataSize);        // Compress data (DEFLATE algorithm), memory must be MemFree()
RLAPI unsigned char *RLDecompressData(const unsigned char *compData, int compDataSize, int *dataSize);  // Decompress data (DEFLATE algorithm), memory must be MemFree()
RLAPI char *RLEncodeDataBase64(const unsigned char *data, int dataSize, int *outputSize);               // Encode data to Base64 string (includes NULL terminator), memory must be MemFree()
RLAPI unsigned char *RLDecodeDataBase64(const char *text, int *outputSize);                             // Decode Base64 string (expected NULL terminated), memory must be MemFree()
RLAPI unsigned int RLComputeCRC32(unsigned char *data, int dataSize);       // Compute CRC32 hash code
RLAPI unsigned int *RLComputeMD5(unsigned char *data, int dataSize);        // Compute MD5 hash code, returns static int[4] (16 bytes)
RLAPI unsigned int *RLComputeSHA1(unsigned char *data, int dataSize);       // Compute SHA1 hash code, returns static int[5] (20 bytes)
RLAPI unsigned int *RLComputeSHA256(unsigned char *data, int dataSize);     // Compute SHA256 hash code, returns static int[8] (32 bytes)

// Automation events functionality
RLAPI RLAutomationEventList RLLoadAutomationEventList(const char *fileName); // Load automation events list from file, NULL for empty list, capacity = MAX_AUTOMATION_EVENTS
RLAPI void RLUnloadAutomationEventList(RLAutomationEventList list);   // Unload automation events list from file
RLAPI bool RLExportAutomationEventList(RLAutomationEventList list, const char *fileName); // Export automation events list as text file
RLAPI void RLSetAutomationEventList(RLAutomationEventList *list);     // Set automation event list to record to
RLAPI void RLSetAutomationEventBaseFrame(int frame);                // Set automation event internal base frame to start recording
RLAPI void RLStartAutomationEventRecording(void);                   // Start recording automation events (AutomationEventList must be set)
RLAPI void RLStopAutomationEventRecording(void);                    // Stop recording automation events
RLAPI void RLPlayAutomationEvent(RLAutomationEvent event);            // Play a recorded automation event

//------------------------------------------------------------------------------------
// Input Handling Functions (Module: core)
//------------------------------------------------------------------------------------

// Input-related functions: keyboard
RLAPI bool RLIsKeyPressed(int key);                             // Check if a key has been pressed once
RLAPI bool RLIsKeyPressedRepeat(int key);                       // Check if a key has been pressed again
RLAPI bool RLIsKeyDown(int key);                                // Check if a key is being pressed
RLAPI bool RLIsKeyReleased(int key);                            // Check if a key has been released once
RLAPI bool RLIsKeyUp(int key);                                  // Check if a key is NOT being pressed
RLAPI int RLGetKeyPressed(void);                                // Get key pressed (keycode), call it multiple times for keys queued, returns 0 when the queue is empty
RLAPI int RLGetCharPressed(void);                               // Get char pressed (unicode), call it multiple times for chars queued, returns 0 when the queue is empty
RLAPI const char *RLGetKeyName(int key);                        // Get name of a QWERTY key on the current keyboard layout (eg returns string 'q' for KEY_A on an AZERTY keyboard)
RLAPI void RLSetExitKey(int key);                               // Set a custom key to exit program (default is ESC)

// Input-related functions: gamepads
RLAPI bool RLIsGamepadAvailable(int gamepad);                   // Check if a gamepad is available
RLAPI const char *RLGetGamepadName(int gamepad);                // Get gamepad internal name id
RLAPI bool RLIsGamepadButtonPressed(int gamepad, int button);   // Check if a gamepad button has been pressed once
RLAPI bool RLIsGamepadButtonDown(int gamepad, int button);      // Check if a gamepad button is being pressed
RLAPI bool RLIsGamepadButtonReleased(int gamepad, int button);  // Check if a gamepad button has been released once
RLAPI bool RLIsGamepadButtonUp(int gamepad, int button);        // Check if a gamepad button is NOT being pressed
RLAPI int RLGetGamepadButtonPressed(void);                      // Get the last gamepad button pressed
RLAPI int RLGetGamepadAxisCount(int gamepad);                   // Get axis count for a gamepad
RLAPI float RLGetGamepadAxisMovement(int gamepad, int axis);    // Get movement value for a gamepad axis
RLAPI int RLSetGamepadMappings(const char *mappings);           // Set internal gamepad mappings (SDL_GameControllerDB)
RLAPI void RLSetGamepadVibration(int gamepad, float leftMotor, float rightMotor, float duration); // Set gamepad vibration for both motors (duration in seconds)

// Input-related functions: mouse
RLAPI bool RLIsMouseButtonPressed(int button);                  // Check if a mouse button has been pressed once
RLAPI bool RLIsMouseButtonDown(int button);                     // Check if a mouse button is being pressed
RLAPI bool RLIsMouseButtonReleased(int button);                 // Check if a mouse button has been released once
RLAPI bool RLIsMouseButtonUp(int button);                       // Check if a mouse button is NOT being pressed
RLAPI int RLGetMouseX(void);                                    // Get mouse position X
RLAPI int RLGetMouseY(void);                                    // Get mouse position Y
RLAPI RLVector2 RLGetMousePosition(void);                         // Get mouse position XY
RLAPI RLVector2 RLGetMouseDelta(void);                            // Get mouse delta between frames
RLAPI void RLSetMousePosition(int x, int y);                    // Set mouse position XY
RLAPI void RLSetMouseOffset(int offsetX, int offsetY);          // Set mouse offset
RLAPI void RLSetMouseScale(float scaleX, float scaleY);         // Set mouse scaling
RLAPI float RLGetMouseWheelMove(void);                          // Get mouse wheel movement for X or Y, whichever is larger
RLAPI RLVector2 RLGetMouseWheelMoveV(void);                       // Get mouse wheel movement for both X and Y
RLAPI void RLSetMouseCursor(int cursor);                        // Set mouse cursor

// Input-related functions: touch
RLAPI int RLGetTouchX(void);                                    // Get touch position X for touch point 0 (relative to screen size)
RLAPI int RLGetTouchY(void);                                    // Get touch position Y for touch point 0 (relative to screen size)
RLAPI RLVector2 RLGetTouchPosition(int index);                    // Get touch position XY for a touch point index (relative to screen size)
RLAPI int RLGetTouchPointId(int index);                         // Get touch point identifier for given index
RLAPI int RLGetTouchPointCount(void);                           // Get number of touch points

//------------------------------------------------------------------------------------
// Gestures and Touch Handling Functions (Module: rgestures)
//------------------------------------------------------------------------------------
RLAPI void RLSetGesturesEnabled(unsigned int flags);            // Enable a set of gestures using flags
RLAPI bool RLIsGestureDetected(unsigned int gesture);           // Check if a gesture have been detected
RLAPI int RLGetGestureDetected(void);                           // Get latest detected gesture
RLAPI float RLGetGestureHoldDuration(void);                     // Get gesture hold time in seconds
RLAPI RLVector2 RLGetGestureDragVector(void);                     // Get gesture drag vector
RLAPI float RLGetGestureDragAngle(void);                        // Get gesture drag angle
RLAPI RLVector2 RLGetGesturePinchVector(void);                    // Get gesture pinch delta
RLAPI float RLGetGesturePinchAngle(void);                       // Get gesture pinch angle

//------------------------------------------------------------------------------------
// Camera System Functions (Module: rcamera)
//------------------------------------------------------------------------------------
RLAPI void RLUpdateCamera(RLCamera *camera, int mode);            // Update camera position for selected mode
RLAPI void RLUpdateCameraPro(RLCamera *camera, RLVector3 movement, RLVector3 rotation, float zoom); // Update camera movement/rotation

//------------------------------------------------------------------------------------
// Basic Shapes Drawing Functions (Module: shapes)
//------------------------------------------------------------------------------------
// Set texture and rectangle to be used on shapes drawing
// NOTE: It can be useful when using basic shapes and one single font,
// defining a font char white rectangle would allow drawing everything in a single draw call
RLAPI void RLSetShapesTexture(RLTexture2D texture, RLRectangle source); // Set texture and rectangle to be used on shapes drawing
RLAPI RLTexture2D RLGetShapesTexture(void);                 // Get texture that is used for shapes drawing
RLAPI RLRectangle RLGetShapesTextureRectangle(void);        // Get texture source rectangle that is used for shapes drawing

// Basic shapes drawing functions
RLAPI void RLDrawPixel(int posX, int posY, RLColor color);                                                   // Draw a pixel using geometry [Can be slow, use with care]
RLAPI void RLDrawPixelV(RLVector2 position, RLColor color);                                                    // Draw a pixel using geometry (Vector version) [Can be slow, use with care]
RLAPI void RLDrawLine(int startPosX, int startPosY, int endPosX, int endPosY, RLColor color);                // Draw a line
RLAPI void RLDrawLineV(RLVector2 startPos, RLVector2 endPos, RLColor color);                                     // Draw a line (using gl lines)
RLAPI void RLDrawLineEx(RLVector2 startPos, RLVector2 endPos, float thick, RLColor color);                       // Draw a line (using triangles/quads)
RLAPI void RLDrawLineStrip(const RLVector2 *points, int pointCount, RLColor color);                            // Draw lines sequence (using gl lines)
RLAPI void RLDrawLineBezier(RLVector2 startPos, RLVector2 endPos, float thick, RLColor color);                   // Draw line segment cubic-bezier in-out interpolation
RLAPI void RLDrawLineDashed(RLVector2 startPos, RLVector2 endPos, int dashSize, int spaceSize, RLColor color);   // Draw a dashed line
RLAPI void RLDrawCircle(int centerX, int centerY, float radius, RLColor color);                              // Draw a color-filled circle
RLAPI void RLDrawCircleSector(RLVector2 center, float radius, float startAngle, float endAngle, int segments, RLColor color);      // Draw a piece of a circle
RLAPI void RLDrawCircleSectorLines(RLVector2 center, float radius, float startAngle, float endAngle, int segments, RLColor color); // Draw circle sector outline
RLAPI void RLDrawCircleGradient(int centerX, int centerY, float radius, RLColor inner, RLColor outer);         // Draw a gradient-filled circle
RLAPI void RLDrawCircleV(RLVector2 center, float radius, RLColor color);                                       // Draw a color-filled circle (Vector version)
RLAPI void RLDrawCircleLines(int centerX, int centerY, float radius, RLColor color);                         // Draw circle outline
RLAPI void RLDrawCircleLinesV(RLVector2 center, float radius, RLColor color);                                  // Draw circle outline (Vector version)
RLAPI void RLDrawEllipse(int centerX, int centerY, float radiusH, float radiusV, RLColor color);             // Draw ellipse
RLAPI void RLDrawEllipseV(RLVector2 center, float radiusH, float radiusV, RLColor color);                      // Draw ellipse (Vector version)
RLAPI void RLDrawEllipseLines(int centerX, int centerY, float radiusH, float radiusV, RLColor color);        // Draw ellipse outline
RLAPI void RLDrawEllipseLinesV(RLVector2 center, float radiusH, float radiusV, RLColor color);                 // Draw ellipse outline (Vector version)
RLAPI void RLDrawRing(RLVector2 center, float innerRadius, float outerRadius, float startAngle, float endAngle, int segments, RLColor color); // Draw ring
RLAPI void RLDrawRingLines(RLVector2 center, float innerRadius, float outerRadius, float startAngle, float endAngle, int segments, RLColor color);    // Draw ring outline
RLAPI void RLDrawRectangle(int posX, int posY, int width, int height, RLColor color);                        // Draw a color-filled rectangle
RLAPI void RLDrawRectangleV(RLVector2 position, RLVector2 size, RLColor color);                                  // Draw a color-filled rectangle (Vector version)
RLAPI void RLDrawRectangleRec(RLRectangle rec, RLColor color);                                                 // Draw a color-filled rectangle
RLAPI void RLDrawRectanglePro(RLRectangle rec, RLVector2 origin, float rotation, RLColor color);                 // Draw a color-filled rectangle with pro parameters
RLAPI void RLDrawRectangleGradientV(int posX, int posY, int width, int height, RLColor top, RLColor bottom);   // Draw a vertical-gradient-filled rectangle
RLAPI void RLDrawRectangleGradientH(int posX, int posY, int width, int height, RLColor left, RLColor right);   // Draw a horizontal-gradient-filled rectangle
RLAPI void RLDrawRectangleGradientEx(RLRectangle rec, RLColor topLeft, RLColor bottomLeft, RLColor bottomRight, RLColor topRight); // Draw a gradient-filled rectangle with custom vertex colors
RLAPI void RLDrawRectangleLines(int posX, int posY, int width, int height, RLColor color);                   // Draw rectangle outline
RLAPI void RLDrawRectangleLinesEx(RLRectangle rec, float lineThick, RLColor color);                            // Draw rectangle outline with extended parameters
RLAPI void RLDrawRectangleRounded(RLRectangle rec, float roundness, int segments, RLColor color);              // Draw rectangle with rounded edges
RLAPI void RLDrawRectangleRoundedLines(RLRectangle rec, float roundness, int segments, RLColor color);         // Draw rectangle lines with rounded edges
RLAPI void RLDrawRectangleRoundedLinesEx(RLRectangle rec, float roundness, int segments, float lineThick, RLColor color); // Draw rectangle with rounded edges outline
RLAPI void RLDrawTriangle(RLVector2 v1, RLVector2 v2, RLVector2 v3, RLColor color);                                // Draw a color-filled triangle (vertex in counter-clockwise order!)
RLAPI void RLDrawTriangleLines(RLVector2 v1, RLVector2 v2, RLVector2 v3, RLColor color);                           // Draw triangle outline (vertex in counter-clockwise order!)
RLAPI void RLDrawTriangleFan(const RLVector2 *points, int pointCount, RLColor color);                          // Draw a triangle fan defined by points (first vertex is the center)
RLAPI void RLDrawTriangleStrip(const RLVector2 *points, int pointCount, RLColor color);                        // Draw a triangle strip defined by points
RLAPI void RLDrawPoly(RLVector2 center, int sides, float radius, float rotation, RLColor color);               // Draw a regular polygon (Vector version)
RLAPI void RLDrawPolyLines(RLVector2 center, int sides, float radius, float rotation, RLColor color);          // Draw a polygon outline of n sides
RLAPI void RLDrawPolyLinesEx(RLVector2 center, int sides, float radius, float rotation, float lineThick, RLColor color); // Draw a polygon outline of n sides with extended parameters

// Splines drawing functions
RLAPI void RLDrawSplineLinear(const RLVector2 *points, int pointCount, float thick, RLColor color);            // Draw spline: Linear, minimum 2 points
RLAPI void RLDrawSplineBasis(const RLVector2 *points, int pointCount, float thick, RLColor color);             // Draw spline: B-Spline, minimum 4 points
RLAPI void RLDrawSplineCatmullRom(const RLVector2 *points, int pointCount, float thick, RLColor color);        // Draw spline: Catmull-Rom, minimum 4 points
RLAPI void RLDrawSplineBezierQuadratic(const RLVector2 *points, int pointCount, float thick, RLColor color);   // Draw spline: Quadratic Bezier, minimum 3 points (1 control point): [p1, c2, p3, c4...]
RLAPI void RLDrawSplineBezierCubic(const RLVector2 *points, int pointCount, float thick, RLColor color);       // Draw spline: Cubic Bezier, minimum 4 points (2 control points): [p1, c2, c3, p4, c5, c6...]
RLAPI void RLDrawSplineSegmentLinear(RLVector2 p1, RLVector2 p2, float thick, RLColor color);                    // Draw spline segment: Linear, 2 points
RLAPI void RLDrawSplineSegmentBasis(RLVector2 p1, RLVector2 p2, RLVector2 p3, RLVector2 p4, float thick, RLColor color); // Draw spline segment: B-Spline, 4 points
RLAPI void RLDrawSplineSegmentCatmullRom(RLVector2 p1, RLVector2 p2, RLVector2 p3, RLVector2 p4, float thick, RLColor color); // Draw spline segment: Catmull-Rom, 4 points
RLAPI void RLDrawSplineSegmentBezierQuadratic(RLVector2 p1, RLVector2 c2, RLVector2 p3, float thick, RLColor color); // Draw spline segment: Quadratic Bezier, 2 points, 1 control point
RLAPI void RLDrawSplineSegmentBezierCubic(RLVector2 p1, RLVector2 c2, RLVector2 c3, RLVector2 p4, float thick, RLColor color); // Draw spline segment: Cubic Bezier, 2 points, 2 control points

// Spline segment point evaluation functions, for a given t [0.0f .. 1.0f]
RLAPI RLVector2 RLGetSplinePointLinear(RLVector2 startPos, RLVector2 endPos, float t);                           // Get (evaluate) spline point: Linear
RLAPI RLVector2 RLGetSplinePointBasis(RLVector2 p1, RLVector2 p2, RLVector2 p3, RLVector2 p4, float t);              // Get (evaluate) spline point: B-Spline
RLAPI RLVector2 RLGetSplinePointCatmullRom(RLVector2 p1, RLVector2 p2, RLVector2 p3, RLVector2 p4, float t);         // Get (evaluate) spline point: Catmull-Rom
RLAPI RLVector2 RLGetSplinePointBezierQuad(RLVector2 p1, RLVector2 c2, RLVector2 p3, float t);                     // Get (evaluate) spline point: Quadratic Bezier
RLAPI RLVector2 RLGetSplinePointBezierCubic(RLVector2 p1, RLVector2 c2, RLVector2 c3, RLVector2 p4, float t);        // Get (evaluate) spline point: Cubic Bezier

// Basic shapes collision detection functions
RLAPI bool RLCheckCollisionRecs(RLRectangle rec1, RLRectangle rec2);                                           // Check collision between two rectangles
RLAPI bool RLCheckCollisionCircles(RLVector2 center1, float radius1, RLVector2 center2, float radius2);        // Check collision between two circles
RLAPI bool RLCheckCollisionCircleRec(RLVector2 center, float radius, RLRectangle rec);                         // Check collision between circle and rectangle
RLAPI bool RLCheckCollisionCircleLine(RLVector2 center, float radius, RLVector2 p1, RLVector2 p2);               // Check if circle collides with a line created betweeen two points [p1] and [p2]
RLAPI bool RLCheckCollisionPointRec(RLVector2 point, RLRectangle rec);                                         // Check if point is inside rectangle
RLAPI bool RLCheckCollisionPointCircle(RLVector2 point, RLVector2 center, float radius);                       // Check if point is inside circle
RLAPI bool RLCheckCollisionPointTriangle(RLVector2 point, RLVector2 p1, RLVector2 p2, RLVector2 p3);               // Check if point is inside a triangle
RLAPI bool RLCheckCollisionPointLine(RLVector2 point, RLVector2 p1, RLVector2 p2, int threshold);                // Check if point belongs to line created between two points [p1] and [p2] with defined margin in pixels [threshold]
RLAPI bool RLCheckCollisionPointPoly(RLVector2 point, const RLVector2 *points, int pointCount);                // Check if point is within a polygon described by array of vertices
RLAPI bool RLCheckCollisionLines(RLVector2 startPos1, RLVector2 endPos1, RLVector2 startPos2, RLVector2 endPos2, RLVector2 *collisionPoint); // Check the collision between two lines defined by two points each, returns collision point by reference
RLAPI RLRectangle RLGetCollisionRec(RLRectangle rec1, RLRectangle rec2);                                         // Get collision rectangle for two rectangles collision

//------------------------------------------------------------------------------------
// Texture Loading and Drawing Functions (Module: textures)
//------------------------------------------------------------------------------------

// Image loading functions
// NOTE: These functions do not require GPU access
RLAPI RLImage RLLoadImage(const char *fileName);                                                             // Load image from file into CPU memory (RAM)
RLAPI RLImage RLLoadImageRaw(const char *fileName, int width, int height, int format, int headerSize);       // Load image from RAW file data
RLAPI RLImage RLLoadImageAnim(const char *fileName, int *frames);                                            // Load image sequence from file (frames appended to image.data)
RLAPI RLImage RLLoadImageAnimFromMemory(const char *fileType, const unsigned char *fileData, int dataSize, int *frames); // Load image sequence from memory buffer
RLAPI RLImage RLLoadImageFromMemory(const char *fileType, const unsigned char *fileData, int dataSize);      // Load image from memory buffer, fileType refers to extension: i.e. '.png'
RLAPI RLImage RLLoadImageFromTexture(RLTexture2D texture);                                                     // Load image from GPU texture data
RLAPI RLImage RLLoadImageFromScreen(void);                                                                   // Load image from screen buffer and (screenshot)
RLAPI bool RLIsImageValid(RLImage image);                                                                    // Check if an image is valid (data and parameters)
RLAPI void RLUnloadImage(RLImage image);                                                                     // Unload image from CPU memory (RAM)
RLAPI bool RLExportImage(RLImage image, const char *fileName);                                               // Export image data to file, returns true on success
RLAPI unsigned char *RLExportImageToMemory(RLImage image, const char *fileType, int *fileSize);              // Export image to memory buffer
RLAPI bool RLExportImageAsCode(RLImage image, const char *fileName);                                         // Export image as code file defining an array of bytes, returns true on success

// Image generation functions
RLAPI RLImage RLGenImageColor(int width, int height, RLColor color);                                           // Generate image: plain color
RLAPI RLImage RLGenImageGradientLinear(int width, int height, int direction, RLColor start, RLColor end);        // Generate image: linear gradient, direction in degrees [0..360], 0=Vertical gradient
RLAPI RLImage RLGenImageGradientRadial(int width, int height, float density, RLColor inner, RLColor outer);      // Generate image: radial gradient
RLAPI RLImage RLGenImageGradientSquare(int width, int height, float density, RLColor inner, RLColor outer);      // Generate image: square gradient
RLAPI RLImage RLGenImageChecked(int width, int height, int checksX, int checksY, RLColor col1, RLColor col2);    // Generate image: checked
RLAPI RLImage RLGenImageWhiteNoise(int width, int height, float factor);                                     // Generate image: white noise
RLAPI RLImage RLGenImagePerlinNoise(int width, int height, int offsetX, int offsetY, float scale);           // Generate image: perlin noise
RLAPI RLImage RLGenImageCellular(int width, int height, int tileSize);                                       // Generate image: cellular algorithm, bigger tileSize means bigger cells
RLAPI RLImage RLGenImageText(int width, int height, const char *text);                                       // Generate image: grayscale image from text data

// Image manipulation functions
RLAPI RLImage RLImageCopy(RLImage image);                                                                      // Create an image duplicate (useful for transformations)
RLAPI RLImage RLImageFromImage(RLImage image, RLRectangle rec);                                                  // Create an image from another image piece
RLAPI RLImage RLImageFromChannel(RLImage image, int selectedChannel);                                          // Create an image from a selected channel of another image (GRAYSCALE)
RLAPI RLImage RLImageText(const char *text, int fontSize, RLColor color);                                      // Create an image from text (default font)
RLAPI RLImage RLImageTextEx(RLFont font, const char *text, float fontSize, float spacing, RLColor tint);         // Create an image from text (custom sprite font)
RLAPI void RLImageFormat(RLImage *image, int newFormat);                                                     // Convert image data to desired format
RLAPI void RLImageToPOT(RLImage *image, RLColor fill);                                                         // Convert image to POT (power-of-two)
RLAPI void RLImageCrop(RLImage *image, RLRectangle crop);                                                      // Crop an image to a defined rectangle
RLAPI void RLImageAlphaCrop(RLImage *image, float threshold);                                                // Crop image depending on alpha value
RLAPI void RLImageAlphaClear(RLImage *image, RLColor color, float threshold);                                  // Clear alpha channel to desired color
RLAPI void RLImageAlphaMask(RLImage *image, RLImage alphaMask);                                                // Apply alpha mask to image
RLAPI void RLImageAlphaPremultiply(RLImage *image);                                                          // Premultiply alpha channel
RLAPI void RLImageBlurGaussian(RLImage *image, int blurSize);                                                // Apply Gaussian blur using a box blur approximation
RLAPI void RLImageKernelConvolution(RLImage *image, const float *kernel, int kernelSize);                    // Apply custom square convolution kernel to image
RLAPI void RLImageResize(RLImage *image, int newWidth, int newHeight);                                       // Resize image (Bicubic scaling algorithm)
RLAPI void RLImageResizeNN(RLImage *image, int newWidth, int newHeight);                                     // Resize image (Nearest-Neighbor scaling algorithm)
RLAPI void RLImageResizeCanvas(RLImage *image, int newWidth, int newHeight, int offsetX, int offsetY, RLColor fill); // Resize canvas and fill with color
RLAPI void RLImageMipmaps(RLImage *image);                                                                   // Compute all mipmap levels for a provided image
RLAPI void RLImageDither(RLImage *image, int rBpp, int gBpp, int bBpp, int aBpp);                            // Dither image data to 16bpp or lower (Floyd-Steinberg dithering)
RLAPI void RLImageFlipVertical(RLImage *image);                                                              // Flip image vertically
RLAPI void RLImageFlipHorizontal(RLImage *image);                                                            // Flip image horizontally
RLAPI void RLImageRotate(RLImage *image, int degrees);                                                       // Rotate image by input angle in degrees (-359 to 359)
RLAPI void RLImageRotateCW(RLImage *image);                                                                  // Rotate image clockwise 90deg
RLAPI void RLImageRotateCCW(RLImage *image);                                                                 // Rotate image counter-clockwise 90deg
RLAPI void RLImageColorTint(RLImage *image, RLColor color);                                                    // Modify image color: tint
RLAPI void RLImageColorInvert(RLImage *image);                                                               // Modify image color: invert
RLAPI void RLImageColorGrayscale(RLImage *image);                                                            // Modify image color: grayscale
RLAPI void RLImageColorContrast(RLImage *image, float contrast);                                             // Modify image color: contrast (-100 to 100)
RLAPI void RLImageColorBrightness(RLImage *image, int brightness);                                           // Modify image color: brightness (-255 to 255)
RLAPI void RLImageColorReplace(RLImage *image, RLColor color, RLColor replace);                                  // Modify image color: replace color
RLAPI RLColor *RLLoadImageColors(RLImage image);                                                               // Load color data from image as a Color array (RGBA - 32bit)
RLAPI RLColor *RLLoadImagePalette(RLImage image, int maxPaletteSize, int *colorCount);                         // Load colors palette from image as a Color array (RGBA - 32bit)
RLAPI void RLUnloadImageColors(RLColor *colors);                                                             // Unload color data loaded with LoadImageColors()
RLAPI void RLUnloadImagePalette(RLColor *colors);                                                            // Unload colors palette loaded with LoadImagePalette()
RLAPI RLRectangle RLGetImageAlphaBorder(RLImage image, float threshold);                                       // Get image alpha border rectangle
RLAPI RLColor RLGetImageColor(RLImage image, int x, int y);                                                    // Get image pixel color at (x, y) position

// Image drawing functions
// NOTE: Image software-rendering functions (CPU)
RLAPI void RLImageClearBackground(RLImage *dst, RLColor color);                                                // Clear image background with given color
RLAPI void RLImageDrawPixel(RLImage *dst, int posX, int posY, RLColor color);                                  // Draw pixel within an image
RLAPI void RLImageDrawPixelV(RLImage *dst, RLVector2 position, RLColor color);                                   // Draw pixel within an image (Vector version)
RLAPI void RLImageDrawLine(RLImage *dst, int startPosX, int startPosY, int endPosX, int endPosY, RLColor color); // Draw line within an image
RLAPI void RLImageDrawLineV(RLImage *dst, RLVector2 start, RLVector2 end, RLColor color);                          // Draw line within an image (Vector version)
RLAPI void RLImageDrawLineEx(RLImage *dst, RLVector2 start, RLVector2 end, int thick, RLColor color);              // Draw a line defining thickness within an image
RLAPI void RLImageDrawCircle(RLImage *dst, int centerX, int centerY, int radius, RLColor color);               // Draw a filled circle within an image
RLAPI void RLImageDrawCircleV(RLImage *dst, RLVector2 center, int radius, RLColor color);                        // Draw a filled circle within an image (Vector version)
RLAPI void RLImageDrawCircleLines(RLImage *dst, int centerX, int centerY, int radius, RLColor color);          // Draw circle outline within an image
RLAPI void RLImageDrawCircleLinesV(RLImage *dst, RLVector2 center, int radius, RLColor color);                   // Draw circle outline within an image (Vector version)
RLAPI void RLImageDrawRectangle(RLImage *dst, int posX, int posY, int width, int height, RLColor color);       // Draw rectangle within an image
RLAPI void RLImageDrawRectangleV(RLImage *dst, RLVector2 position, RLVector2 size, RLColor color);                 // Draw rectangle within an image (Vector version)
RLAPI void RLImageDrawRectangleRec(RLImage *dst, RLRectangle rec, RLColor color);                                // Draw rectangle within an image
RLAPI void RLImageDrawRectangleLines(RLImage *dst, RLRectangle rec, int thick, RLColor color);                   // Draw rectangle lines within an image
RLAPI void RLImageDrawTriangle(RLImage *dst, RLVector2 v1, RLVector2 v2, RLVector2 v3, RLColor color);               // Draw triangle within an image
RLAPI void RLImageDrawTriangleEx(RLImage *dst, RLVector2 v1, RLVector2 v2, RLVector2 v3, RLColor c1, RLColor c2, RLColor c3); // Draw triangle with interpolated colors within an image
RLAPI void RLImageDrawTriangleLines(RLImage *dst, RLVector2 v1, RLVector2 v2, RLVector2 v3, RLColor color);          // Draw triangle outline within an image
RLAPI void RLImageDrawTriangleFan(RLImage *dst, const RLVector2 *points, int pointCount, RLColor color);               // Draw a triangle fan defined by points within an image (first vertex is the center)
RLAPI void RLImageDrawTriangleStrip(RLImage *dst, const RLVector2 *points, int pointCount, RLColor color);             // Draw a triangle strip defined by points within an image
RLAPI void RLImageDraw(RLImage *dst, RLImage src, RLRectangle srcRec, RLRectangle dstRec, RLColor tint);             // Draw a source image within a destination image (tint applied to source)
RLAPI void RLImageDrawText(RLImage *dst, const char *text, int posX, int posY, int fontSize, RLColor color);   // Draw text (using default font) within an image (destination)
RLAPI void RLImageDrawTextEx(RLImage *dst, RLFont font, const char *text, RLVector2 position, float fontSize, float spacing, RLColor tint); // Draw text (custom sprite font) within an image (destination)

// Texture loading functions
// NOTE: These functions require GPU access
RLAPI RLTexture2D RLLoadTexture(const char *fileName);                                                       // Load texture from file into GPU memory (VRAM)
RLAPI RLTexture2D RLLoadTextureFromImage(RLImage image);                                                       // Load texture from image data
RLAPI RLTextureCubemap RLLoadTextureCubemap(RLImage image, int layout);                                        // Load cubemap from image, multiple image cubemap layouts supported
RLAPI RLRenderTexture2D RLLoadRenderTexture(int width, int height);                                          // Load texture for rendering (framebuffer)
RLAPI bool RLIsTextureValid(RLTexture2D texture);                                                            // Check if a texture is valid (loaded in GPU)
RLAPI void RLUnloadTexture(RLTexture2D texture);                                                             // Unload texture from GPU memory (VRAM)
RLAPI bool RLIsRenderTextureValid(RLRenderTexture2D target);                                                 // Check if a render texture is valid (loaded in GPU)
RLAPI void RLUnloadRenderTexture(RLRenderTexture2D target);                                                  // Unload render texture from GPU memory (VRAM)
RLAPI void RLUpdateTexture(RLTexture2D texture, const void *pixels);                                         // Update GPU texture with new data (pixels should be able to fill texture)
RLAPI void RLUpdateTextureRec(RLTexture2D texture, RLRectangle rec, const void *pixels);                       // Update GPU texture rectangle with new data (pixels and rec should fit in texture)

// Texture configuration functions
RLAPI void RLGenTextureMipmaps(RLTexture2D *texture);                                                        // Generate GPU mipmaps for a texture
RLAPI void RLSetTextureFilter(RLTexture2D texture, int filter);                                              // Set texture scaling filter mode
RLAPI void RLSetTextureWrap(RLTexture2D texture, int wrap);                                                  // Set texture wrapping mode

// Texture drawing functions
RLAPI void RLDrawTexture(RLTexture2D texture, int posX, int posY, RLColor tint);                               // Draw a Texture2D
RLAPI void RLDrawTextureV(RLTexture2D texture, RLVector2 position, RLColor tint);                                // Draw a Texture2D with position defined as Vector2
RLAPI void RLDrawTextureEx(RLTexture2D texture, RLVector2 position, float rotation, float scale, RLColor tint);  // Draw a Texture2D with extended parameters
RLAPI void RLDrawTextureRec(RLTexture2D texture, RLRectangle source, RLVector2 position, RLColor tint);            // Draw a part of a texture defined by a rectangle
RLAPI void RLDrawTexturePro(RLTexture2D texture, RLRectangle source, RLRectangle dest, RLVector2 origin, float rotation, RLColor tint); // Draw a part of a texture defined by a rectangle with 'pro' parameters
RLAPI void RLDrawTextureNPatch(RLTexture2D texture, RLNPatchInfo nPatchInfo, RLRectangle dest, RLVector2 origin, float rotation, RLColor tint); // Draws a texture (or part of it) that stretches or shrinks nicely

// Color/pixel related functions
RLAPI bool RLColorIsEqual(RLColor col1, RLColor col2);                            // Check if two colors are equal
RLAPI RLColor RLFade(RLColor color, float alpha);                                 // Get color with alpha applied, alpha goes from 0.0f to 1.0f
RLAPI int RLColorToInt(RLColor color);                                          // Get hexadecimal value for a Color (0xRRGGBBAA)
RLAPI RLVector4 RLColorNormalize(RLColor color);                                  // Get Color normalized as float [0..1]
RLAPI RLColor RLColorFromNormalized(RLVector4 normalized);                        // Get Color from normalized values [0..1]
RLAPI RLVector3 RLColorToHSV(RLColor color);                                      // Get HSV values for a Color, hue [0..360], saturation/value [0..1]
RLAPI RLColor RLColorFromHSV(float hue, float saturation, float value);         // Get a Color from HSV values, hue [0..360], saturation/value [0..1]
RLAPI RLColor RLColorTint(RLColor color, RLColor tint);                             // Get color multiplied with another color
RLAPI RLColor RLColorBrightness(RLColor color, float factor);                     // Get color with brightness correction, brightness factor goes from -1.0f to 1.0f
RLAPI RLColor RLColorContrast(RLColor color, float contrast);                     // Get color with contrast correction, contrast values between -1.0f and 1.0f
RLAPI RLColor RLColorAlpha(RLColor color, float alpha);                           // Get color with alpha applied, alpha goes from 0.0f to 1.0f
RLAPI RLColor RLColorAlphaBlend(RLColor dst, RLColor src, RLColor tint);              // Get src alpha-blended into dst color with tint
RLAPI RLColor RLColorLerp(RLColor color1, RLColor color2, float factor);            // Get color lerp interpolation between two colors, factor [0.0f..1.0f]
RLAPI RLColor RLGetColor(unsigned int hexValue);                                // Get Color structure from hexadecimal value
RLAPI RLColor RLGetPixelColor(void *srcPtr, int format);                        // Get Color from a source pixel pointer of certain format
RLAPI void RLSetPixelColor(void *dstPtr, RLColor color, int format);            // Set color formatted into destination pixel pointer
RLAPI int RLGetPixelDataSize(int width, int height, int format);              // Get pixel data size in bytes for certain format

//------------------------------------------------------------------------------------
// Font Loading and Text Drawing Functions (Module: text)
//------------------------------------------------------------------------------------

// Font loading/unloading functions
RLAPI RLFont RLGetFontDefault(void);                                                            // Get the default Font
RLAPI RLFont RLLoadFont(const char *fileName);                                                  // Load font from file into GPU memory (VRAM)
RLAPI RLFont RLLoadFontEx(const char *fileName, int fontSize, const int *codepoints, int codepointCount); // Load font from file with extended parameters, use NULL for codepoints and 0 for codepointCount to load the default character set, font size is provided in pixels height
RLAPI RLFont RLLoadFontFromImage(RLImage image, RLColor key, int firstChar);                        // Load font from Image (XNA style)
RLAPI RLFont RLLoadFontFromMemory(const char *fileType, const unsigned char *fileData, int dataSize, int fontSize, const int *codepoints, int codepointCount); // Load font from memory buffer, fileType refers to extension: i.e. '.ttf'
RLAPI bool RLIsFontValid(RLFont font);                                                          // Check if a font is valid (font data loaded, WARNING: GPU texture not checked)
RLAPI RLGlyphInfo *RLLoadFontData(const unsigned char *fileData, int dataSize, int fontSize, const int *codepoints, int codepointCount, int type, int *glyphCount); // Load font data for further use
RLAPI RLImage RLGenImageFontAtlas(const RLGlyphInfo *glyphs, RLRectangle **glyphRecs, int glyphCount, int fontSize, int padding, int packMethod); // Generate image font atlas using chars info
RLAPI void RLUnloadFontData(RLGlyphInfo *glyphs, int glyphCount);                               // Unload font chars info data (RAM)
RLAPI void RLUnloadFont(RLFont font);                                                           // Unload font from GPU memory (VRAM)
RLAPI bool RLExportFontAsCode(RLFont font, const char *fileName);                               // Export font as code file, returns true on success

// Text drawing functions
RLAPI void RLDrawFPS(int posX, int posY);                                                     // Draw current FPS
RLAPI void RLDrawText(const char *text, int posX, int posY, int fontSize, RLColor color);       // Draw text (using default font)
RLAPI void RLDrawTextEx(RLFont font, const char *text, RLVector2 position, float fontSize, float spacing, RLColor tint); // Draw text using font and additional parameters
RLAPI void RLDrawTextPro(RLFont font, const char *text, RLVector2 position, RLVector2 origin, float rotation, float fontSize, float spacing, RLColor tint); // Draw text using Font and pro parameters (rotation)
RLAPI void RLDrawTextCodepoint(RLFont font, int codepoint, RLVector2 position, float fontSize, RLColor tint); // Draw one character (codepoint)
RLAPI void RLDrawTextCodepoints(RLFont font, const int *codepoints, int codepointCount, RLVector2 position, float fontSize, float spacing, RLColor tint); // Draw multiple character (codepoint)

// Text font info functions
RLAPI void RLSetTextLineSpacing(int spacing);                                                 // Set vertical line spacing when drawing with line-breaks
RLAPI int RLMeasureText(const char *text, int fontSize);                                      // Measure string width for default font
RLAPI RLVector2 RLMeasureTextEx(RLFont font, const char *text, float fontSize, float spacing);    // Measure string size for Font
RLAPI int RLGetGlyphIndex(RLFont font, int codepoint);                                          // Get glyph index position in font for a codepoint (unicode character), fallback to '?' if not found
RLAPI RLGlyphInfo RLGetGlyphInfo(RLFont font, int codepoint);                                     // Get glyph font info data for a codepoint (unicode character), fallback to '?' if not found
RLAPI RLRectangle RLGetGlyphAtlasRec(RLFont font, int codepoint);                                 // Get glyph rectangle in font atlas for a codepoint (unicode character), fallback to '?' if not found

// Text codepoints management functions (unicode characters)
RLAPI char *RLLoadUTF8(const int *codepoints, int length);                                    // Load UTF-8 text encoded from codepoints array
RLAPI void RLUnloadUTF8(char *text);                                                          // Unload UTF-8 text encoded from codepoints array
RLAPI int *RLLoadCodepoints(const char *text, int *count);                                    // Load all codepoints from a UTF-8 text string, codepoints count returned by parameter
RLAPI void RLUnloadCodepoints(int *codepoints);                                               // Unload codepoints data from memory
RLAPI int RLGetCodepointCount(const char *text);                                              // Get total number of codepoints in a UTF-8 encoded string
RLAPI int RLGetCodepoint(const char *text, int *codepointSize);                               // Get next codepoint in a UTF-8 encoded string, 0x3f('?') is returned on failure
RLAPI int RLGetCodepointNext(const char *text, int *codepointSize);                           // Get next codepoint in a UTF-8 encoded string, 0x3f('?') is returned on failure
RLAPI int RLGetCodepointPrevious(const char *text, int *codepointSize);                       // Get previous codepoint in a UTF-8 encoded string, 0x3f('?') is returned on failure
RLAPI const char *RLCodepointToUTF8(int codepoint, int *utf8Size);                            // Encode one codepoint into UTF-8 byte array (array length returned as parameter)

// Text strings management functions (no UTF-8 strings, only byte chars)
// WARNING 1: Most of these functions use internal static buffers[], it's recommended to store returned data on user-side for re-use
// WARNING 2: Some strings allocate memory internally for the returned strings, those strings must be free by user using MemFree()
RLAPI char **RLLoadTextLines(const char *text, int *count);                                   // Load text as separate lines ('\n')
RLAPI void RLUnloadTextLines(char **text, int lineCount);                                     // Unload text lines
RLAPI int RLTextCopy(char *dst, const char *src);                                             // Copy one string to another, returns bytes copied
RLAPI bool RLTextIsEqual(const char *text1, const char *text2);                               // Check if two text string are equal
RLAPI unsigned int RLTextLength(const char *text);                                            // Get text length, checks for '\0' ending
RLAPI const char *RLTextFormat(const char *text, ...);                                        // Text formatting with variables (sprintf() style)
RLAPI const char *RLTextSubtext(const char *text, int position, int length);                  // Get a piece of a text string
RLAPI const char *RLTextRemoveSpaces(const char *text);                                       // Remove text spaces, concat words
RLAPI char *RLGetTextBetween(const char *text, const char *begin, const char *end);           // Get text between two strings
RLAPI char *RLTextReplace(const char *text, const char *search, const char *replacement);     // Replace text string (WARNING: memory must be freed!)
RLAPI char *RLTextReplaceBetween(const char *text, const char *begin, const char *end, const char *replacement); // Replace text between two specific strings (WARNING: memory must be freed!)
RLAPI char *RLTextInsert(const char *text, const char *insert, int position);                 // Insert text in a position (WARNING: memory must be freed!)
RLAPI char *RLTextJoin(char **textList, int count, const char *delimiter);                    // Join text strings with delimiter
RLAPI char **RLTextSplit(const char *text, char delimiter, int *count);                       // Split text into multiple strings, using MAX_TEXTSPLIT_COUNT static strings
RLAPI void RLTextAppend(char *text, const char *append, int *position);                       // Append text at specific position and move cursor
RLAPI int RLTextFindIndex(const char *text, const char *search);                              // Find first text occurrence within a string, -1 if not found
RLAPI char *RLTextToUpper(const char *text);                                                  // Get upper case version of provided string
RLAPI char *RLTextToLower(const char *text);                                                  // Get lower case version of provided string
RLAPI char *RLTextToPascal(const char *text);                                                 // Get Pascal case notation version of provided string
RLAPI char *RLTextToSnake(const char *text);                                                  // Get Snake case notation version of provided string
RLAPI char *RLTextToCamel(const char *text);                                                  // Get Camel case notation version of provided string
RLAPI int RLTextToInteger(const char *text);                                                  // Get integer value from text
RLAPI float RLTextToFloat(const char *text);                                                  // Get float value from text

//------------------------------------------------------------------------------------
// Basic 3d Shapes Drawing Functions (Module: models)
//------------------------------------------------------------------------------------

// Basic geometric 3D shapes drawing functions
RLAPI void RLDrawLine3D(RLVector3 startPos, RLVector3 endPos, RLColor color);                                    // Draw a line in 3D world space
RLAPI void RLDrawPoint3D(RLVector3 position, RLColor color);                                                   // Draw a point in 3D space, actually a small line
RLAPI void RLDrawCircle3D(RLVector3 center, float radius, RLVector3 rotationAxis, float rotationAngle, RLColor color); // Draw a circle in 3D world space
RLAPI void RLDrawTriangle3D(RLVector3 v1, RLVector3 v2, RLVector3 v3, RLColor color);                              // Draw a color-filled triangle (vertex in counter-clockwise order!)
RLAPI void RLDrawTriangleStrip3D(const RLVector3 *points, int pointCount, RLColor color);                      // Draw a triangle strip defined by points
RLAPI void RLDrawCube(RLVector3 position, float width, float height, float length, RLColor color);             // Draw cube
RLAPI void RLDrawCubeV(RLVector3 position, RLVector3 size, RLColor color);                                       // Draw cube (Vector version)
RLAPI void RLDrawCubeWires(RLVector3 position, float width, float height, float length, RLColor color);        // Draw cube wires
RLAPI void RLDrawCubeWiresV(RLVector3 position, RLVector3 size, RLColor color);                                  // Draw cube wires (Vector version)
RLAPI void RLDrawSphere(RLVector3 centerPos, float radius, RLColor color);                                     // Draw sphere
RLAPI void RLDrawSphereEx(RLVector3 centerPos, float radius, int rings, int slices, RLColor color);            // Draw sphere with extended parameters
RLAPI void RLDrawSphereWires(RLVector3 centerPos, float radius, int rings, int slices, RLColor color);         // Draw sphere wires
RLAPI void RLDrawCylinder(RLVector3 position, float radiusTop, float radiusBottom, float height, int slices, RLColor color); // Draw a cylinder/cone
RLAPI void RLDrawCylinderEx(RLVector3 startPos, RLVector3 endPos, float startRadius, float endRadius, int sides, RLColor color); // Draw a cylinder with base at startPos and top at endPos
RLAPI void RLDrawCylinderWires(RLVector3 position, float radiusTop, float radiusBottom, float height, int slices, RLColor color); // Draw a cylinder/cone wires
RLAPI void RLDrawCylinderWiresEx(RLVector3 startPos, RLVector3 endPos, float startRadius, float endRadius, int sides, RLColor color); // Draw a cylinder wires with base at startPos and top at endPos
RLAPI void RLDrawCapsule(RLVector3 startPos, RLVector3 endPos, float radius, int slices, int rings, RLColor color); // Draw a capsule with the center of its sphere caps at startPos and endPos
RLAPI void RLDrawCapsuleWires(RLVector3 startPos, RLVector3 endPos, float radius, int slices, int rings, RLColor color); // Draw capsule wireframe with the center of its sphere caps at startPos and endPos
RLAPI void RLDrawPlane(RLVector3 centerPos, RLVector2 size, RLColor color);                                      // Draw a plane XZ
RLAPI void RLDrawRay(RLRay ray, RLColor color);                                                                // Draw a ray line
RLAPI void RLDrawGrid(int slices, float spacing);                                                          // Draw a grid (centered at (0, 0, 0))

//------------------------------------------------------------------------------------
// Model 3d Loading and Drawing Functions (Module: models)
//------------------------------------------------------------------------------------

// Model management functions
RLAPI RLModel RLLoadModel(const char *fileName);                                                // Load model from files (meshes and materials)
RLAPI RLModel RLLoadModelFromMesh(RLMesh mesh);                                                   // Load model from generated mesh (default material)
RLAPI bool RLIsModelValid(RLModel model);                                                       // Check if a model is valid (loaded in GPU, VAO/VBOs)
RLAPI void RLUnloadModel(RLModel model);                                                        // Unload model (including meshes) from memory (RAM and/or VRAM)
RLAPI RLBoundingBox RLGetModelBoundingBox(RLModel model);                                         // Compute model bounding box limits (considers all meshes)

// Model drawing functions
RLAPI void RLDrawModel(RLModel model, RLVector3 position, float scale, RLColor tint);               // Draw a model (with texture if set)
RLAPI void RLDrawModelEx(RLModel model, RLVector3 position, RLVector3 rotationAxis, float rotationAngle, RLVector3 scale, RLColor tint); // Draw a model with extended parameters
RLAPI void RLDrawModelWires(RLModel model, RLVector3 position, float scale, RLColor tint);          // Draw a model wires (with texture if set)
RLAPI void RLDrawModelWiresEx(RLModel model, RLVector3 position, RLVector3 rotationAxis, float rotationAngle, RLVector3 scale, RLColor tint); // Draw a model wires (with texture if set) with extended parameters
RLAPI void RLDrawModelPoints(RLModel model, RLVector3 position, float scale, RLColor tint); // Draw a model as points
RLAPI void RLDrawModelPointsEx(RLModel model, RLVector3 position, RLVector3 rotationAxis, float rotationAngle, RLVector3 scale, RLColor tint); // Draw a model as points with extended parameters
RLAPI void RLDrawBoundingBox(RLBoundingBox box, RLColor color);                                   // Draw bounding box (wires)
RLAPI void RLDrawBillboard(RLCamera camera, RLTexture2D texture, RLVector3 position, float scale, RLColor tint);   // Draw a billboard texture
RLAPI void RLDrawBillboardRec(RLCamera camera, RLTexture2D texture, RLRectangle source, RLVector3 position, RLVector2 size, RLColor tint); // Draw a billboard texture defined by source
RLAPI void RLDrawBillboardPro(RLCamera camera, RLTexture2D texture, RLRectangle source, RLVector3 position, RLVector3 up, RLVector2 size, RLVector2 origin, float rotation, RLColor tint); // Draw a billboard texture defined by source and rotation

// Mesh management functions
RLAPI void RLUploadMesh(RLMesh *mesh, bool dynamic);                                            // Upload mesh vertex data in GPU and provide VAO/VBO ids
RLAPI void RLUpdateMeshBuffer(RLMesh mesh, int index, const void *data, int dataSize, int offset); // Update mesh vertex data in GPU for a specific buffer index
RLAPI void RLUnloadMesh(RLMesh mesh);                                                           // Unload mesh data from CPU and GPU
RLAPI void RLDrawMesh(RLMesh mesh, RLMaterial material, RLMatrix transform);                        // Draw a 3d mesh with material and transform
RLAPI void RLDrawMeshInstanced(RLMesh mesh, RLMaterial material, const RLMatrix *transforms, int instances); // Draw multiple mesh instances with material and different transforms
RLAPI RLBoundingBox RLGetMeshBoundingBox(RLMesh mesh);                                            // Compute mesh bounding box limits
RLAPI void RLGenMeshTangents(RLMesh *mesh);                                                     // Compute mesh tangents
RLAPI bool RLExportMesh(RLMesh mesh, const char *fileName);                                     // Export mesh data to file, returns true on success
RLAPI bool RLExportMeshAsCode(RLMesh mesh, const char *fileName);                               // Export mesh as code file (.h) defining multiple arrays of vertex attributes

// Mesh generation functions
RLAPI RLMesh RLGenMeshPoly(int sides, float radius);                                            // Generate polygonal mesh
RLAPI RLMesh RLGenMeshPlane(float width, float length, int resX, int resZ);                     // Generate plane mesh (with subdivisions)
RLAPI RLMesh RLGenMeshCube(float width, float height, float length);                            // Generate cuboid mesh
RLAPI RLMesh RLGenMeshSphere(float radius, int rings, int slices);                              // Generate sphere mesh (standard sphere)
RLAPI RLMesh RLGenMeshHemiSphere(float radius, int rings, int slices);                          // Generate half-sphere mesh (no bottom cap)
RLAPI RLMesh RLGenMeshCylinder(float radius, float height, int slices);                         // Generate cylinder mesh
RLAPI RLMesh RLGenMeshCone(float radius, float height, int slices);                             // Generate cone/pyramid mesh
RLAPI RLMesh RLGenMeshTorus(float radius, float size, int radSeg, int sides);                   // Generate torus mesh
RLAPI RLMesh RLGenMeshKnot(float radius, float size, int radSeg, int sides);                    // Generate trefoil knot mesh
RLAPI RLMesh RLGenMeshHeightmap(RLImage heightmap, RLVector3 size);                                 // Generate heightmap mesh from image data
RLAPI RLMesh RLGenMeshCubicmap(RLImage cubicmap, RLVector3 cubeSize);                               // Generate cubes-based map mesh from image data

// Material loading/unloading functions
RLAPI RLMaterial *RLLoadMaterials(const char *fileName, int *materialCount);                    // Load materials from model file
RLAPI RLMaterial RLLoadMaterialDefault(void);                                                   // Load default material (Supports: DIFFUSE, SPECULAR, NORMAL maps)
RLAPI bool RLIsMaterialValid(RLMaterial material);                                              // Check if a material is valid (shader assigned, map textures loaded in GPU)
RLAPI void RLUnloadMaterial(RLMaterial material);                                               // Unload material from GPU memory (VRAM)
RLAPI void RLSetMaterialTexture(RLMaterial *material, int mapType, RLTexture2D texture);          // Set texture for a material map type (MATERIAL_MAP_DIFFUSE, MATERIAL_MAP_SPECULAR...)
RLAPI void RLSetModelMeshMaterial(RLModel *model, int meshId, int materialId);                  // Set material for a mesh

// Model animations loading/unloading functions
RLAPI RLModelAnimation *RLLoadModelAnimations(const char *fileName, int *animCount);            // Load model animations from file
RLAPI void RLUpdateModelAnimation(RLModel model, RLModelAnimation anim, int frame);               // Update model animation pose (CPU)
RLAPI void RLUpdateModelAnimationBones(RLModel model, RLModelAnimation anim, int frame);          // Update model animation mesh bone matrices (GPU skinning)
RLAPI void RLUnloadModelAnimation(RLModelAnimation anim);                                       // Unload animation data
RLAPI void RLUnloadModelAnimations(RLModelAnimation *animations, int animCount);                // Unload animation array data
RLAPI bool RLIsModelAnimationValid(RLModel model, RLModelAnimation anim);                         // Check model animation skeleton match

// Collision detection functions
RLAPI bool RLCheckCollisionSpheres(RLVector3 center1, float radius1, RLVector3 center2, float radius2); // Check collision between two spheres
RLAPI bool RLCheckCollisionBoxes(RLBoundingBox box1, RLBoundingBox box2);                         // Check collision between two bounding boxes
RLAPI bool RLCheckCollisionBoxSphere(RLBoundingBox box, RLVector3 center, float radius);          // Check collision between box and sphere
RLAPI RLRayCollision RLGetRayCollisionSphere(RLRay ray, RLVector3 center, float radius);            // Get collision info between ray and sphere
RLAPI RLRayCollision RLGetRayCollisionBox(RLRay ray, RLBoundingBox box);                            // Get collision info between ray and box
RLAPI RLRayCollision RLGetRayCollisionMesh(RLRay ray, RLMesh mesh, RLMatrix transform);               // Get collision info between ray and mesh
RLAPI RLRayCollision RLGetRayCollisionTriangle(RLRay ray, RLVector3 p1, RLVector3 p2, RLVector3 p3);    // Get collision info between ray and triangle
RLAPI RLRayCollision RLGetRayCollisionQuad(RLRay ray, RLVector3 p1, RLVector3 p2, RLVector3 p3, RLVector3 p4); // Get collision info between ray and quad

//------------------------------------------------------------------------------------
// Audio Loading and Playing Functions (Module: audio)
//------------------------------------------------------------------------------------
typedef void (*RLAudioCallback)(void *bufferData, unsigned int frames);

// Audio device management functions
RLAPI void RLInitAudioDevice(void);                                     // Initialize audio device and context
RLAPI void RLCloseAudioDevice(void);                                    // Close the audio device and context
RLAPI bool RLIsAudioDeviceReady(void);                                  // Check if audio device has been initialized successfully
RLAPI void RLSetMasterVolume(float volume);                             // Set master volume (listener)
RLAPI float RLGetMasterVolume(void);                                    // Get master volume (listener)

// Wave/Sound loading/unloading functions
RLAPI RLWave RLLoadWave(const char *fileName);                            // Load wave data from file
RLAPI RLWave RLLoadWaveFromMemory(const char *fileType, const unsigned char *fileData, int dataSize); // Load wave from memory buffer, fileType refers to extension: i.e. '.wav'
RLAPI bool RLIsWaveValid(RLWave wave);                                    // Checks if wave data is valid (data loaded and parameters)
RLAPI RLSound RLLoadSound(const char *fileName);                          // Load sound from file
RLAPI RLSound RLLoadSoundFromWave(RLWave wave);                             // Load sound from wave data
RLAPI RLSound RLLoadSoundAlias(RLSound source);                             // Create a new sound that shares the same sample data as the source sound, does not own the sound data
RLAPI bool RLIsSoundValid(RLSound sound);                                 // Checks if a sound is valid (data loaded and buffers initialized)
RLAPI void RLUpdateSound(RLSound sound, const void *data, int sampleCount); // Update sound buffer with new data (default data format: 32 bit float, stereo)
RLAPI void RLUnloadWave(RLWave wave);                                     // Unload wave data
RLAPI void RLUnloadSound(RLSound sound);                                  // Unload sound
RLAPI void RLUnloadSoundAlias(RLSound alias);                             // Unload a sound alias (does not deallocate sample data)
RLAPI bool RLExportWave(RLWave wave, const char *fileName);               // Export wave data to file, returns true on success
RLAPI bool RLExportWaveAsCode(RLWave wave, const char *fileName);         // Export wave sample data to code (.h), returns true on success

// Wave/Sound management functions
RLAPI void RLPlaySound(RLSound sound);                                    // Play a sound
RLAPI void RLStopSound(RLSound sound);                                    // Stop playing a sound
RLAPI void RLPauseSound(RLSound sound);                                   // Pause a sound
RLAPI void RLResumeSound(RLSound sound);                                  // Resume a paused sound
RLAPI bool RLIsSoundPlaying(RLSound sound);                               // Check if a sound is currently playing
RLAPI void RLSetSoundVolume(RLSound sound, float volume);                 // Set volume for a sound (1.0 is max level)
RLAPI void RLSetSoundPitch(RLSound sound, float pitch);                   // Set pitch for a sound (1.0 is base level)
RLAPI void RLSetSoundPan(RLSound sound, float pan);                       // Set pan for a sound (-1.0 left, 0.0 center, 1.0 right)
RLAPI RLWave RLWaveCopy(RLWave wave);                                       // Copy a wave to a new wave
RLAPI void RLWaveCrop(RLWave *wave, int initFrame, int finalFrame);       // Crop a wave to defined frames range
RLAPI void RLWaveFormat(RLWave *wave, int sampleRate, int sampleSize, int channels); // Convert wave data to desired format
RLAPI float *RLLoadWaveSamples(RLWave wave);                              // Load samples data from wave as a 32bit float data array
RLAPI void RLUnloadWaveSamples(float *samples);                         // Unload samples data loaded with LoadWaveSamples()

// Music management functions
RLAPI RLMusic RLLoadMusicStream(const char *fileName);                    // Load music stream from file
RLAPI RLMusic RLLoadMusicStreamFromMemory(const char *fileType, const unsigned char *data, int dataSize); // Load music stream from data
RLAPI bool RLIsMusicValid(RLMusic music);                                 // Checks if a music stream is valid (context and buffers initialized)
RLAPI void RLUnloadMusicStream(RLMusic music);                            // Unload music stream
RLAPI void RLPlayMusicStream(RLMusic music);                              // Start music playing
RLAPI bool RLIsMusicStreamPlaying(RLMusic music);                         // Check if music is playing
RLAPI void RLUpdateMusicStream(RLMusic music);                            // Updates buffers for music streaming
RLAPI void RLStopMusicStream(RLMusic music);                              // Stop music playing
RLAPI void RLPauseMusicStream(RLMusic music);                             // Pause music playing
RLAPI void RLResumeMusicStream(RLMusic music);                            // Resume playing paused music
RLAPI void RLSeekMusicStream(RLMusic music, float position);              // Seek music to a position (in seconds)
RLAPI void RLSetMusicVolume(RLMusic music, float volume);                 // Set volume for music (1.0 is max level)
RLAPI void RLSetMusicPitch(RLMusic music, float pitch);                   // Set pitch for a music (1.0 is base level)
RLAPI void RLSetMusicPan(RLMusic music, float pan);                       // Set pan for a music (-1.0 left, 0.0 center, 1.0 right)
RLAPI float RLGetMusicTimeLength(RLMusic music);                          // Get music time length (in seconds)
RLAPI float RLGetMusicTimePlayed(RLMusic music);                          // Get current music time played (in seconds)

// AudioStream management functions
RLAPI RLAudioStream RLLoadAudioStream(unsigned int sampleRate, unsigned int sampleSize, unsigned int channels); // Load audio stream (to stream raw audio pcm data)
RLAPI bool RLIsAudioStreamValid(RLAudioStream stream);                    // Checks if an audio stream is valid (buffers initialized)
RLAPI void RLUnloadAudioStream(RLAudioStream stream);                     // Unload audio stream and free memory
RLAPI void RLUpdateAudioStream(RLAudioStream stream, const void *data, int frameCount); // Update audio stream buffers with data
RLAPI bool RLIsAudioStreamProcessed(RLAudioStream stream);                // Check if any audio stream buffers requires refill
RLAPI void RLPlayAudioStream(RLAudioStream stream);                       // Play audio stream
RLAPI void RLPauseAudioStream(RLAudioStream stream);                      // Pause audio stream
RLAPI void RLResumeAudioStream(RLAudioStream stream);                     // Resume audio stream
RLAPI bool RLIsAudioStreamPlaying(RLAudioStream stream);                  // Check if audio stream is playing
RLAPI void RLStopAudioStream(RLAudioStream stream);                       // Stop audio stream
RLAPI void RLSetAudioStreamVolume(RLAudioStream stream, float volume);    // Set volume for audio stream (1.0 is max level)
RLAPI void RLSetAudioStreamPitch(RLAudioStream stream, float pitch);      // Set pitch for audio stream (1.0 is base level)
RLAPI void RLSetAudioStreamPan(RLAudioStream stream, float pan);          // Set pan for audio stream (0.5 is centered)
RLAPI void RLSetAudioStreamBufferSizeDefault(int size);                 // Default size for new audio streams
RLAPI void RLSetAudioStreamCallback(RLAudioStream stream, RLAudioCallback callback); // Audio thread callback to request new data

RLAPI void RLAttachAudioStreamProcessor(RLAudioStream stream, RLAudioCallback processor); // Attach audio stream processor to stream, receives frames x 2 samples as 'float' (stereo)
RLAPI void RLDetachAudioStreamProcessor(RLAudioStream stream, RLAudioCallback processor); // Detach audio stream processor from stream

RLAPI void RLAttachAudioMixedProcessor(RLAudioCallback processor); // Attach audio stream processor to the entire audio pipeline, receives frames x 2 samples as 'float' (stereo)
RLAPI void RLDetachAudioMixedProcessor(RLAudioCallback processor); // Detach audio stream processor from the entire audio pipeline

//------------------------------------------------------------------------------------
// Event-thread diagnostics (desktop GLFW Win32 extensions)
//------------------------------------------------------------------------------------
// NOTE: Meaningful only when FLAG_WINDOW_EVENT_THREAD is used and the library is built with
// RL_EVENT_DIAG_STATS=1 (see src/config.h or compile definitions).

typedef struct RLEventThreadDiagStats {
    // Render-thread task envelope (internal)
    unsigned long long renderCallAlloc;
    unsigned long long renderCallFree;

    // Payload allocations (common input/window callbacks posted across threads)
    unsigned long long payloadAlloc;
    unsigned long long payloadFree;
    unsigned long long payloadAllocBytes;
    unsigned long long payloadFreeBytes;
    unsigned long long payloadOutstandingMax;

    // Per-payload breakdown (counts only; bytes are included in payload*Bytes totals)
    unsigned long long mouseMoveAlloc, mouseMoveFree;
    unsigned long long mouseWheelAlloc, mouseWheelFree;
    unsigned long long mouseButtonAlloc, mouseButtonFree;
    unsigned long long keyAlloc, keyFree;
    unsigned long long chAlloc, chFree;
    unsigned long long winPosAlloc, winPosFree;
    unsigned long long fbSizeAlloc, fbSizeFree;
    unsigned long long scaleAlloc, scaleFree;
    unsigned long long dropAlloc, dropFree;
    unsigned long long winCloseAlloc, winCloseFree;
    unsigned long long otherAlloc, otherFree;

    // Task/pump performance
    unsigned long long tasksPosted;
    unsigned long long tasksExecuted;
    unsigned long long pumpCalls;
    unsigned long long pumpTasksExecutedTotal;
    unsigned int        pumpTasksExecutedMax;
    double              pumpTimeTotalMs;
    double              pumpTimeMaxMs;
} RLEventThreadDiagStats;

RLAPI RLEventThreadDiagStats RLGetEventThreadDiagStats(void);
RLAPI void RLResetEventThreadDiagStats(void);

#if defined(__cplusplus)
}
#endif

#endif // RAYLIB_H
