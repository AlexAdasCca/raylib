//------------------------------------------------------------------------------------
// Window and Graphics Device Functions (Module: core)
//------------------------------------------------------------------------------------

// Window-related functions
RLAPI void RLInitWindow(int width, int height, const char *title);  // Initialize window and OpenGL context
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
RLAPI RLImage RLGetClipboardImage(void);                              // Get clipboard image
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
RLAPI void RLSetShaderValueTexture(RLShader shader, int locIndex, RLTexture2D texture); // Set shader uniform value for texture (sampler2d)
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
RLAPI void RLSetTargetFPS(int fps);                                 // Set target FPS (maximum)
RLAPI float RLGetFrameTime(void);                                   // Get time in seconds for last frame drawn (delta time)
RLAPI double RLGetTime(void);                                       // Get elapsed time in seconds since InitWindow()
RLAPI int RLGetFPS(void);                                           // Get current FPS

// Custom frame control functions
// NOTE: Those functions are intended for advanced users that want full control over the frame processing
// By default EndDrawing() does this job: draws everything + SwapScreenBuffer() + manage frame timing + PollInputEvents()
// To avoid that behaviour and control frame processes manually, enable in config.h: SUPPORT_CUSTOM_FRAME_CONTROL
RLAPI void RLSwapScreenBuffer(void);                                // Swap back buffer with front buffer (screen drawing)
RLAPI void RLPollInputEvents(void);                                 // Register all input events
RLAPI void RLWaitTime(double seconds);                              // Wait for some time (halt program execution)

// Random values generation functions
RLAPI void RLSetRandomSeed(unsigned int seed);                      // Set the seed for the random number generator
RLAPI int RLGetRandomValue(int min, int max);                       // Get a random value between min and max (both included)
RLAPI int *RLLoadRandomSequence(unsigned int count, int min, int max); // Load random values sequence, no values repeated
RLAPI void RLUnloadRandomSequence(int *sequence);                   // Unload random values sequence

// Misc. functions
RLAPI void RLTakeScreenshot(const char *fileName);                  // Takes a screenshot of current screen (filename extension defines format)
RLAPI void RLSetConfigFlags(unsigned int flags);                    // Setup init configuration flags (view FLAGS)
RLAPI void RLOpenURL(const char *url);                              // Open URL with default system browser (if available)

// NOTE: Following functions implemented in module [utils]
//------------------------------------------------------------------
RLAPI void RLTraceLog(int logLevel, const char *text, ...);         // Show trace log messages (LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR...)
RLAPI void RLSetTraceLogLevel(int logLevel);                        // Set the current threshold (minimum) log level
RLAPI void *RLMemAlloc(unsigned int size);                          // Internal memory allocator
RLAPI void *RLMemRealloc(void *ptr, unsigned int size);             // Internal memory reallocator
RLAPI void RLMemFree(void *ptr);                                    // Internal memory free

// Set custom callbacks
// WARNING: Callbacks setup is intended for advanced users
RLAPI void RLSetTraceLogCallback(RLTraceLogCallback callback);         // Set custom trace log
RLAPI void RLSetLoadFileDataCallback(RLLoadFileDataCallback callback); // Set custom file binary data loader
RLAPI void RLSetSaveFileDataCallback(RLSaveFileDataCallback callback); // Set custom file binary data saver
RLAPI void RLSetLoadFileTextCallback(RLLoadFileTextCallback callback); // Set custom file text data loader
RLAPI void RLSetSaveFileTextCallback(RLSaveFileTextCallback callback); // Set custom file text data saver

// Files management functions
RLAPI unsigned char *RLLoadFileData(const char *fileName, int *dataSize); // Load file data as byte array (read)
RLAPI void RLUnloadFileData(unsigned char *data);                   // Unload file data allocated by LoadFileData()
RLAPI bool RLSaveFileData(const char *fileName, void *data, int dataSize); // Save data to file from byte array (write), returns true on success
RLAPI bool RLExportDataAsCode(const unsigned char *data, int dataSize, const char *fileName); // Export data to code (.h), returns true on success
RLAPI char *RLLoadFileText(const char *fileName);                   // Load text data from file (read), returns a '\0' terminated string
RLAPI void RLUnloadFileText(char *text);                            // Unload file text data allocated by LoadFileText()
RLAPI bool RLSaveFileText(const char *fileName, char *text);        // Save text data to file (write), string must be '\0' terminated, returns true on success
//------------------------------------------------------------------

// File system functions
RLAPI bool RLFileExists(const char *fileName);                      // Check if file exists
RLAPI bool RLDirectoryExists(const char *dirPath);                  // Check if a directory path exists
RLAPI bool RLIsFileExtension(const char *fileName, const char *ext); // Check file extension (including point: .png, .wav)
RLAPI int RLGetFileLength(const char *fileName);                    // Get file length in bytes (NOTE: GetFileSize() conflicts with windows.h)
RLAPI const char *RLGetFileExtension(const char *fileName);         // Get pointer to extension for a filename string (includes dot: '.png')
RLAPI const char *RLGetFileName(const char *filePath);              // Get pointer to filename for a path string
RLAPI const char *RLGetFileNameWithoutExt(const char *filePath);    // Get filename string without extension (uses static string)
RLAPI const char *RLGetDirectoryPath(const char *filePath);         // Get full path for a given fileName with path (uses static string)
RLAPI const char *RLGetPrevDirectoryPath(const char *dirPath);      // Get previous directory path for a given path (uses static string)
RLAPI const char *RLGetWorkingDirectory(void);                      // Get current working directory (uses static string)
RLAPI const char *RLGetApplicationDirectory(void);                  // Get the directory of the running application (uses static string)
RLAPI int RLMakeDirectory(const char *dirPath);                     // Create directories (including full path requested), returns 0 on success
RLAPI bool RLChangeDirectory(const char *dir);                      // Change working directory, return true on success
RLAPI bool RLIsPathFile(const char *path);                          // Check if a given path is a file or a directory
RLAPI bool RLIsFileNameValid(const char *fileName);                 // Check if fileName is valid for the platform/OS
RLAPI RLFilePathList RLLoadDirectoryFiles(const char *dirPath);       // Load directory filepaths
RLAPI RLFilePathList RLLoadDirectoryFilesEx(const char *basePath, const char *filter, bool scanSubdirs); // Load directory filepaths with extension filtering and recursive directory scan. Use 'DIR' in the filter string to include directories in the result
RLAPI void RLUnloadDirectoryFiles(RLFilePathList files);              // Unload filepaths
RLAPI bool RLIsFileDropped(void);                                   // Check if a file has been dropped into window
RLAPI RLFilePathList RLLoadDroppedFiles(void);                        // Load dropped filepaths
RLAPI void RLUnloadDroppedFiles(RLFilePathList files);                // Unload dropped filepaths
RLAPI long RLGetFileModTime(const char *fileName);                  // Get file modification time (last write time)

// Compression/Encoding functionality
RLAPI unsigned char *RLCompressData(const unsigned char *data, int dataSize, int *compDataSize);        // Compress data (DEFLATE algorithm), memory must be MemFree()
RLAPI unsigned char *RLDecompressData(const unsigned char *compData, int compDataSize, int *dataSize);  // Decompress data (DEFLATE algorithm), memory must be MemFree()
RLAPI char *RLEncodeDataBase64(const unsigned char *data, int dataSize, int *outputSize);               // Encode data to Base64 string, memory must be MemFree()
RLAPI unsigned char *RLDecodeDataBase64(const unsigned char *data, int *outputSize);                    // Decode Base64 string data, memory must be MemFree()
RLAPI unsigned int RLComputeCRC32(unsigned char *data, int dataSize);     // Compute CRC32 hash code
RLAPI unsigned int *RLComputeMD5(unsigned char *data, int dataSize);      // Compute MD5 hash code, returns static int[4] (16 bytes)
RLAPI unsigned int *RLComputeSHA1(unsigned char *data, int dataSize);     // Compute SHA1 hash code, returns static int[5] (20 bytes)


// Automation events functionality
RLAPI RLAutomationEventList RLLoadAutomationEventList(const char *fileName);                // Load automation events list from file, NULL for empty list, capacity = MAX_AUTOMATION_EVENTS
RLAPI void RLUnloadAutomationEventList(RLAutomationEventList list);                         // Unload automation events list from file
RLAPI bool RLExportAutomationEventList(RLAutomationEventList list, const char *fileName);   // Export automation events list as text file
RLAPI void RLSetAutomationEventList(RLAutomationEventList *list);                           // Set automation event list to record to
RLAPI void RLSetAutomationEventBaseFrame(int frame);                                      // Set automation event internal base frame to start recording
RLAPI void RLStartAutomationEventRecording(void);                                         // Start recording automation events (AutomationEventList must be set)
RLAPI void RLStopAutomationEventRecording(void);                                          // Stop recording automation events
RLAPI void RLPlayAutomationEvent(RLAutomationEvent event);                                  // Play a recorded automation event

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
RLAPI void RLSetExitKey(int key);                               // Set a custom key to exit program (default is ESC)
RLAPI const char *RLGetKeyName(int key);                        // Get name of a QWERTY key on the current keyboard layout (eg returns string "q" for KEY_A on an AZERTY keyboard)

// Input-related functions: gamepads
RLAPI bool RLIsGamepadAvailable(int gamepad);                                        // Check if a gamepad is available
RLAPI const char *RLGetGamepadName(int gamepad);                                     // Get gamepad internal name id
RLAPI bool RLIsGamepadButtonPressed(int gamepad, int button);                        // Check if a gamepad button has been pressed once
RLAPI bool RLIsGamepadButtonDown(int gamepad, int button);                           // Check if a gamepad button is being pressed
RLAPI bool RLIsGamepadButtonReleased(int gamepad, int button);                       // Check if a gamepad button has been released once
RLAPI bool RLIsGamepadButtonUp(int gamepad, int button);                             // Check if a gamepad button is NOT being pressed
RLAPI int RLGetGamepadButtonPressed(void);                                           // Get the last gamepad button pressed
RLAPI int RLGetGamepadAxisCount(int gamepad);                                        // Get gamepad axis count for a gamepad
RLAPI float RLGetGamepadAxisMovement(int gamepad, int axis);                         // Get axis movement value for a gamepad axis
RLAPI int RLSetGamepadMappings(const char *mappings);                                // Set internal gamepad mappings (SDL_GameControllerDB)
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
RLAPI void RLSetGesturesEnabled(unsigned int flags);      // Enable a set of gestures using flags
RLAPI bool RLIsGestureDetected(unsigned int gesture);     // Check if a gesture have been detected
RLAPI int RLGetGestureDetected(void);                     // Get latest detected gesture
RLAPI float RLGetGestureHoldDuration(void);               // Get gesture hold time in seconds
RLAPI RLVector2 RLGetGestureDragVector(void);               // Get gesture drag vector
RLAPI float RLGetGestureDragAngle(void);                  // Get gesture drag angle
RLAPI RLVector2 RLGetGesturePinchVector(void);              // Get gesture pinch delta
RLAPI float RLGetGesturePinchAngle(void);                 // Get gesture pinch angle

//------------------------------------------------------------------------------------
// Camera System Functions (Module: rcamera)
//------------------------------------------------------------------------------------
RLAPI void RLUpdateCamera(RLCamera *camera, int mode);      // Update camera position for selected mode
RLAPI void RLUpdateCameraPro(RLCamera *camera, RLVector3 movement, RLVector3 rotation, float zoom); // Update camera movement/rotation

//------------------------------------------------------------------------------------
// Basic Shapes Drawing Functions (Module: shapes)
//------------------------------------------------------------------------------------
// Set texture and rectangle to be used on shapes drawing
// NOTE: It can be useful when using basic shapes and one single font,
// defining a font char white rectangle would allow drawing everything in a single draw call
RLAPI void RLSetShapesTexture(RLTexture2D texture, RLRectangle source);       // Set texture and rectangle to be used on shapes drawing
RLAPI RLTexture2D RLGetShapesTexture(void);                                 // Get texture that is used for shapes drawing
RLAPI RLRectangle RLGetShapesTextureRectangle(void);                        // Get texture source rectangle that is used for shapes drawing

// Basic shapes drawing functions
RLAPI void RLDrawPixel(int posX, int posY, RLColor color);                                                   // Draw a pixel using geometry [Can be slow, use with care]
RLAPI void RLDrawPixelV(RLVector2 position, RLColor color);                                                    // Draw a pixel using geometry (Vector version) [Can be slow, use with care]
RLAPI void RLDrawLine(int startPosX, int startPosY, int endPosX, int endPosY, RLColor color);                // Draw a line
RLAPI void RLDrawLineV(RLVector2 startPos, RLVector2 endPos, RLColor color);                                     // Draw a line (using gl lines)
RLAPI void RLDrawLineEx(RLVector2 startPos, RLVector2 endPos, float thick, RLColor color);                       // Draw a line (using triangles/quads)
RLAPI void RLDrawLineStrip(const RLVector2 *points, int pointCount, RLColor color);                            // Draw lines sequence (using gl lines)
RLAPI void RLDrawLineBezier(RLVector2 startPos, RLVector2 endPos, float thick, RLColor color);                   // Draw line segment cubic-bezier in-out interpolation
RLAPI void RLDrawCircle(int centerX, int centerY, float radius, RLColor color);                              // Draw a color-filled circle
RLAPI void RLDrawCircleSector(RLVector2 center, float radius, float startAngle, float endAngle, int segments, RLColor color);      // Draw a piece of a circle
RLAPI void RLDrawCircleSectorLines(RLVector2 center, float radius, float startAngle, float endAngle, int segments, RLColor color); // Draw circle sector outline
RLAPI void RLDrawCircleGradient(int centerX, int centerY, float radius, RLColor inner, RLColor outer);         // Draw a gradient-filled circle
RLAPI void RLDrawCircleV(RLVector2 center, float radius, RLColor color);                                       // Draw a color-filled circle (Vector version)
RLAPI void RLDrawCircleLines(int centerX, int centerY, float radius, RLColor color);                         // Draw circle outline
RLAPI void RLDrawCircleLinesV(RLVector2 center, float radius, RLColor color);                                  // Draw circle outline (Vector version)
RLAPI void RLDrawEllipse(int centerX, int centerY, float radiusH, float radiusV, RLColor color);             // Draw ellipse
RLAPI void RLDrawEllipseLines(int centerX, int centerY, float radiusH, float radiusV, RLColor color);        // Draw ellipse outline
RLAPI void RLDrawRing(RLVector2 center, float innerRadius, float outerRadius, float startAngle, float endAngle, int segments, RLColor color); // Draw ring
RLAPI void RLDrawRingLines(RLVector2 center, float innerRadius, float outerRadius, float startAngle, float endAngle, int segments, RLColor color);    // Draw ring outline
RLAPI void RLDrawRectangle(int posX, int posY, int width, int height, RLColor color);                        // Draw a color-filled rectangle
RLAPI void RLDrawRectangleV(RLVector2 position, RLVector2 size, RLColor color);                                  // Draw a color-filled rectangle (Vector version)
RLAPI void RLDrawRectangleRec(RLRectangle rec, RLColor color);                                                 // Draw a color-filled rectangle
RLAPI void RLDrawRectanglePro(RLRectangle rec, RLVector2 origin, float rotation, RLColor color);                 // Draw a color-filled rectangle with pro parameters
RLAPI void RLDrawRectangleGradientV(int posX, int posY, int width, int height, RLColor top, RLColor bottom);   // Draw a vertical-gradient-filled rectangle
RLAPI void RLDrawRectangleGradientH(int posX, int posY, int width, int height, RLColor left, RLColor right);   // Draw a horizontal-gradient-filled rectangle
RLAPI void RLDrawRectangleGradientEx(RLRectangle rec, RLColor topLeft, RLColor bottomLeft, RLColor topRight, RLColor bottomRight); // Draw a gradient-filled rectangle with custom vertex colors
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
RLAPI void RLDrawSplineLinear(const RLVector2 *points, int pointCount, float thick, RLColor color);                  // Draw spline: Linear, minimum 2 points
RLAPI void RLDrawSplineBasis(const RLVector2 *points, int pointCount, float thick, RLColor color);                   // Draw spline: B-Spline, minimum 4 points
RLAPI void RLDrawSplineCatmullRom(const RLVector2 *points, int pointCount, float thick, RLColor color);              // Draw spline: Catmull-Rom, minimum 4 points
RLAPI void RLDrawSplineBezierQuadratic(const RLVector2 *points, int pointCount, float thick, RLColor color);         // Draw spline: Quadratic Bezier, minimum 3 points (1 control point): [p1, c2, p3, c4...]
RLAPI void RLDrawSplineBezierCubic(const RLVector2 *points, int pointCount, float thick, RLColor color);             // Draw spline: Cubic Bezier, minimum 4 points (2 control points): [p1, c2, c3, p4, c5, c6...]
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
RLAPI void RLImageResizeNN(RLImage *image, int newWidth,int newHeight);                                      // Resize image (Nearest-Neighbor scaling algorithm)
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
RLAPI void RLUpdateTexture(RLTexture2D texture, const void *pixels);                                         // Update GPU texture with new data
RLAPI void RLUpdateTextureRec(RLTexture2D texture, RLRectangle rec, const void *pixels);                       // Update GPU texture rectangle with new data

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
RLAPI RLFont RLLoadFontEx(const char *fileName, int fontSize, int *codepoints, int codepointCount); // Load font from file with extended parameters, use NULL for codepoints and 0 for codepointCount to load the default character set, font size is provided in pixels height
RLAPI RLFont RLLoadFontFromImage(RLImage image, RLColor key, int firstChar);                        // Load font from Image (XNA style)
RLAPI RLFont RLLoadFontFromMemory(const char *fileType, const unsigned char *fileData, int dataSize, int fontSize, int *codepoints, int codepointCount); // Load font from memory buffer, fileType refers to extension: i.e. '.ttf'
RLAPI bool RLIsFontValid(RLFont font);                                                          // Check if a font is valid (font data loaded, WARNING: GPU texture not checked)
RLAPI RLGlyphInfo *RLLoadFontData(const unsigned char *fileData, int dataSize, int fontSize, int *codepoints, int codepointCount, int type); // Load font data for further use
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
RLAPI char *RLLoadUTF8(const int *codepoints, int length);                // Load UTF-8 text encoded from codepoints array
RLAPI void RLUnloadUTF8(char *text);                                      // Unload UTF-8 text encoded from codepoints array
RLAPI int *RLLoadCodepoints(const char *text, int *count);                // Load all codepoints from a UTF-8 text string, codepoints count returned by parameter
RLAPI void RLUnloadCodepoints(int *codepoints);                           // Unload codepoints data from memory
RLAPI int RLGetCodepointCount(const char *text);                          // Get total number of codepoints in a UTF-8 encoded string
RLAPI int RLGetCodepoint(const char *text, int *codepointSize);           // Get next codepoint in a UTF-8 encoded string, 0x3f('?') is returned on failure
RLAPI int RLGetCodepointNext(const char *text, int *codepointSize);       // Get next codepoint in a UTF-8 encoded string, 0x3f('?') is returned on failure
RLAPI int RLGetCodepointPrevious(const char *text, int *codepointSize);   // Get previous codepoint in a UTF-8 encoded string, 0x3f('?') is returned on failure
RLAPI const char *RLCodepointToUTF8(int codepoint, int *utf8Size);        // Encode one codepoint into UTF-8 byte array (array length returned as parameter)

// Text strings management functions (no UTF-8 strings, only byte chars)
// NOTE: Some strings allocate memory internally for returned strings, just be careful!
RLAPI int RLTextCopy(char *dst, const char *src);                                             // Copy one string to another, returns bytes copied
RLAPI bool RLTextIsEqual(const char *text1, const char *text2);                               // Check if two text string are equal
RLAPI unsigned int RLTextLength(const char *text);                                            // Get text length, checks for '\0' ending
RLAPI const char *RLTextFormat(const char *text, ...);                                        // Text formatting with variables (sprintf() style)
RLAPI const char *RLTextSubtext(const char *text, int position, int length);                  // Get a piece of a text string
RLAPI char *RLTextReplace(const char *text, const char *replace, const char *by);             // Replace text string (WARNING: memory must be freed!)
RLAPI char *RLTextInsert(const char *text, const char *insert, int position);                 // Insert text in a position (WARNING: memory must be freed!)
RLAPI const char *RLTextJoin(const char **textList, int count, const char *delimiter);        // Join text strings with delimiter
RLAPI const char **RLTextSplit(const char *text, char delimiter, int *count);                 // Split text into multiple strings
RLAPI void RLTextAppend(char *text, const char *append, int *position);                       // Append text at specific position and move cursor!
RLAPI int RLTextFindIndex(const char *text, const char *find);                                // Find first text occurrence within a string
RLAPI const char *RLTextToUpper(const char *text);                      // Get upper case version of provided string
RLAPI const char *RLTextToLower(const char *text);                      // Get lower case version of provided string
RLAPI const char *RLTextToPascal(const char *text);                     // Get Pascal case notation version of provided string
RLAPI const char *RLTextToSnake(const char *text);                      // Get Snake case notation version of provided string
RLAPI const char *RLTextToCamel(const char *text);                      // Get Camel case notation version of provided string

RLAPI int RLTextToInteger(const char *text);                            // Get integer value from text (negative values not supported)
RLAPI float RLTextToFloat(const char *text);                            // Get float value from text (negative values not supported)

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
RLAPI bool RLCheckCollisionSpheres(RLVector3 center1, float radius1, RLVector3 center2, float radius2);   // Check collision between two spheres
RLAPI bool RLCheckCollisionBoxes(RLBoundingBox box1, RLBoundingBox box2);                                 // Check collision between two bounding boxes
RLAPI bool RLCheckCollisionBoxSphere(RLBoundingBox box, RLVector3 center, float radius);                  // Check collision between box and sphere
RLAPI RLRayCollision RLGetRayCollisionSphere(RLRay ray, RLVector3 center, float radius);                    // Get collision info between ray and sphere
RLAPI RLRayCollision RLGetRayCollisionBox(RLRay ray, RLBoundingBox box);                                    // Get collision info between ray and box
RLAPI RLRayCollision RLGetRayCollisionMesh(RLRay ray, RLMesh mesh, RLMatrix transform);                       // Get collision info between ray and mesh
RLAPI RLRayCollision RLGetRayCollisionTriangle(RLRay ray, RLVector3 p1, RLVector3 p2, RLVector3 p3);            // Get collision info between ray and triangle
RLAPI RLRayCollision RLGetRayCollisionQuad(RLRay ray, RLVector3 p1, RLVector3 p2, RLVector3 p3, RLVector3 p4);    // Get collision info between ray and quad

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
RLAPI void RLUpdateSound(RLSound sound, const void *data, int sampleCount); // Update sound buffer with new data
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
RLAPI void RLSetSoundPan(RLSound sound, float pan);                       // Set pan for a sound (0.5 is center)
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
RLAPI void RLSetMusicPan(RLMusic music, float pan);                       // Set pan for a music (0.5 is center)
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

