/**********************************************************************************************
*
*   rcore_android - Functions to manage window, graphics device and inputs
*
*   PLATFORM: ANDROID
*       - Android (ARM, ARM64)
*
*   LIMITATIONS:
*       - Limitation 01
*       - Limitation 02
*
*   POSSIBLE IMPROVEMENTS:
*       - Improvement 01
*       - Improvement 02
*
*   CONFIGURATION:
*       #define RCORE_PLATFORM_CUSTOM_FLAG
*           Custom flag for rcore on target platform -not used-
*
*   DEPENDENCIES:
*       - Android NDK: Provides C API to access Android functionality
*       - gestures: Gestures system for touch-ready devices (or simulated from mouse inputs)
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2013-2026 Ramon Santamaria (@raysan5) and contributors
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

#include <android_native_app_glue.h>    // Required for: android_app struct and activity management
#include <android/window.h>             // Required for: AWINDOW_FLAG_FULLSCREEN definition and others
#include <android/log.h>                // Required for: Android log system: __android_log_vprint()
#include <android/asset_manager.h>      // Required for: AAssetManager
//#include <android/sensor.h>           // Required for: Android sensors functions (accelerometer, gyroscope, light...)

#include <errno.h>                      // Required for: error types
#include <jni.h>                        // Required for: JNIEnv and JavaVM [Used in OpenURL() and GetCurrentMonitor()]

#include <EGL/egl.h>                    // Native platform windowing system interface

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct {
    // Application data
    struct android_app *app;            // Android activity
    struct android_poll_source *source; // Android events polling source
    bool appEnabled;                    // Flag to detect if app is active ** = true
    bool contextRebindRequired;         // Used to know context rebind required

    // Display data
    EGLDisplay device;                  // Native display device (physical screen connection)
    EGLSurface surface;                 // Surface to draw on, framebuffers (connected to context)
    EGLContext context;                 // Graphic context, mode in which drawing can be done
    EGLConfig config;                   // Graphic config
} PlatformData;

typedef struct {
    // Store data for both Hover and Touch events
    // Used to ignore Hover events which are interpreted as Touch events
    int32_t pointCount;                             // Number of touch points active
    int32_t pointId[MAX_TOUCH_POINTS];              // Point identifiers
    RLVector2 position[MAX_TOUCH_POINTS];             // Touch position on screen

    int32_t hoverPoints[MAX_TOUCH_POINTS];          // Hover Points
} TouchRaw;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
extern CoreData CORE;                   // Global CORE state context
static PlatformData platform = { 0 };   // Platform specific data

//----------------------------------------------------------------------------------
// Local Variables Definition
//----------------------------------------------------------------------------------
#define KEYCODE_MAP_SIZE 162
static const RLKeyboardKey mapKeycode[KEYCODE_MAP_SIZE] = {
    RL_E_KEY_NULL,           // AKEYCODE_UNKNOWN
    0,                  // AKEYCODE_SOFT_LEFT
    0,                  // AKEYCODE_SOFT_RIGHT
    0,                  // AKEYCODE_HOME
    RL_E_KEY_BACK,           // AKEYCODE_BACK
    0,                  // AKEYCODE_CALL
    0,                  // AKEYCODE_ENDCALL
    RL_E_KEY_ZERO,           // AKEYCODE_0
    RL_E_KEY_ONE,            // AKEYCODE_1
    RL_E_KEY_TWO,            // AKEYCODE_2
    RL_E_KEY_THREE,          // AKEYCODE_3
    RL_E_KEY_FOUR,           // AKEYCODE_4
    RL_E_KEY_FIVE,           // AKEYCODE_5
    RL_E_KEY_SIX,            // AKEYCODE_6
    RL_E_KEY_SEVEN,          // AKEYCODE_7
    RL_E_KEY_EIGHT,          // AKEYCODE_8
    RL_E_KEY_NINE,           // AKEYCODE_9
    0,                  // AKEYCODE_STAR
    0,                  // AKEYCODE_POUND
    RL_E_KEY_UP,             // AKEYCODE_DPAD_UP
    RL_E_KEY_DOWN,           // AKEYCODE_DPAD_DOWN
    RL_E_KEY_LEFT,           // AKEYCODE_DPAD_LEFT
    RL_E_KEY_RIGHT,          // AKEYCODE_DPAD_RIGHT
    0,                  // AKEYCODE_DPAD_CENTER
    RL_E_KEY_VOLUME_UP,      // AKEYCODE_VOLUME_UP
    RL_E_KEY_VOLUME_DOWN,    // AKEYCODE_VOLUME_DOWN
    0,                  // AKEYCODE_POWER
    0,                  // AKEYCODE_CAMERA
    0,                  // AKEYCODE_CLEAR
    RL_E_KEY_A,              // AKEYCODE_A
    RL_E_KEY_B,              // AKEYCODE_B
    RL_E_KEY_C,              // AKEYCODE_C
    RL_E_KEY_D,              // AKEYCODE_D
    RL_E_KEY_E,              // AKEYCODE_E
    RL_E_KEY_F,              // AKEYCODE_F
    RL_E_KEY_G,              // AKEYCODE_G
    RL_E_KEY_H,              // AKEYCODE_H
    RL_E_KEY_I,              // AKEYCODE_I
    RL_E_KEY_J,              // AKEYCODE_J
    RL_E_KEY_K,              // AKEYCODE_K
    RL_E_KEY_L,              // AKEYCODE_L
    RL_E_KEY_M,              // AKEYCODE_M
    RL_E_KEY_N,              // AKEYCODE_N
    RL_E_KEY_O,              // AKEYCODE_O
    RL_E_KEY_P,              // AKEYCODE_P
    RL_E_KEY_Q,              // AKEYCODE_Q
    RL_E_KEY_R,              // AKEYCODE_R
    RL_E_KEY_S,              // AKEYCODE_S
    RL_E_KEY_T,              // AKEYCODE_T
    RL_E_KEY_U,              // AKEYCODE_U
    RL_E_KEY_V,              // AKEYCODE_V
    RL_E_KEY_W,              // AKEYCODE_W
    RL_E_KEY_X,              // AKEYCODE_X
    RL_E_KEY_Y,              // AKEYCODE_Y
    RL_E_KEY_Z,              // AKEYCODE_Z
    RL_E_KEY_COMMA,          // AKEYCODE_COMMA
    RL_E_KEY_PERIOD,         // AKEYCODE_PERIOD
    RL_E_KEY_LEFT_ALT,       // AKEYCODE_ALT_LEFT
    RL_E_KEY_RIGHT_ALT,      // AKEYCODE_ALT_RIGHT
    RL_E_KEY_LEFT_SHIFT,     // AKEYCODE_SHIFT_LEFT
    RL_E_KEY_RIGHT_SHIFT,    // AKEYCODE_SHIFT_RIGHT
    RL_E_KEY_TAB,            // AKEYCODE_TAB
    RL_E_KEY_SPACE,          // AKEYCODE_SPACE
    0,                  // AKEYCODE_SYM
    0,                  // AKEYCODE_EXPLORER
    0,                  // AKEYCODE_ENVELOPE
    RL_E_KEY_ENTER,          // AKEYCODE_ENTER
    RL_E_KEY_BACKSPACE,      // AKEYCODE_DEL
    RL_E_KEY_GRAVE,          // AKEYCODE_GRAVE
    RL_E_KEY_MINUS,          // AKEYCODE_MINUS
    RL_E_KEY_EQUAL,          // AKEYCODE_EQUALS
    RL_E_KEY_LEFT_BRACKET,   // AKEYCODE_LEFT_BRACKET
    RL_E_KEY_RIGHT_BRACKET,  // AKEYCODE_RIGHT_BRACKET
    RL_E_KEY_BACKSLASH,      // AKEYCODE_BACKSLASH
    RL_E_KEY_SEMICOLON,      // AKEYCODE_SEMICOLON
    RL_E_KEY_APOSTROPHE,     // AKEYCODE_APOSTROPHE
    RL_E_KEY_SLASH,          // AKEYCODE_SLASH
    0,                  // AKEYCODE_AT
    0,                  // AKEYCODE_NUM
    0,                  // AKEYCODE_HEADSETHOOK
    0,                  // AKEYCODE_FOCUS
    0,                  // AKEYCODE_PLUS
    RL_E_KEY_MENU,           // AKEYCODE_MENU
    0,                  // AKEYCODE_NOTIFICATION
    0,                  // AKEYCODE_SEARCH
    0,                  // AKEYCODE_MEDIA_PLAY_PAUSE
    0,                  // AKEYCODE_MEDIA_STOP
    0,                  // AKEYCODE_MEDIA_NEXT
    0,                  // AKEYCODE_MEDIA_PREVIOUS
    0,                  // AKEYCODE_MEDIA_REWIND
    0,                  // AKEYCODE_MEDIA_FAST_FORWARD
    0,                  // AKEYCODE_MUTE
    RL_E_KEY_PAGE_UP,        // AKEYCODE_PAGE_UP
    RL_E_KEY_PAGE_DOWN,      // AKEYCODE_PAGE_DOWN
    0,                  // AKEYCODE_PICTSYMBOLS
    0,                  // AKEYCODE_SWITCH_CHARSET
    0,                  // AKEYCODE_BUTTON_A
    0,                  // AKEYCODE_BUTTON_B
    0,                  // AKEYCODE_BUTTON_C
    0,                  // AKEYCODE_BUTTON_X
    0,                  // AKEYCODE_BUTTON_Y
    0,                  // AKEYCODE_BUTTON_Z
    0,                  // AKEYCODE_BUTTON_L1
    0,                  // AKEYCODE_BUTTON_R1
    0,                  // AKEYCODE_BUTTON_L2
    0,                  // AKEYCODE_BUTTON_R2
    0,                  // AKEYCODE_BUTTON_THUMBL
    0,                  // AKEYCODE_BUTTON_THUMBR
    0,                  // AKEYCODE_BUTTON_START
    0,                  // AKEYCODE_BUTTON_SELECT
    0,                  // AKEYCODE_BUTTON_MODE
    RL_E_KEY_ESCAPE,         // AKEYCODE_ESCAPE
    RL_E_KEY_DELETE,         // AKEYCODE_FORWARD_DELL
    RL_E_KEY_LEFT_CONTROL,   // AKEYCODE_CTRL_LEFT
    RL_E_KEY_RIGHT_CONTROL,  // AKEYCODE_CTRL_RIGHT
    RL_E_KEY_CAPS_LOCK,      // AKEYCODE_CAPS_LOCK
    RL_E_KEY_SCROLL_LOCK,    // AKEYCODE_SCROLL_LOCK
    RL_E_KEY_LEFT_SUPER,     // AKEYCODE_META_LEFT
    RL_E_KEY_RIGHT_SUPER,    // AKEYCODE_META_RIGHT
    0,                  // AKEYCODE_FUNCTION
    RL_E_KEY_PRINT_SCREEN,   // AKEYCODE_SYSRQ
    RL_E_KEY_PAUSE,          // AKEYCODE_BREAK
    RL_E_KEY_HOME,           // AKEYCODE_MOVE_HOME
    RL_E_KEY_END,            // AKEYCODE_MOVE_END
    RL_E_KEY_INSERT,         // AKEYCODE_INSERT
    0,                  // AKEYCODE_FORWARD
    0,                  // AKEYCODE_MEDIA_PLAY
    0,                  // AKEYCODE_MEDIA_PAUSE
    0,                  // AKEYCODE_MEDIA_CLOSE
    0,                  // AKEYCODE_MEDIA_EJECT
    0,                  // AKEYCODE_MEDIA_RECORD
    RL_E_KEY_F1,             // AKEYCODE_F1
    RL_E_KEY_F2,             // AKEYCODE_F2
    RL_E_KEY_F3,             // AKEYCODE_F3
    RL_E_KEY_F4,             // AKEYCODE_F4
    RL_E_KEY_F5,             // AKEYCODE_F5
    RL_E_KEY_F6,             // AKEYCODE_F6
    RL_E_KEY_F7,             // AKEYCODE_F7
    RL_E_KEY_F8,             // AKEYCODE_F8
    RL_E_KEY_F9,             // AKEYCODE_F9
    RL_E_KEY_F10,            // AKEYCODE_F10
    RL_E_KEY_F11,            // AKEYCODE_F11
    RL_E_KEY_F12,            // AKEYCODE_F12
    RL_E_KEY_NUM_LOCK,       // AKEYCODE_NUM_LOCK
    RL_E_KEY_KP_0,           // AKEYCODE_NUMPAD_0
    RL_E_KEY_KP_1,           // AKEYCODE_NUMPAD_1
    RL_E_KEY_KP_2,           // AKEYCODE_NUMPAD_2
    RL_E_KEY_KP_3,           // AKEYCODE_NUMPAD_3
    RL_E_KEY_KP_4,           // AKEYCODE_NUMPAD_4
    RL_E_KEY_KP_5,           // AKEYCODE_NUMPAD_5
    RL_E_KEY_KP_6,           // AKEYCODE_NUMPAD_6
    RL_E_KEY_KP_7,           // AKEYCODE_NUMPAD_7
    RL_E_KEY_KP_8,           // AKEYCODE_NUMPAD_8
    RL_E_KEY_KP_9,           // AKEYCODE_NUMPAD_9
    RL_E_KEY_KP_DIVIDE,      // AKEYCODE_NUMPAD_DIVIDE
    RL_E_KEY_KP_MULTIPLY,    // AKEYCODE_NUMPAD_MULTIPLY
    RL_E_KEY_KP_SUBTRACT,    // AKEYCODE_NUMPAD_SUBTRACT
    RL_E_KEY_KP_ADD,         // AKEYCODE_NUMPAD_ADD
    RL_E_KEY_KP_DECIMAL,     // AKEYCODE_NUMPAD_DOT
    0,                  // AKEYCODE_NUMPAD_COMMA
    RL_E_KEY_KP_ENTER,       // AKEYCODE_NUMPAD_ENTER
    RL_E_KEY_KP_EQUAL        // AKEYCODE_NUMPAD_EQUALS
};

static TouchRaw touchRaw = { 0 };

//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------
int InitPlatform(void);          // Initialize platform (graphics, inputs and more)
void ClosePlatform(void);        // Close platform

static void AndroidCommandCallback(struct android_app *app, int32_t cmd);           // Process Android activity lifecycle commands
static int32_t AndroidInputCallback(struct android_app *app, AInputEvent *event);   // Process Android inputs
static RLGamepadButton AndroidTranslateGamepadButton(int button);                     // Map Android gamepad button to raylib gamepad button

static void SetupFramebuffer(int width, int height); // Setup main framebuffer (required by InitPlatform())

static int android_read(void *cookie, char *buf, int size);
static int android_write(void *cookie, const char *buf, int size);
static fpos_t android_seek(void *cookie, fpos_t offset, int whence);
static int android_close(void *cookie);

FILE *android_fopen(const char *fileName, const char *mode); // Replacement for fopen() -> Read-only!
FILE *funopen(const void *cookie, int (*readfn)(void *, char *, int), int (*writefn)(void *, const char *, int),
              fpos_t (*seekfn)(void *, fpos_t, int), int (*closefn)(void *));

#define fopen(name, mode) android_fopen(name, mode)

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
// NOTE: Functions declaration is provided by raylib.h

//----------------------------------------------------------------------------------
// Module Functions Definition: Application
//----------------------------------------------------------------------------------

// To allow easier porting to android, allow the user to define a
// custom main function which is called from android_main
extern int main(int argc, char *argv[]);

// Android main function
void android_main(struct android_app *app)
{
    char arg0[] = "raylib";     // NOTE: argv[] are mutable
    platform.app = app;

    // NOTE: Return from main is ignored
    (void)main(1, (char *[]) { arg0, NULL });

    // Request to end the native activity
    ANativeActivity_finish(app->activity);

    // Android ALooper_pollOnce() variables
    int pollResult = 0;
    int pollEvents = 0;

    // Waiting for application events before complete finishing
    while (!app->destroyRequested)
    {
        // Poll all events until return value TIMEOUT is reached, meaning no events left to process
        while ((pollResult = ALooper_pollOnce(0, NULL, &pollEvents, (void **)&platform.source)) > ALOOPER_POLL_TIMEOUT)
        {
            if (platform.source != NULL) platform.source->process(app, platform.source);
        }
    }
}

// NOTE: Add this to header (if apps really need it)
struct android_app *GetAndroidApp(void)
{
    return platform.app;
}

//----------------------------------------------------------------------------------
// Module Functions Definition: Window and Graphics Device
//----------------------------------------------------------------------------------

// Check if application should close
bool RLWindowShouldClose(void)
{
    if (CORE.Window.ready) return CORE.Window.shouldClose;
    else return true;
}

// Toggle fullscreen mode
void RLToggleFullscreen(void)
{
    TRACELOG(RL_E_LOG_WARNING, "ToggleFullscreen() not available on target platform");
}

// Toggle borderless windowed mode
void RLToggleBorderlessWindowed(void)
{
    TRACELOG(RL_E_LOG_WARNING, "ToggleBorderlessWindowed() not available on target platform");
}

// Set window state: maximized, if resizable
void RLMaximizeWindow(void)
{
    TRACELOG(RL_E_LOG_WARNING, "MaximizeWindow() not available on target platform");
}

// Set window state: minimized
void RLMinimizeWindow(void)
{
    TRACELOG(RL_E_LOG_WARNING, "MinimizeWindow() not available on target platform");
}

// Restore window from being minimized/maximized
void RLRestoreWindow(void)
{
    TRACELOG(RL_E_LOG_WARNING, "RestoreWindow() not available on target platform");
}

// Set window configuration state using flags
void RLSetWindowState(unsigned int flags)
{
    if (!CORE.Window.ready) TRACELOG(RL_E_LOG_WARNING, "WINDOW: SetWindowState does nothing before window initialization, Use \"SetConfigFlags\" instead");

    // State change: FLAG_WINDOW_ALWAYS_RUN
    if (FLAG_IS_SET(flags, RL_E_FLAG_WINDOW_ALWAYS_RUN)) FLAG_SET(CORE.Window.flags, RL_E_FLAG_WINDOW_ALWAYS_RUN);
}

// Clear window configuration state flags
void RLClearWindowState(unsigned int flags)
{
    // State change: FLAG_WINDOW_ALWAYS_RUN
    if (FLAG_IS_SET(flags, RL_E_FLAG_WINDOW_ALWAYS_RUN)) FLAG_CLEAR(CORE.Window.flags, RL_E_FLAG_WINDOW_ALWAYS_RUN);
}

// Set icon for window
void RLSetWindowIcon(RLImage image)
{
    TRACELOG(RL_E_LOG_WARNING, "SetWindowIcon() not available on target platform");
}

// Set icon for window
void RLSetWindowIcons(RLImage *images, int count)
{
    TRACELOG(RL_E_LOG_WARNING, "SetWindowIcons() not available on target platform");
}

// Set title for window
void RLSetWindowTitle(const char *title)
{
    CORE.Window.title = title;
}

// Set window position on screen (windowed mode)
void RLSetWindowPosition(int x, int y)
{
    TRACELOG(RL_E_LOG_WARNING, "SetWindowPosition() not available on target platform");
}

// Set monitor for the current window
void RLSetWindowMonitor(int monitor)
{
    TRACELOG(RL_E_LOG_WARNING, "SetWindowMonitor() not available on target platform");
}

// Set window minimum dimensions (FLAG_WINDOW_RESIZABLE)
void RLSetWindowMinSize(int width, int height)
{
    CORE.Window.screenMin.width = width;
    CORE.Window.screenMin.height = height;
}

// Set window maximum dimensions (FLAG_WINDOW_RESIZABLE)
void RLSetWindowMaxSize(int width, int height)
{
    CORE.Window.screenMax.width = width;
    CORE.Window.screenMax.height = height;
}

// Set window dimensions
void RLSetWindowSize(int width, int height)
{
    TRACELOG(RL_E_LOG_WARNING, "SetWindowSize() not available on target platform");
}

// Set window opacity, value opacity is between 0.0 and 1.0
void RLSetWindowOpacity(float opacity)
{
    TRACELOG(RL_E_LOG_WARNING, "SetWindowOpacity() not available on target platform");
}

// Set window focused
void RLSetWindowFocused(void)
{
    TRACELOG(RL_E_LOG_WARNING, "SetWindowFocused() not available on target platform");
}

// Get native window handle
void *RLGetWindowHandle(void)
{
    TRACELOG(RL_E_LOG_WARNING, "GetWindowHandle() not implemented on target platform");
    return NULL;
}

// Get number of monitors
int RLGetMonitorCount(void)
{
    TRACELOG(RL_E_LOG_WARNING, "GetMonitorCount() not implemented on target platform");
    return 1;
}

// Get current monitor where window is placed
int RLGetCurrentMonitor(void)
{
    int displayId = -1;
    JNIEnv *env = NULL;
    JavaVM *vm = platform.app->activity->vm;
    (*vm)->AttachCurrentThread(vm, &env, NULL);

    jobject activity = platform.app->activity->clazz;
    jclass activityClass = (*env)->GetObjectClass(env, activity);

    jmethodID getDisplayMethod = (*env)->GetMethodID(env, activityClass, "getDisplay", "()Landroid/view/Display;");

    jobject display = (*env)->CallObjectMethod(env, activity, getDisplayMethod);

    if (display == NULL)
    {
        TRACELOG(RL_E_LOG_ERROR, "GetCurrentMonitor() couldn't get the display object");
    }
    else
    {
        jclass displayClass = (*env)->FindClass(env, "android/view/Display");
        jmethodID getDisplayIdMethod = (*env)->GetMethodID(env, displayClass, "getDisplayId", "()I");
        displayId = (int)(*env)->CallIntMethod(env, display, getDisplayIdMethod);
        (*env)->DeleteLocalRef(env, displayClass);
    }

    (*env)->DeleteLocalRef(env, activityClass);
    (*env)->DeleteLocalRef(env, display);

    (*vm)->DetachCurrentThread(vm);
    return displayId;
}

// Get selected monitor position
RLVector2 RLGetMonitorPosition(int monitor)
{
    TRACELOG(RL_E_LOG_WARNING, "GetMonitorPosition() not implemented on target platform");
    return (RLVector2){ 0, 0 };
}

// Get selected monitor width (currently used by monitor)
int RLGetMonitorWidth(int monitor)
{
    TRACELOG(RL_E_LOG_WARNING, "GetMonitorWidth() not implemented on target platform");
    return 0;
}

// Get selected monitor height (currently used by monitor)
int RLGetMonitorHeight(int monitor)
{
    TRACELOG(RL_E_LOG_WARNING, "GetMonitorHeight() not implemented on target platform");
    return 0;
}

// Get selected monitor physical width in millimetres
// NOTE: It seems to return a slightly underestimated value on some devices
int RLGetMonitorPhysicalWidth(int monitor)
{
    int widthPixels = ANativeWindow_getWidth(platform.app->window);
    float dpi = AConfiguration_getDensity(platform.app->config);
    return (widthPixels/dpi)*25.4f;
}

// Get selected monitor physical height in millimetres
// NOTE: It seems to return a slightly underestimated value on some devices
int RLGetMonitorPhysicalHeight(int monitor)
{
    int heightPixels = ANativeWindow_getHeight(platform.app->window);
    float dpi = AConfiguration_getDensity(platform.app->config);
    return (heightPixels/dpi)*25.4f;
}

// Get selected monitor refresh rate
int RLGetMonitorRefreshRate(int monitor)
{
    TRACELOG(RL_E_LOG_WARNING, "GetMonitorRefreshRate() not implemented on target platform");
    return 0;
}

// Get the human-readable, UTF-8 encoded name of the selected monitor
const char *RLGetMonitorName(int monitor)
{
    TRACELOG(RL_E_LOG_WARNING, "GetMonitorName() not implemented on target platform");
    return "";
}

// Get window position XY on monitor
RLVector2 RLGetWindowPosition(void)
{
    TRACELOG(RL_E_LOG_WARNING, "GetWindowPosition() not implemented on target platform");
    return (RLVector2){ 0, 0 };
}

// Get window scale DPI factor for current monitor
RLVector2 RLGetWindowScaleDPI(void)
{
    int density = AConfiguration_getDensity(platform.app->config);
    float scale = (float)density/160;
    return (RLVector2){ scale, scale };
}

// Set clipboard text content
void RLSetClipboardText(const char *text)
{
    TRACELOG(RL_E_LOG_WARNING, "SetClipboardText() not implemented on target platform");
}

// Get clipboard text content
// NOTE: returned string is allocated and freed by GLFW
const char *RLGetClipboardText(void)
{
    TRACELOG(RL_E_LOG_WARNING, "GetClipboardText() not implemented on target platform");
    return NULL;
}

// Get clipboard image
RLImage RLGetClipboardImage(void)
{
    RLImage image = { 0 };

    TRACELOG(RL_E_LOG_WARNING, "GetClipboardImage() not implemented on target platform");

    return image;
}

// Show mouse cursor
void RLShowCursor(void)
{
    CORE.Input.Mouse.cursorHidden = false;
}

// Hides mouse cursor
void RLHideCursor(void)
{
    CORE.Input.Mouse.cursorHidden = true;
}

// Enables cursor (unlock cursor)
void RLEnableCursor(void)
{
    // Set cursor position in the middle
    RLSetMousePosition(CORE.Window.screen.width/2, CORE.Window.screen.height/2);

    CORE.Input.Mouse.cursorLocked = false;
}

// Disables cursor (lock cursor)
void RLDisableCursor(void)
{
    // Set cursor position in the middle
    RLSetMousePosition(CORE.Window.screen.width/2, CORE.Window.screen.height/2);

    CORE.Input.Mouse.cursorLocked = true;
}

// Swap back buffer with front buffer (screen drawing)
void RLSwapScreenBuffer(void)
{
    if (platform.surface != EGL_NO_SURFACE) eglSwapBuffers(platform.device, platform.surface);
}

//----------------------------------------------------------------------------------
// Module Functions Definition: Misc
//----------------------------------------------------------------------------------

// Get elapsed time measure in seconds since InitTimer()
double RLGetTime(void)
{
    double time = 0.0;
    struct timespec ts = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &ts);
    unsigned long long int nanoSeconds = (unsigned long long int)ts.tv_sec*1000000000LLU + (unsigned long long int)ts.tv_nsec;

    time = (double)(nanoSeconds - CORE.Time.base)*1e-9;  // Elapsed time since InitTimer()

    return time;
}

// Open URL with default system browser (if available)
// NOTE: This function is only safe to use if the provided URL is safe
// A user could craft a malicious string performing another action
// Avoid calling this function with user input non-validated strings
void RLOpenURL(const char *url)
{
    // Security check to (partially) avoid malicious code
    if (strchr(url, '\'') != NULL) TRACELOG(RL_E_LOG_WARNING, "SYSTEM: Provided URL could be potentially malicious, avoid [\'] character");
    else
    {
        JNIEnv *env = NULL;
        JavaVM *vm = platform.app->activity->vm;
        (*vm)->AttachCurrentThread(vm, &env, NULL);

        jstring urlString = (*env)->NewStringUTF(env, url);
        jclass uriClass = (*env)->FindClass(env, "android/net/Uri");
        jmethodID uriParse = (*env)->GetStaticMethodID(env, uriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
        jobject uri = (*env)->CallStaticObjectMethod(env, uriClass, uriParse, urlString);

        jclass intentClass = (*env)->FindClass(env, "android/content/Intent");
        jfieldID actionViewId = (*env)->GetStaticFieldID(env, intentClass, "ACTION_VIEW", "Ljava/lang/String;");
        jobject actionView = (*env)->GetStaticObjectField(env, intentClass, actionViewId);
        jmethodID newIntent = (*env)->GetMethodID(env, intentClass, "<init>", "(Ljava/lang/String;Landroid/net/Uri;)V");
        jobject intent = (*env)->AllocObject(env, intentClass);

        (*env)->CallVoidMethod(env, intent, newIntent, actionView, uri);
        jclass activityClass = (*env)->FindClass(env, "android/app/Activity");
        jmethodID startActivity = (*env)->GetMethodID(env, activityClass, "startActivity", "(Landroid/content/Intent;)V");
        (*env)->CallVoidMethod(env, platform.app->activity->clazz, startActivity, intent);

        (*vm)->DetachCurrentThread(vm);
    }
}

//----------------------------------------------------------------------------------
// Module Functions Definition: Inputs
//----------------------------------------------------------------------------------

// Set internal gamepad mappings
int RLSetGamepadMappings(const char *mappings)
{
    TRACELOG(RL_E_LOG_WARNING, "SetGamepadMappings() not implemented on target platform");
    return 0;
}

// Set gamepad vibration
void RLSetGamepadVibration(int gamepad, float leftMotor, float rightMotor, float duration)
{
    TRACELOG(RL_E_LOG_WARNING, "SetGamepadVibration() not implemented on target platform");
}

// Set mouse position XY
void RLSetMousePosition(int x, int y)
{
    CORE.Input.Mouse.currentPosition = (RLVector2){ (float)x, (float)y };
    CORE.Input.Mouse.previousPosition = CORE.Input.Mouse.currentPosition;
}

// Set mouse cursor
void RLSetMouseCursor(int cursor)
{
    TRACELOG(RL_E_LOG_WARNING, "SetMouseCursor() not implemented on target platform");
}

// Get physical key name
const char *RLGetKeyName(int key)
{
    TRACELOG(RL_E_LOG_WARNING, "GetKeyName() not implemented on target platform");
    return "";
}

// Register all input events
void RLPollInputEvents(void)
{
#if defined(SUPPORT_GESTURES_SYSTEM)
    // NOTE: Gestures update must be called every frame to reset gestures correctly
    // because ProcessGestureEvent() is just called on an event, not every frame
    UpdateGestures();
#endif

    // Reset keys/chars pressed registered
    CORE.Input.Keyboard.keyPressedQueueCount = 0;
    CORE.Input.Keyboard.charPressedQueueCount = 0;
    // Reset key repeats
    for (int i = 0; i < MAX_KEYBOARD_KEYS; i++) CORE.Input.Keyboard.keyRepeatInFrame[i] = 0;

    // Reset last gamepad button/axis registered state
    CORE.Input.Gamepad.lastButtonPressed = 0;       // GAMEPAD_BUTTON_UNKNOWN
    //CORE.Input.Gamepad.axisCount = 0;

    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (CORE.Input.Gamepad.ready[i])     // Check if gamepad is available
        {
            // Register previous gamepad states
            for (int k = 0; k < MAX_GAMEPAD_BUTTONS; k++)
                CORE.Input.Gamepad.previousButtonState[i][k] = CORE.Input.Gamepad.currentButtonState[i][k];
        }
    }

    // Register previous touch states
    for (int i = 0; i < MAX_TOUCH_POINTS; i++) CORE.Input.Touch.previousTouchState[i] = CORE.Input.Touch.currentTouchState[i];

    // Reset touch positions
    //for (int i = 0; i < MAX_TOUCH_POINTS; i++) CORE.Input.Touch.position[i] = (Vector2){ 0, 0 };

    // Register previous keys states
    // NOTE: Android supports up to 260 keys
    for (int i = 0; i < 260; i++)
    {
        CORE.Input.Keyboard.previousKeyState[i] = CORE.Input.Keyboard.currentKeyState[i];
        CORE.Input.Keyboard.keyRepeatInFrame[i] = 0;
    }

    // Android ALooper_pollOnce() variables
    int pollResult = 0;
    int pollEvents = 0;

    // Poll Events (registered events) until TIMEOUT is reached which indicates there are no events left to poll
    // NOTE: Activity is paused if not enabled (platform.appEnabled) and always run flag is not set (FLAG_WINDOW_ALWAYS_RUN)
    while ((pollResult = ALooper_pollOnce((platform.appEnabled || FLAG_IS_SET(CORE.Window.flags, RL_E_FLAG_WINDOW_ALWAYS_RUN))? 0 : -1, NULL, &pollEvents, ((void **)&platform.source)) > ALOOPER_POLL_TIMEOUT))
    {
        // Process this event
        if (platform.source != NULL) platform.source->process(platform.app, platform.source);

        // NOTE: Allow closing the window in case a configuration change happened
        // The android_main function should be allowed to return to its caller in order for the
        // Android OS to relaunch the activity
        if (platform.app->destroyRequested != 0)
        {
            CORE.Window.shouldClose = true;
        }
    }
}

//----------------------------------------------------------------------------------
// Module Internal Functions Definition
//----------------------------------------------------------------------------------

// Initialize platform: graphics, inputs and more
int InitPlatform(void)
{
    // Initialize display basic configuration
    //----------------------------------------------------------------------------
    CORE.Window.currentFbo.width = CORE.Window.screen.width;
    CORE.Window.currentFbo.height = CORE.Window.screen.height;

    // Set desired windows flags before initializing anything
    ANativeActivity_setWindowFlags(platform.app->activity, AWINDOW_FLAG_FULLSCREEN, 0);  //AWINDOW_FLAG_SCALED, AWINDOW_FLAG_DITHER

    int orientation = AConfiguration_getOrientation(platform.app->config);

    if (orientation == ACONFIGURATION_ORIENTATION_PORT) TRACELOG(RL_E_LOG_INFO, "ANDROID: Window orientation set as portrait");
    else if (orientation == ACONFIGURATION_ORIENTATION_LAND) TRACELOG(RL_E_LOG_INFO, "ANDROID: Window orientation set as landscape");

    // TODO: Automatic orientation doesn't seem to work
    if (CORE.Window.screen.width <= CORE.Window.screen.height)
    {
        AConfiguration_setOrientation(platform.app->config, ACONFIGURATION_ORIENTATION_PORT);
        TRACELOG(RL_E_LOG_WARNING, "ANDROID: Window orientation changed to portrait");
    }
    else
    {
        AConfiguration_setOrientation(platform.app->config, ACONFIGURATION_ORIENTATION_LAND);
        TRACELOG(RL_E_LOG_WARNING, "ANDROID: Window orientation changed to landscape");
    }

    //AConfiguration_getDensity(platform.app->config);
    //AConfiguration_getKeyboard(platform.app->config);
    //AConfiguration_getScreenSize(platform.app->config);
    //AConfiguration_getScreenLong(platform.app->config);

    // Set some default window flags
    FLAG_CLEAR(CORE.Window.flags, RL_E_FLAG_WINDOW_HIDDEN);       // false
    FLAG_CLEAR(CORE.Window.flags, RL_E_FLAG_WINDOW_MINIMIZED);    // false
    FLAG_SET(CORE.Window.flags, RL_E_FLAG_WINDOW_MAXIMIZED);      // true
    FLAG_CLEAR(CORE.Window.flags, RL_E_FLAG_WINDOW_UNFOCUSED);    // false
    //----------------------------------------------------------------------------

    // Initialize App command system
    // NOTE: On APP_CMD_INIT_WINDOW -> InitGraphicsDevice(), InitTimer(), LoadFontDefault()...
    //----------------------------------------------------------------------------
    platform.app->onAppCmd = AndroidCommandCallback;
    //----------------------------------------------------------------------------

    // Initialize input events system
    //----------------------------------------------------------------------------
    platform.app->onInputEvent = AndroidInputCallback;
    //----------------------------------------------------------------------------

    // Initialize storage system
    //----------------------------------------------------------------------------
    CORE.Storage.basePath = platform.app->activity->internalDataPath;   // Define base path for storage
    //----------------------------------------------------------------------------

    TRACELOG(RL_E_LOG_INFO, "PLATFORM: ANDROID: Initialized successfully");

    // Android ALooper_pollOnce() variables
    int pollResult = 0;
    int pollEvents = 0;

    // Wait for window to be initialized (display and context)
    while (!CORE.Window.ready)
    {
        // Process events until TIMEOUT is reached, which indicates no more events queued
        while ((pollResult = ALooper_pollOnce(0, NULL, &pollEvents, ((void **)&platform.source)) > ALOOPER_POLL_TIMEOUT))
        {
            // Process this event
            if (platform.source != NULL) platform.source->process(platform.app, platform.source);

            // NOTE: It's highly likely destroyRequested will never be non-zero at the start of the activity lifecycle
            //if (platform.app->destroyRequested != 0) CORE.Window.shouldClose = true;
        }
    }

    for (int i = 0; i < MAX_TOUCH_POINTS; i++) touchRaw.hoverPoints[i] = -1;

    return 0;
}

// Close platform
void ClosePlatform(void)
{
    // Close surface, context and display
    if (platform.device != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(platform.device, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (platform.surface != EGL_NO_SURFACE)
        {
            eglDestroySurface(platform.device, platform.surface);
            platform.surface = EGL_NO_SURFACE;
        }

        if (platform.context != EGL_NO_CONTEXT)
        {
            eglDestroyContext(platform.device, platform.context);
            platform.context = EGL_NO_CONTEXT;
        }

        eglTerminate(platform.device);
        platform.device = EGL_NO_DISPLAY;
    }

    // NOTE: Reset global state in case the activity is being relaunched
    if (platform.app->destroyRequested != 0)
    {
        CORE = (CoreData){ 0 };
        platform = (PlatformData){ 0 };
    }
}

// Initialize display device and framebuffer
// NOTE: width and height represent the screen (framebuffer) desired size, not actual display size
// If width or height are 0, default display size will be used for framebuffer size
// NOTE: returns false in case graphic device could not be created
static int InitGraphicsDevice(void)
{
    FLAG_SET(CORE.Window.flags, RL_E_FLAG_FULLSCREEN_MODE);

    EGLint samples = 0;
    EGLint sampleBuffer = 0;
    if (FLAG_IS_SET(CORE.Window.flags, RL_E_FLAG_MSAA_4X_HINT))
    {
        samples = 4;
        sampleBuffer = 1;
        TRACELOG(RL_E_LOG_INFO, "DISPLAY: Trying to enable MSAA x4");
    }

    const EGLint framebufferAttribs[] = {
        EGL_RENDERABLE_TYPE, (rlGetVersion() == RL_OPENGL_ES_30)? EGL_OPENGL_ES3_BIT : EGL_OPENGL_ES2_BIT, // Type of context support
        EGL_RED_SIZE, 8,            // RED color bit depth (alternative: 5)
        EGL_GREEN_SIZE, 8,          // GREEN color bit depth (alternative: 6)
        EGL_BLUE_SIZE, 8,           // BLUE color bit depth (alternative: 5)
        //EGL_TRANSPARENT_TYPE, EGL_NONE, // Request transparent framebuffer (EGL_TRANSPARENT_RGB does not work on RPI)
        EGL_DEPTH_SIZE, 24,         // Depth buffer size (Required to use Depth testing!)
        //EGL_STENCIL_SIZE, 8,      // Stencil buffer size
        EGL_SAMPLE_BUFFERS, sampleBuffer, // Activate MSAA
        EGL_SAMPLES, samples,       // 4x Antialiasing if activated (Free on MALI GPUs)
        EGL_NONE
    };

    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLint numConfigs = 0;

    // Get an EGL device connection
    platform.device = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    if (platform.device == EGL_NO_DISPLAY)
    {
        TRACELOG(RL_E_LOG_WARNING, "DISPLAY: Failed to initialize EGL device");
        return -1;
    }

    // Initialize the EGL device connection
    if (eglInitialize(platform.device, NULL, NULL) == EGL_FALSE)
    {
        // If all of the calls to eglInitialize returned EGL_FALSE then an error has occurred
        TRACELOG(RL_E_LOG_WARNING, "DISPLAY: Failed to initialize EGL device");
        return -1;
    }

    // Get an appropriate EGL framebuffer configuration
    eglChooseConfig(platform.device, framebufferAttribs, &platform.config, 1, &numConfigs);

    // Set rendering API
    eglBindAPI(EGL_OPENGL_ES_API);

    // Create an EGL rendering context
    platform.context = eglCreateContext(platform.device, platform.config, EGL_NO_CONTEXT, contextAttribs);
    if (platform.context == EGL_NO_CONTEXT)
    {
        TRACELOG(RL_E_LOG_WARNING, "DISPLAY: Failed to create EGL context");
        return -1;
    }

    // Create an EGL window surface
    //---------------------------------------------------------------------------------
    EGLint displayFormat = 0;

    // EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is guaranteed to be accepted by ANativeWindow_setBuffersGeometry()
    // As soon as an EGLConfig is picked, it's safe to reconfigure the ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID
    eglGetConfigAttrib(platform.device, platform.config, EGL_NATIVE_VISUAL_ID, &displayFormat);

    // At this point render size vs screen size needs to be managed
    // NOTE: This function use and modify global module variables:
    //  -> CORE.Window.screen.width/CORE.Window.screen.height
    //  -> CORE.Window.render.width/CORE.Window.render.height
    //  -> CORE.Window.screenScale
    SetupFramebuffer(CORE.Window.display.width, CORE.Window.display.height);

    ANativeWindow_setBuffersGeometry(platform.app->window, CORE.Window.render.width, CORE.Window.render.height, displayFormat);
    //ANativeWindow_setBuffersGeometry(platform.app->window, 0, 0, displayFormat);       // Force use of native display size

    platform.surface = eglCreateWindowSurface(platform.device, platform.config, platform.app->window, NULL);

    // There must be at least one frame displayed before the buffers are swapped
    //eglSwapInterval(platform.device, 1);

    if (eglMakeCurrent(platform.device, platform.surface, platform.surface, platform.context) == EGL_FALSE)
    {
        TRACELOG(RL_E_LOG_WARNING, "DISPLAY: Failed to attach EGL rendering context to EGL surface");
        return -1;
    }
    else
    {
        CORE.Window.render.width = CORE.Window.screen.width;
        CORE.Window.render.height = CORE.Window.screen.height;
        CORE.Window.currentFbo.width = CORE.Window.render.width;
        CORE.Window.currentFbo.height = CORE.Window.render.height;

        TRACELOG(RL_E_LOG_INFO, "DISPLAY: Device initialized successfully");
        TRACELOG(RL_E_LOG_INFO, "    > Display size: %i x %i", CORE.Window.display.width, CORE.Window.display.height);
        TRACELOG(RL_E_LOG_INFO, "    > Screen size:  %i x %i", CORE.Window.screen.width, CORE.Window.screen.height);
        TRACELOG(RL_E_LOG_INFO, "    > Render size:  %i x %i", CORE.Window.render.width, CORE.Window.render.height);
        TRACELOG(RL_E_LOG_INFO, "    > Viewport offsets: %i, %i", CORE.Window.renderOffset.x, CORE.Window.renderOffset.y);
    }

    // Load OpenGL extensions
    // NOTE: GL procedures address loader is required to load extensions
    rlLoadExtensions(eglGetProcAddress);

    CORE.Window.ready = true;

    if (FLAG_IS_SET(CORE.Window.flags, RL_E_FLAG_WINDOW_MINIMIZED)) RLMinimizeWindow();

    return 0;
}

// ANDROID: Process activity lifecycle commands
static void AndroidCommandCallback(struct android_app *app, int32_t cmd)
{
    switch (cmd)
    {
        case APP_CMD_START:
        {
            //rendering = true;
        } break;
        case APP_CMD_RESUME: break;
        case APP_CMD_INIT_WINDOW:
        {
            if (app->window != NULL)
            {
                if (platform.contextRebindRequired)
                {
                    // Reset screen scaling to full display size
                    EGLint displayFormat = 0;
                    eglGetConfigAttrib(platform.device, platform.config, EGL_NATIVE_VISUAL_ID, &displayFormat);

                    // Adding renderOffset here feels rather hackish, but the viewport scaling is wrong after the
                    // context rebinding if the screen is scaled unless offsets are added. There's probably a more
                    // appropriate way to fix this
                    ANativeWindow_setBuffersGeometry(app->window,
                        CORE.Window.render.width + CORE.Window.renderOffset.x,
                        CORE.Window.render.height + CORE.Window.renderOffset.y,
                        displayFormat);

                    // Recreate display surface and re-attach OpenGL context
                    platform.surface = eglCreateWindowSurface(platform.device, platform.config, app->window, NULL);
                    eglMakeCurrent(platform.device, platform.surface, platform.surface, platform.context);

                    platform.contextRebindRequired = false;
                }
                else
                {
                    CORE.Window.display.width = ANativeWindow_getWidth(platform.app->window);
                    CORE.Window.display.height = ANativeWindow_getHeight(platform.app->window);

                    // Initialize graphics device (display device and OpenGL context)
                    InitGraphicsDevice();

                    // Initialize OpenGL context (states and resources)
                    // NOTE: CORE.Window.currentFbo.width and CORE.Window.currentFbo.height not used, just stored as globals in rlgl
                    rlglInit(CORE.Window.currentFbo.width, CORE.Window.currentFbo.height);

                    // Setup default viewport
                    // NOTE: It updated CORE.Window.render.width and CORE.Window.render.height
                    SetupViewport(CORE.Window.currentFbo.width, CORE.Window.currentFbo.height);

                    // Initialize hi-res timer
                    InitTimer();

                #if defined(SUPPORT_MODULE_RTEXT) && defined(SUPPORT_DEFAULT_FONT)
                    // Load default font
                    // WARNING: External function: Module required: rtext
                    LoadFontDefault();
                    #if defined(SUPPORT_MODULE_RSHAPES)
                    // Set font white rectangle for shapes drawing, so shapes and text can be batched together
                    // WARNING: rshapes module is required, if not available, default internal white rectangle is used
                    RLRectangle rec = RLGetFontDefault().recs[95];
                    if (FLAG_IS_SET(CORE.Window.flags, RL_E_FLAG_MSAA_4X_HINT))
                    {
                        // NOTE: Trying to maxime rec padding to avoid pixel bleeding on MSAA filtering
                        RLSetShapesTexture(RLGetFontDefault().texture, (RLRectangle){ rec.x + 2, rec.y + 2, 1, 1 });
                    }
                    else
                    {
                        // NOTE: Setting up a 1px padding on char rectangle to avoid pixel bleeding
                        RLSetShapesTexture(RLGetFontDefault().texture, (RLRectangle){ rec.x + 1, rec.y + 1, rec.width - 2, rec.height - 2 });
                    }
                    #endif
                #else
                    #if defined(SUPPORT_MODULE_RSHAPES)
                    // Set default texture and rectangle to be used for shapes drawing
                    // NOTE: rlgl default texture is a 1x1 pixel UNCOMPRESSED_R8G8B8A8
                    RLTexture2D texture = { rlGetTextureIdDefault(), 1, 1, 1, RL_E_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
                    RLSetShapesTexture(texture, (RLRectangle){ 0.0f, 0.0f, 1.0f, 1.0f });    // WARNING: Module required: rshapes
                    #endif
                #endif

                    // Initialize random seed
                    RLSetRandomSeed((unsigned int)time(NULL));
                }
            }
        } break;
        case APP_CMD_GAINED_FOCUS:
        {
            platform.appEnabled = true;
            FLAG_CLEAR(CORE.Window.flags, RL_E_FLAG_WINDOW_UNFOCUSED);
            //ResumeMusicStream();
        } break;
        case APP_CMD_PAUSE: break;
        case APP_CMD_LOST_FOCUS:
        {
            platform.appEnabled = false;
            FLAG_SET(CORE.Window.flags, RL_E_FLAG_WINDOW_UNFOCUSED);
            //PauseMusicStream();
        } break;
        case APP_CMD_TERM_WINDOW:
        {
            // Detach OpenGL context and destroy display surface
            // NOTE 1: This case is used when the user exits the app without closing it, context is detached to ensure everything is recoverable upon resuming
            // NOTE 2: Detaching context before destroying display surface avoids losing our resources (textures, shaders, VBOs...)
            // NOTE 3: In some cases (too many context loaded), OS could unload context automatically... :(
            if (platform.device != EGL_NO_DISPLAY)
            {
                eglMakeCurrent(platform.device, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

                if (platform.surface != EGL_NO_SURFACE)
                {
                    eglDestroySurface(platform.device, platform.surface);
                    platform.surface = EGL_NO_SURFACE;
                }

                platform.contextRebindRequired = true;
            }
            // If 'platform.device' is already set to 'EGL_NO_DISPLAY'
            // this means that the user has already called 'CloseWindow()'

        } break;
        case APP_CMD_SAVE_STATE: break;
        case APP_CMD_STOP: break;
        case APP_CMD_DESTROY: break;
        case APP_CMD_CONFIG_CHANGED:
        {
            //AConfiguration_fromAssetManager(platform.app->config, platform.app->activity->assetManager);
            //print_cur_config(platform.app);

            // Check screen orientation here!
        } break;
        default: break;
    }
}

// ANDROID: Map Android gamepad button to raylib gamepad button
static RLGamepadButton AndroidTranslateGamepadButton(int button)
{
    switch (button)
    {
        case AKEYCODE_BUTTON_A: return RL_E_GAMEPAD_BUTTON_RIGHT_FACE_DOWN;
        case AKEYCODE_BUTTON_B: return RL_E_GAMEPAD_BUTTON_RIGHT_FACE_RIGHT;
        case AKEYCODE_BUTTON_X: return RL_E_GAMEPAD_BUTTON_RIGHT_FACE_LEFT;
        case AKEYCODE_BUTTON_Y: return RL_E_GAMEPAD_BUTTON_RIGHT_FACE_UP;
        case AKEYCODE_BUTTON_L1: return RL_E_GAMEPAD_BUTTON_LEFT_TRIGGER_1;
        case AKEYCODE_BUTTON_R1: return RL_E_GAMEPAD_BUTTON_RIGHT_TRIGGER_1;
        case AKEYCODE_BUTTON_L2: return RL_E_GAMEPAD_BUTTON_LEFT_TRIGGER_2;
        case AKEYCODE_BUTTON_R2: return RL_E_GAMEPAD_BUTTON_RIGHT_TRIGGER_2;
        case AKEYCODE_BUTTON_THUMBL: return RL_E_GAMEPAD_BUTTON_LEFT_THUMB;
        case AKEYCODE_BUTTON_THUMBR: return RL_E_GAMEPAD_BUTTON_RIGHT_THUMB;
        case AKEYCODE_BUTTON_START: return RL_E_GAMEPAD_BUTTON_MIDDLE_RIGHT;
        case AKEYCODE_BUTTON_SELECT: return RL_E_GAMEPAD_BUTTON_MIDDLE_LEFT;
        case AKEYCODE_BUTTON_MODE: return RL_E_GAMEPAD_BUTTON_MIDDLE;
        // On some (most?) gamepads dpad events are reported as axis motion instead
        case AKEYCODE_DPAD_DOWN: return RL_E_GAMEPAD_BUTTON_LEFT_FACE_DOWN;
        case AKEYCODE_DPAD_RIGHT: return RL_E_GAMEPAD_BUTTON_LEFT_FACE_RIGHT;
        case AKEYCODE_DPAD_LEFT: return RL_E_GAMEPAD_BUTTON_LEFT_FACE_LEFT;
        case AKEYCODE_DPAD_UP: return RL_E_GAMEPAD_BUTTON_LEFT_FACE_UP;
        default: return RL_E_GAMEPAD_BUTTON_UNKNOWN;
    }
}

// ANDROID: Get input events
static int32_t AndroidInputCallback(struct android_app *app, AInputEvent *event)
{
    // If additional inputs are required check:
    // REF: https://developer.android.com/ndk/reference/group/input
    // REF: https://developer.android.com/training/game-controllers/controller-input

    int type = AInputEvent_getType(event);
    int source = AInputEvent_getSource(event);

    if (type == AINPUT_EVENT_TYPE_MOTION)
    {
        if (FLAG_IS_SET(source, AINPUT_SOURCE_JOYSTICK) ||
            FLAG_IS_SET(source, AINPUT_SOURCE_GAMEPAD))
        {
            // Assuming a single gamepad, "detected" on its input event
            CORE.Input.Gamepad.ready[0] = true;

            CORE.Input.Gamepad.axisState[0][RL_E_GAMEPAD_AXIS_LEFT_X] = AMotionEvent_getAxisValue(
                    event, AMOTION_EVENT_AXIS_X, 0);
            CORE.Input.Gamepad.axisState[0][RL_E_GAMEPAD_AXIS_LEFT_Y] = AMotionEvent_getAxisValue(
                    event, AMOTION_EVENT_AXIS_Y, 0);
            CORE.Input.Gamepad.axisState[0][RL_E_GAMEPAD_AXIS_RIGHT_X] = AMotionEvent_getAxisValue(
                    event, AMOTION_EVENT_AXIS_Z, 0);
            CORE.Input.Gamepad.axisState[0][RL_E_GAMEPAD_AXIS_RIGHT_Y] = AMotionEvent_getAxisValue(
                    event, AMOTION_EVENT_AXIS_RZ, 0);
            CORE.Input.Gamepad.axisState[0][RL_E_GAMEPAD_AXIS_LEFT_TRIGGER] = AMotionEvent_getAxisValue(
                    event, AMOTION_EVENT_AXIS_BRAKE, 0)*2.0f - 1.0f;
            CORE.Input.Gamepad.axisState[0][RL_E_GAMEPAD_AXIS_RIGHT_TRIGGER] = AMotionEvent_getAxisValue(
                    event, AMOTION_EVENT_AXIS_GAS, 0)*2.0f - 1.0f;

            // dpad is reported as an axis on android
            float dpadX = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HAT_X, 0);
            float dpadY = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HAT_Y, 0);

            if (dpadX == 1.0f)
            {
                CORE.Input.Gamepad.currentButtonState[0][RL_E_GAMEPAD_BUTTON_LEFT_FACE_RIGHT] = 1;
                CORE.Input.Gamepad.currentButtonState[0][RL_E_GAMEPAD_BUTTON_LEFT_FACE_LEFT] = 0;
            }
            else if (dpadX == -1.0f)
            {
                CORE.Input.Gamepad.currentButtonState[0][RL_E_GAMEPAD_BUTTON_LEFT_FACE_RIGHT] = 0;
                CORE.Input.Gamepad.currentButtonState[0][RL_E_GAMEPAD_BUTTON_LEFT_FACE_LEFT] = 1;
            }
            else
            {
                CORE.Input.Gamepad.currentButtonState[0][RL_E_GAMEPAD_BUTTON_LEFT_FACE_RIGHT] = 0;
                CORE.Input.Gamepad.currentButtonState[0][RL_E_GAMEPAD_BUTTON_LEFT_FACE_LEFT] = 0;
            }

            if (dpadY == 1.0f)
            {
                CORE.Input.Gamepad.currentButtonState[0][RL_E_GAMEPAD_BUTTON_LEFT_FACE_DOWN] = 1;
                CORE.Input.Gamepad.currentButtonState[0][RL_E_GAMEPAD_BUTTON_LEFT_FACE_UP] = 0;
            }
            else if (dpadY == -1.0f)
            {
                CORE.Input.Gamepad.currentButtonState[0][RL_E_GAMEPAD_BUTTON_LEFT_FACE_DOWN] = 0;
                CORE.Input.Gamepad.currentButtonState[0][RL_E_GAMEPAD_BUTTON_LEFT_FACE_UP] = 1;
            }
            else
            {
                CORE.Input.Gamepad.currentButtonState[0][RL_E_GAMEPAD_BUTTON_LEFT_FACE_DOWN] = 0;
                CORE.Input.Gamepad.currentButtonState[0][RL_E_GAMEPAD_BUTTON_LEFT_FACE_UP] = 0;
            }

            return 1; // Handled gamepad axis motion
        }
    }
    else if (type == AINPUT_EVENT_TYPE_KEY)
    {
        int32_t keycode = AKeyEvent_getKeyCode(event);
        //int32_t AKeyEvent_getMetaState(event);

        // Handle gamepad button presses and releases
        // NOTE: Skip gamepad handling if this is a keyboard event, as some devices
        // report both AINPUT_SOURCE_KEYBOARD and AINPUT_SOURCE_GAMEPAD flags
        if ((FLAG_IS_SET(source, AINPUT_SOURCE_JOYSTICK) ||
             FLAG_IS_SET(source, AINPUT_SOURCE_GAMEPAD)) &&
            !FLAG_IS_SET(source, AINPUT_SOURCE_KEYBOARD))
        {
            // Assuming a single gamepad, "detected" on its input event
            CORE.Input.Gamepad.ready[0] = true;

            RLGamepadButton button = AndroidTranslateGamepadButton(keycode);

            if (button == RL_E_GAMEPAD_BUTTON_UNKNOWN) return 1;

            if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_DOWN)
            {
                CORE.Input.Gamepad.currentButtonState[0][button] = 1;
            }
            else CORE.Input.Gamepad.currentButtonState[0][button] = 0;  // Key up

            return 1; // Handled gamepad button
        }

        RLKeyboardKey key = ((keycode > 0) && (keycode < KEYCODE_MAP_SIZE))? mapKeycode[keycode] : RL_E_KEY_NULL;
        if (key != RL_E_KEY_NULL)
        {
            // Save current key and its state
            // NOTE: Android key action is 0 for down and 1 for up
            if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_DOWN)
            {
                CORE.Input.Keyboard.currentKeyState[key] = 1;   // Key down

                CORE.Input.Keyboard.keyPressedQueue[CORE.Input.Keyboard.keyPressedQueueCount] = key;
                CORE.Input.Keyboard.keyPressedQueueCount++;
            }
            else if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_MULTIPLE) CORE.Input.Keyboard.keyRepeatInFrame[key] = 1;
            else CORE.Input.Keyboard.currentKeyState[key] = 0;  // Key up
        }

        if (keycode == AKEYCODE_POWER)
        {
            // Let the OS handle input to avoid app stuck. Behaviour: CMD_PAUSE -> CMD_SAVE_STATE -> CMD_STOP -> CMD_CONFIG_CHANGED -> CMD_LOST_FOCUS
            // Resuming Behaviour: CMD_START -> CMD_RESUME -> CMD_CONFIG_CHANGED -> CMD_CONFIG_CHANGED -> CMD_GAINED_FOCUS
            // It seems like locking mobile, screen size (CMD_CONFIG_CHANGED) is affected
            // NOTE: AndroidManifest.xml must have <activity android:configChanges="orientation|keyboardHidden|screenSize" >
            // Before that change, activity was calling CMD_TERM_WINDOW and CMD_DESTROY when locking mobile, so that was not a normal behaviour
            return 0;
        }
        else if ((keycode == AKEYCODE_BACK) || (keycode == AKEYCODE_MENU))
        {
            // Eat BACK_BUTTON and AKEYCODE_MENU, just do nothing... and don't let to be handled by OS!
            return 1;
        }
        else if ((keycode == AKEYCODE_VOLUME_UP) || (keycode == AKEYCODE_VOLUME_DOWN))
        {
            // Set default OS behaviour
            return 0;
        }

        return 0;
    }

    // Register touch points count
    touchRaw.pointCount = AMotionEvent_getPointerCount(event);

    for (int i = 0; (i < touchRaw.pointCount) && (i < MAX_TOUCH_POINTS); i++)
    {
        // Register touch points id
        touchRaw.pointId[i] = AMotionEvent_getPointerId(event, i);

        // Register touch points position
        touchRaw.position[i] = (RLVector2){ AMotionEvent_getX(event, i), AMotionEvent_getY(event, i) };

        // Normalize CORE.Input.Touch.position[i] for CORE.Window.screen.width and CORE.Window.screen.height
        float widthRatio = (float)(CORE.Window.screen.width + CORE.Window.renderOffset.x)/(float)CORE.Window.display.width;
        float heightRatio = (float)(CORE.Window.screen.height + CORE.Window.renderOffset.y)/(float)CORE.Window.display.height;
        touchRaw.position[i].x = touchRaw.position[i].x*widthRatio - (float)CORE.Window.renderOffset.x/2;
        touchRaw.position[i].y = touchRaw.position[i].y*heightRatio - (float)CORE.Window.renderOffset.y/2;
    }

    int32_t action = AMotionEvent_getAction(event);
    unsigned int flags = action & AMOTION_EVENT_ACTION_MASK;
    int32_t pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

    if (flags == AMOTION_EVENT_ACTION_HOVER_ENTER)
    {
        // The new pointer is hover
        // So add it to hoverPoints
        for (int i = 0; i < MAX_TOUCH_POINTS; i++)
        {
            if (touchRaw.hoverPoints[i] == -1)
            {
                touchRaw.hoverPoints[i] = touchRaw.pointId[pointerIndex];
                break;
            }
        }
    }

#if defined(SUPPORT_GESTURES_SYSTEM)
    RLGestureEvent gestureEvent = { 0 };

    gestureEvent.pointCount = 0;

    // Register touch actions
    if (flags == AMOTION_EVENT_ACTION_DOWN) gestureEvent.touchAction = RL_E_TOUCH_ACTION_DOWN;
    else if (flags == AMOTION_EVENT_ACTION_UP) gestureEvent.touchAction = RL_E_TOUCH_ACTION_UP;
    else if (flags == AMOTION_EVENT_ACTION_MOVE) gestureEvent.touchAction = RL_E_TOUCH_ACTION_MOVE;
    else if (flags == AMOTION_EVENT_ACTION_CANCEL) gestureEvent.touchAction = RL_E_TOUCH_ACTION_CANCEL;

    for (int i = 0; (i < touchRaw.pointCount) && (i < MAX_TOUCH_POINTS); i++)
    {
        // If the touchPoint is hover, Ignore it
        bool hover = false;
        for (int j = 0; j < MAX_TOUCH_POINTS; j++)
        {
            // Check if the touchPoint is in hoverPointers
            if (touchRaw.hoverPoints[j] == touchRaw.pointId[i])
            {
                hover = true;
                break;
            }
        }
        if (hover) continue;

        gestureEvent.pointId[gestureEvent.pointCount] = touchRaw.pointId[i];
        gestureEvent.position[gestureEvent.pointCount] = touchRaw.position[i];
        gestureEvent.position[gestureEvent.pointCount].x /= (float)RLGetScreenWidth();
        gestureEvent.position[gestureEvent.pointCount].y /= (float)RLGetScreenHeight();
        gestureEvent.pointCount++;
    }

    // Gesture data is sent to gestures system for processing
    ProcessGestureEvent(gestureEvent);
#endif

    if (flags == AMOTION_EVENT_ACTION_HOVER_EXIT)
    {
        // Hover exited. So, remove it from hoverPoints
        for (int i = 0; i < MAX_TOUCH_POINTS; i++)
        {
            if (touchRaw.hoverPoints[i] == touchRaw.pointId[pointerIndex])
            {
                touchRaw.hoverPoints[i] = -1;
                break;
            }
        }
    }

    if ((flags == AMOTION_EVENT_ACTION_POINTER_UP) || (flags == AMOTION_EVENT_ACTION_UP))
    {
        // One of the touchpoints is released, remove it from touch point arrays
        for (int i = pointerIndex; (i < touchRaw.pointCount - 1) && (i < MAX_TOUCH_POINTS - 1); i++)
        {
            touchRaw.pointId[i] = touchRaw.pointId[i+1];
            touchRaw.position[i] = touchRaw.position[i+1];
        }
        touchRaw.pointCount--;
    }

    CORE.Input.Touch.pointCount = 0;
    for (int i = 0; (i < touchRaw.pointCount) && (i < MAX_TOUCH_POINTS); i++)
    {
        // If the touchPoint is hover, Ignore it
        bool hover = false;
        for (int j = 0; j < MAX_TOUCH_POINTS; j++)
        {
            // Check if the touchPoint is in hoverPointers
            if (touchRaw.hoverPoints[j] == touchRaw.pointId[i])
            {
                hover = true;
                break;
            }
        }
        if (hover) continue;

        CORE.Input.Touch.pointId[CORE.Input.Touch.pointCount] = touchRaw.pointId[i];
        CORE.Input.Touch.position[CORE.Input.Touch.pointCount] = touchRaw.position[i];
        CORE.Input.Touch.pointCount++;
    }

    // When all touchpoints are tapped and released really quickly, this event is generated
    if (flags == AMOTION_EVENT_ACTION_CANCEL) CORE.Input.Touch.pointCount = 0;

    if (CORE.Input.Touch.pointCount > 0) CORE.Input.Touch.currentTouchState[RL_E_MOUSE_BUTTON_LEFT] = 1;
    else CORE.Input.Touch.currentTouchState[RL_E_MOUSE_BUTTON_LEFT] = 0;

    // Stores the previous position of touch[0] only while it's active to calculate the delta
    if (flags == AMOTION_EVENT_ACTION_MOVE) CORE.Input.Mouse.previousPosition = CORE.Input.Mouse.currentPosition;
    else CORE.Input.Mouse.previousPosition = CORE.Input.Touch.position[0];

    // Map touch[0] as mouse input for convenience
    CORE.Input.Mouse.currentPosition = CORE.Input.Touch.position[0];
    CORE.Input.Mouse.currentWheelMove = (RLVector2){ 0.0f, 0.0f };

    return 0;
}

// Compute framebuffer size relative to screen size and display size
// NOTE: Global variables CORE.Window.render.width/CORE.Window.render.height and CORE.Window.renderOffset.x/CORE.Window.renderOffset.y can be modified
static void SetupFramebuffer(int width, int height)
{
    // Calculate CORE.Window.render.width and CORE.Window.render.height, having the display size (input params) and the desired screen size (global var)
    if ((CORE.Window.screen.width > CORE.Window.display.width) || (CORE.Window.screen.height > CORE.Window.display.height))
    {
        TRACELOG(RL_E_LOG_WARNING, "DISPLAY: Downscaling required: Screen size (%ix%i) is bigger than display size (%ix%i)", CORE.Window.screen.width, CORE.Window.screen.height, CORE.Window.display.width, CORE.Window.display.height);

        // Downscaling to fit display with border-bars
        float widthRatio = (float)CORE.Window.display.width/(float)CORE.Window.screen.width;
        float heightRatio = (float)CORE.Window.display.height/(float)CORE.Window.screen.height;

        if (widthRatio <= heightRatio)
        {
            CORE.Window.render.width = CORE.Window.display.width;
            CORE.Window.render.height = (int)round((float)CORE.Window.screen.height*widthRatio);
            CORE.Window.renderOffset.x = 0;
            CORE.Window.renderOffset.y = (CORE.Window.display.height - CORE.Window.render.height);
        }
        else
        {
            CORE.Window.render.width = (int)round((float)CORE.Window.screen.width*heightRatio);
            CORE.Window.render.height = CORE.Window.display.height;
            CORE.Window.renderOffset.x = (CORE.Window.display.width - CORE.Window.render.width);
            CORE.Window.renderOffset.y = 0;
        }

        // Screen scaling required
        float scaleRatio = (float)CORE.Window.render.width/(float)CORE.Window.screen.width;
        CORE.Window.screenScale = RLMatrixScale(scaleRatio, scaleRatio, 1.0f);

        // NOTE: Rendering to full display resolution
        // Above parameters need to be calculate for downscale matrix and offsets
        CORE.Window.render.width = CORE.Window.display.width;
        CORE.Window.render.height = CORE.Window.display.height;

        TRACELOG(RL_E_LOG_WARNING, "DISPLAY: Downscale matrix generated, content will be rendered at (%ix%i)", CORE.Window.render.width, CORE.Window.render.height);
    }
    else if ((CORE.Window.screen.width < CORE.Window.display.width) || (CORE.Window.screen.height < CORE.Window.display.height))
    {
        // Required screen size is smaller than display size
        TRACELOG(RL_E_LOG_INFO, "DISPLAY: Upscaling required: Screen size (%ix%i) smaller than display size (%ix%i)", CORE.Window.screen.width, CORE.Window.screen.height, CORE.Window.display.width, CORE.Window.display.height);

        if ((CORE.Window.screen.width == 0) || (CORE.Window.screen.height == 0))
        {
            CORE.Window.screen.width = CORE.Window.display.width;
            CORE.Window.screen.height = CORE.Window.display.height;
        }

        // Upscaling to fit display with border-bars
        float displayRatio = (float)CORE.Window.display.width/(float)CORE.Window.display.height;
        float screenRatio = (float)CORE.Window.screen.width/(float)CORE.Window.screen.height;

        if (displayRatio <= screenRatio)
        {
            CORE.Window.render.width = CORE.Window.screen.width;
            CORE.Window.render.height = (int)round((float)CORE.Window.screen.width/displayRatio);
            CORE.Window.renderOffset.x = 0;
            CORE.Window.renderOffset.y = (CORE.Window.render.height - CORE.Window.screen.height);
        }
        else
        {
            CORE.Window.render.width = (int)round((float)CORE.Window.screen.height*displayRatio);
            CORE.Window.render.height = CORE.Window.screen.height;
            CORE.Window.renderOffset.x = (CORE.Window.render.width - CORE.Window.screen.width);
            CORE.Window.renderOffset.y = 0;
        }
    }
    else
    {
        CORE.Window.render.width = CORE.Window.screen.width;
        CORE.Window.render.height = CORE.Window.screen.height;
        CORE.Window.renderOffset.x = 0;
        CORE.Window.renderOffset.y = 0;
    }
}

// Replacement for fopen()
// REF: https://developer.android.com/ndk/reference/group/asset
FILE *android_fopen(const char *fileName, const char *mode)
{
    FILE *file = NULL;
    
    if (mode[0] == 'w')
    {
        // NOTE: fopen() is mapped to android_fopen() that only grants read access to
        // assets directory through AAssetManager but it could be required to write data
        // using the standard stdio FILE access functions
        // REF: https://stackoverflow.com/questions/11294487/android-writing-saving-files-from-native-code-only
        #undef fopen
        file = fopen(RLTextFormat("%s/%s", platform.app->activity->internalDataPath, fileName), mode);
        #define fopen(name, mode) android_fopen(name, mode)
    }
    else
    {
        // NOTE: AAsset provides access to read-only asset
        AAsset *asset = AAssetManager_open(platform.app->activity->assetManager, fileName, AASSET_MODE_UNKNOWN);

        if (asset != NULL)
        {
            // Get pointer to file in the assets
            file = funopen(asset, android_read, android_write, android_seek, android_close);
        }
        else
        {
            #undef fopen
            // Just do a regular open if file is not found in the assets
            file = fopen(RLTextFormat("%s/%s", platform.app->activity->internalDataPath, fileName), mode);
            if (file == NULL) file = fopen(fileName, mode);
            #define fopen(name, mode) android_fopen(name, mode)
        }
    }
    
    return file;
}

static int android_read(void *cookie, char *data, int dataSize)
{
    return AAsset_read((AAsset *)cookie, data, dataSize);
}

static int android_write(void *cookie, const char *data, int dataSize)
{
    TRACELOG(RL_E_LOG_WARNING, "ANDROID: Failed to provide write access to APK");

    return EACCES;
}

static fpos_t android_seek(void *cookie, fpos_t offset, int whence)
{
    return AAsset_seek((AAsset *)cookie, offset, whence);
}

static int android_close(void *cookie)
{
    AAsset_close((AAsset *)cookie);
    return 0;
}

// EOF
