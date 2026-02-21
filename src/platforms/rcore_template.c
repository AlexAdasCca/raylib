/**********************************************************************************************
*
*   rcore_<platform> template - Functions to manage window, graphics device and inputs
*
*   PLATFORM: <PLATFORM>
*       - TODO: Define the target platform for the core
*
*   LIMITATIONS:
*       - Limitation 01
*       - Limitation 02
*
*   POSSIBLE IMPROVEMENTS:
*       - Improvement 01
*       - Improvement 02
*
*   ADDITIONAL NOTES:
*       - TRACELOG() function is located in raylib [utils] module
*
*   CONFIGURATION:
*       #define RCORE_PLATFORM_CUSTOM_FLAG
*           Custom flag for rcore on target platform -not used-
*
*   DEPENDENCIES:
*       - <platform-specific SDK dependency>
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

// TODO: Include the platform specific libraries

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct {
    // TODO: Define the platform specific variables required

} PlatformData;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
extern CoreData CORE;                   // Global CORE state context

static PlatformData platform = { 0 };   // Platform specific data

//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------
int InitPlatform(void);          // Initialize platform (graphics, inputs and more)
bool InitGraphicsDevice(void);   // Initialize graphics device

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
// NOTE: Functions declaration is provided by raylib.h

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
    TRACELOG(RL_E_LOG_WARNING, "SetWindowState() not available on target platform");
}

// Clear window configuration state flags
void RLClearWindowState(unsigned int flags)
{
    TRACELOG(RL_E_LOG_WARNING, "ClearWindowState() not available on target platform");
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
    TRACELOG(RL_E_LOG_WARNING, "GetCurrentMonitor() not implemented on target platform");
    return 0;
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
int RLGetMonitorPhysicalWidth(int monitor)
{
    TRACELOG(RL_E_LOG_WARNING, "GetMonitorPhysicalWidth() not implemented on target platform");
    return 0;
}

// Get selected monitor physical height in millimetres
int RLGetMonitorPhysicalHeight(int monitor)
{
    TRACELOG(RL_E_LOG_WARNING, "GetMonitorPhysicalHeight() not implemented on target platform");
    return 0;
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
    TRACELOG(RL_E_LOG_WARNING, "GetWindowScaleDPI() not implemented on target platform");
    return (RLVector2){ 1.0f, 1.0f };
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

    CORE.Input.Mouse.cursorHidden = false;
}

// Disables cursor (lock cursor)
void RLDisableCursor(void)
{
    // Set cursor position in the middle
    RLSetMousePosition(CORE.Window.screen.width/2, CORE.Window.screen.height/2);

    CORE.Input.Mouse.cursorHidden = true;
}

// Swap back buffer with front buffer (screen drawing)
void RLSwapScreenBuffer(void)
{
    eglSwapBuffers(platform.device, platform.surface);
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
// NOTE: This function is only safe to use if you control the URL given.
// A user could craft a malicious string performing another action.
// Only call this function yourself not with user input or make sure to check the string yourself.
// Ref: https://github.com/raysan5/raylib/issues/686
void RLOpenURL(const char *url)
{
    // Security check to (partially) avoid malicious code on target platform
    if (strchr(url, '\'') != NULL) TRACELOG(RL_E_LOG_WARNING, "SYSTEM: Provided URL could be potentially malicious, avoid [\'] character");
    else
    {
        // TODO: Load url using default browser
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

// Get physical key name.
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
    CORE.Input.Gamepad.lastButtonPressed = 0; // GAMEPAD_BUTTON_UNKNOWN
    //CORE.Input.Gamepad.axisCount = 0;

    // Register previous touch states
    for (int i = 0; i < MAX_TOUCH_POINTS; i++) CORE.Input.Touch.previousTouchState[i] = CORE.Input.Touch.currentTouchState[i];

    // Reset touch positions
    // TODO: It resets on target platform the mouse position and not filled again until a move-event,
    // so, if mouse is not moved it returns a (0, 0) position... this behaviour should be reviewed!
    //for (int i = 0; i < MAX_TOUCH_POINTS; i++) CORE.Input.Touch.position[i] = (Vector2){ 0, 0 };

    // Register previous keys states
    // NOTE: Android supports up to 260 keys
    for (int i = 0; i < 260; i++)
    {
        CORE.Input.Keyboard.previousKeyState[i] = CORE.Input.Keyboard.currentKeyState[i];
        CORE.Input.Keyboard.keyRepeatInFrame[i] = 0;
    }

    // TODO: Poll input events for current platform
}

//----------------------------------------------------------------------------------
// Module Internal Functions Definition
//----------------------------------------------------------------------------------

// Initialize platform: graphics, inputs and more
int InitPlatform(void)
{
    // TODO: Initialize graphic device: display/window
    // It usually requires setting up the platform display system configuration
    // and connexion with the GPU through some system graphic API
    // raylib uses OpenGL so, platform should create that kind of connection
    // Below example illustrates that process using EGL library
    //----------------------------------------------------------------------------
    FLAG_SET(CORE.Window.flags, RL_E_FLAG_FULLSCREEN_MODE);

    if (FLAG_IS_SET(CORE.Window.flags, RL_E_FLAG_MSAA_4X_HINT))
    {
        // TODO: Enable MSAA
        
        TRACELOG(RL_E_LOG_INFO, "DISPLAY: Trying to enable MSAA x4");
    }

    // TODO: Init display and graphic device

    // TODO: Check display, device and context activation
    bool result = true;
    if (result)
    {
        CORE.Window.ready = true;

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
    else
    {
        TRACELOG(RL_E_LOG_FATAL, "PLATFORM: Failed to initialize graphics device");
        return -1;
    }
    //----------------------------------------------------------------------------

    // If everything work as expected, we can continue
    CORE.Window.render.width = CORE.Window.screen.width;
    CORE.Window.render.height = CORE.Window.screen.height;
    CORE.Window.currentFbo.width = CORE.Window.render.width;
    CORE.Window.currentFbo.height = CORE.Window.render.height;

    TRACELOG(RL_E_LOG_INFO, "DISPLAY: Device initialized successfully");
    TRACELOG(RL_E_LOG_INFO, "    > Display size: %i x %i", CORE.Window.display.width, CORE.Window.display.height);
    TRACELOG(RL_E_LOG_INFO, "    > Screen size:  %i x %i", CORE.Window.screen.width, CORE.Window.screen.height);
    TRACELOG(RL_E_LOG_INFO, "    > Render size:  %i x %i", CORE.Window.render.width, CORE.Window.render.height);
    TRACELOG(RL_E_LOG_INFO, "    > Viewport offsets: %i, %i", CORE.Window.renderOffset.x, CORE.Window.renderOffset.y);

    // TODO: Load OpenGL extensions
    // NOTE: GL procedures address loader is required to load extensions
    //----------------------------------------------------------------------------
    rlLoadExtensions(eglGetProcAddress);
    //----------------------------------------------------------------------------

    // TODO: Initialize input events system
    // It could imply keyboard, mouse, gamepad, touch...
    // Depending on the platform libraries/SDK it could use a callback mechanism
    // For system events and inputs evens polling on a per-frame basis, use PollInputEvents()
    //----------------------------------------------------------------------------
    // ...
    //----------------------------------------------------------------------------

    // TODO: Initialize timing system
    //----------------------------------------------------------------------------
    InitTimer();
    //----------------------------------------------------------------------------

    // TODO: Initialize storage system
    //----------------------------------------------------------------------------
    CORE.Storage.basePath = RLGetWorkingDirectory();
    //----------------------------------------------------------------------------

    TRACELOG(RL_E_LOG_INFO, "PLATFORM: CUSTOM: Initialized successfully");

    return 0;
}

// Close platform
void ClosePlatform(void)
{
    // TODO: De-initialize graphics, inputs and more
}

// EOF
