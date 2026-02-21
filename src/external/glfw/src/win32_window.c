//========================================================================
// GLFW 3.4 Win32 (modified for raylib) - www.glfw.org; www.raylib.com
//------------------------------------------------------------------------
// Copyright (c) 2002-2006 Marcus Geelnard
// Copyright (c) 2006-2019 Camilla Löwy <elmindreda@glfw.org>
// Copyright (c) 2024 M374LX <wilsalx@gmail.com>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================

#include "internal.h"

#if defined(_GLFW_WIN32)

#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <windowsx.h>
#include <shellapi.h>

// -------------------------------------------------------------------------
// Win32 per-window message hook list
// -------------------------------------------------------------------------

struct _GLFWwin32MessageHook
{
    int (*fn)(GLFWwindow* window,
              HWND hWnd, unsigned int uMsg, uintptr_t wParam, intptr_t lParam,
              intptr_t* result, void* user);
    void* user;
    struct _GLFWwin32MessageHook* next;
};

// Forward declaration needed by window class registration
static LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// -------------------------------------------------------------------------
// Win32 window class registry (per-class ref-counting)
// -------------------------------------------------------------------------

static const WCHAR* _glfwGetDefaultWin32ClassName(void)
{
#ifdef _GLFW_WNDCLASSNAME
    return _GLFW_WNDCLASSNAME;
#else
    return L"GLFW30";
#endif
}

static _GLFWwin32WindowClass* _glfwAcquireWindowClassWin32(const _GLFWwndconfig* wndconfig)
{
    _GLFWwin32WindowClass* it = NULL;
    WCHAR* requested = NULL;
    const WCHAR* classNameW = NULL;

    if (wndconfig && wndconfig->win32.className[0])
        requested = _glfwCreateWideStringFromUTF8Win32(wndconfig->win32.className);

    classNameW = requested ? requested : _glfwGetDefaultWin32ClassName();

    if (_glfw.win32.classLock)
        _glfwPlatformLockMutex(_glfw.win32.classLock);

    for (it = _glfw.win32.windowClasses; it; it = it->next)
    {
        if (it->name && wcscmp(it->name, classNameW) == 0)
        {
            it->refcount++;
            if (_glfw.win32.classLock)
                _glfwPlatformUnlockMutex(_glfw.win32.classLock);
            if (requested) _glfw_free(requested);
            return it;
        }
    }

    // Register new window class
    it = _glfw_calloc(1, sizeof(_GLFWwin32WindowClass));
    if (!it)
    {
        if (_glfw.win32.classLock)
            _glfwPlatformUnlockMutex(_glfw.win32.classLock);
        if (requested) _glfw_free(requested);
        _glfwInputError(GLFW_OUT_OF_MEMORY, "Win32: Failed to allocate window class registry entry");
        return NULL;
    }

    {
        const size_t len = wcslen(classNameW) + 1;
        it->name = _glfw_calloc(len, sizeof(WCHAR));
        if (it->name)
            wcscpy(it->name, classNameW);
    }
    if (!it->name)
    {
        _glfw_free(it);
        if (_glfw.win32.classLock)
            _glfwPlatformUnlockMutex(_glfw.win32.classLock);
        if (requested) _glfw_free(requested);
        _glfwInputError(GLFW_OUT_OF_MEMORY, "Win32: Failed to allocate window class name");
        return NULL;
    }

    it->refcount = 1;

    // Insert into registry list
    it->next = _glfw.win32.windowClasses;
    _glfw.win32.windowClasses = it;

    if (_glfw.win32.classLock)
        _glfwPlatformUnlockMutex(_glfw.win32.classLock);

    if (requested) _glfw_free(requested);

    return it;
}

static GLFWbool _glfwEnsureWindowClassRegisteredWin32(_GLFWwin32WindowClass* cls)
{
    WNDCLASSEXW wc;

    if (!cls)
        return GLFW_FALSE;

    if (_glfw.win32.classLock)
        _glfwPlatformLockMutex(_glfw.win32.classLock);
    if (cls->atom)
    {
        if (_glfw.win32.classLock)
            _glfwPlatformUnlockMutex(_glfw.win32.classLock);
        return GLFW_TRUE;
    }

    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = (WNDPROC) windowProc;
    wc.hInstance     = _glfw.win32.instance;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.lpszClassName = cls->name;

    // Load user-provided icon if available
    wc.hIcon = LoadImageW(GetModuleHandleW(NULL),
                          L"GLFW_ICON", IMAGE_ICON,
                          0, 0, LR_DEFAULTSIZE | LR_SHARED);
    if (!wc.hIcon)
    {
        // No user-provided icon found, load default icon
        wc.hIcon = LoadImageW(NULL,
                              IDI_APPLICATION, IMAGE_ICON,
                              0, 0, LR_DEFAULTSIZE | LR_SHARED);
    }
    wc.hIconSm = wc.hIcon;

    cls->atom = RegisterClassExW(&wc);
    if (!cls->atom)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR, "Win32: Failed to register window class");
        if (_glfw.win32.classLock)
            _glfwPlatformUnlockMutex(_glfw.win32.classLock);
        return GLFW_FALSE;
    }

    if (_glfw.win32.classLock)
        _glfwPlatformUnlockMutex(_glfw.win32.classLock);

    return GLFW_TRUE;
}

static void _glfwReleaseWindowClassWin32(_GLFWwin32WindowClass* cls)
{
    _GLFWwin32WindowClass** pp = NULL;

    if (!cls)
        return;

    if (_glfw.win32.classLock)
        _glfwPlatformLockMutex(_glfw.win32.classLock);

    if (cls->refcount > 0)
        cls->refcount--;

    if (cls->refcount == 0)
    {
        // unlink
        for (pp = &_glfw.win32.windowClasses; *pp; pp = &(*pp)->next)
        {
            if (*pp == cls)
            {
                *pp = cls->next;
                break;
            }
        }
        if (cls->atom)
            UnregisterClassW(MAKEINTATOM(cls->atom), _glfw.win32.instance);
        if (cls->name)
            _glfw_free(cls->name);
        _glfw_free(cls);
    }

    if (_glfw.win32.classLock)
        _glfwPlatformUnlockMutex(_glfw.win32.classLock);
}

// -------------------------------------------------------------------------
// Win32 per-thread event wait / wake support
// -------------------------------------------------------------------------

_GLFWwin32ThreadContext* _glfwGetThreadContextWin32(void)
{
    _GLFWwin32ThreadContext* ctx = NULL;
    const DWORD tid = GetCurrentThreadId();

    if (_glfw.win32.threadLock)
        _glfwPlatformLockMutex(_glfw.win32.threadLock);

    // Find existing context
    for (ctx = _glfw.win32.threadContexts; ctx; ctx = ctx->next)
    {
        if (ctx->tid == tid)
            break;
    }

    if (!ctx)
    {
        ctx = _glfw_calloc(1, sizeof(_GLFWwin32ThreadContext));
        if (!ctx)
        {
            if (_glfw.win32.threadLock)
                _glfwPlatformUnlockMutex(_glfw.win32.threadLock);

            _glfwInputError(GLFW_OUT_OF_MEMORY, "Win32: Failed to allocate thread context");
            return NULL;
        }

        ctx->tid = tid;
        ctx->wakeEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (!ctx->wakeEvent)
        {
            if (_glfw.win32.threadLock)
                _glfwPlatformUnlockMutex(_glfw.win32.threadLock);

            _glfw_free(ctx);
            _glfwInputError(GLFW_PLATFORM_ERROR, "Win32: Failed to create thread wake event");
            return NULL;
        }

        InitializeCriticalSection(&ctx->tasksLock);
        ctx->tasksHead = NULL;
        ctx->tasksTail = NULL;
        ctx->dispatchWindow = NULL;

        ctx->next = _glfw.win32.threadContexts;
        _glfw.win32.threadContexts = ctx;
    }

    if (_glfw.win32.threadLock)
        _glfwPlatformUnlockMutex(_glfw.win32.threadLock);

    // Ensure dispatch window exists (outside of global threadLock)
    _glfwEnsureDispatchWindowWin32(ctx);

    return ctx;
}


// -------------------------------------------------------------------------
// Win32 dispatch window and cross-thread task queue
// -------------------------------------------------------------------------

#define GLFW_WM_THREAD_TASK   (WM_APP + 0x3A)
#define GLFW_WM_THREAD_WAKE   (WM_APP + 0x3B)
#define GLFW_TIMER_REFRESH    1

static LRESULT CALLBACK dispatchWindowProcWin32(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    _GLFWwin32ThreadContext* ctx = (_GLFWwin32ThreadContext*) GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    if (uMsg == WM_NCCREATE)
    {
        const CREATESTRUCTW* cs = (const CREATESTRUCTW*) lParam;
        ctx = (_GLFWwin32ThreadContext*) cs->lpCreateParams;
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR) ctx);
        return TRUE;
    }

    if (!ctx)
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);

    switch (uMsg)
    {

        case GLFW_WM_THREAD_TASK:
            _glfwDrainThreadTasksWin32(ctx);
            return 0;
        case GLFW_WM_THREAD_WAKE:
            // No-op message used to break modal loops / wake message waits
            return 0;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

GLFWbool _glfwEnsureDispatchWindowWin32(_GLFWwin32ThreadContext* ctx)
{
    if (!ctx)
        return GLFW_FALSE;

    if (ctx->dispatchWindow)
        return GLFW_TRUE;

    // Ensure the dispatch window class is registered once per process
    if (!_glfw.win32.dispatchWindowClass)
    {
        WNDCLASSEXW wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = dispatchWindowProcWin32;
        wc.hInstance = _glfw.win32.instance;
        wc.lpszClassName = L"GLFW3 Thread Dispatch";

        _glfw.win32.dispatchWindowClass = RegisterClassExW(&wc);
        if (!_glfw.win32.dispatchWindowClass)
        {
            _glfwInputError(GLFW_PLATFORM_ERROR, "Win32: Failed to register dispatch window class");
            return GLFW_FALSE;
        }
    }

    ctx->dispatchWindow = CreateWindowExW(0,
                                         MAKEINTATOM(_glfw.win32.dispatchWindowClass),
                                         L"",
                                         0, 0, 0, 0, 0,
                                         HWND_MESSAGE,
                                         NULL,
                                         _glfw.win32.instance,
                                         ctx);

    if (!ctx->dispatchWindow)
    {
        _glfwInputError(GLFW_PLATFORM_ERROR, "Win32: Failed to create dispatch window");
        return GLFW_FALSE;
    }

    return GLFW_TRUE;
}

void _glfwDrainThreadTasksWin32(_GLFWwin32ThreadContext* ctx)
{
    if (!ctx)
        return;

    for (;;)
    {
        _GLFWwin32ThreadTask* task = NULL;

        EnterCriticalSection(&ctx->tasksLock);
        task = ctx->tasksHead;
        if (task)
        {
            ctx->tasksHead = task->next;
            if (!ctx->tasksHead)
                ctx->tasksTail = NULL;
        }
        LeaveCriticalSection(&ctx->tasksLock);

        if (!task)
            break;

        if (task->fn)
            task->fn(task->user);

        _glfw_free(task);
    }
}

void _glfwPostTaskWin32(_GLFWwin32ThreadContext* ctx, void (*fn)(void* user), void* user)
{
    if (!ctx || !fn)
        return;

    if (!_glfwEnsureDispatchWindowWin32(ctx))
        return;

    _GLFWwin32ThreadTask* task = _glfw_calloc(1, sizeof(_GLFWwin32ThreadTask));
    if (!task)
    {
        _glfwInputError(GLFW_OUT_OF_MEMORY, NULL);
        return;
    }

    task->fn = fn;
    task->user = user;

    EnterCriticalSection(&ctx->tasksLock);
    if (ctx->tasksTail)
        ctx->tasksTail->next = task;
    else
        ctx->tasksHead = task;
    ctx->tasksTail = task;
    LeaveCriticalSection(&ctx->tasksLock);

    // Wake the owning thread even if it's blocked in WaitEvents or a modal loop
    if (ctx->wakeEvent)
        SetEvent(ctx->wakeEvent);
    PostMessageW(ctx->dispatchWindow, GLFW_WM_THREAD_TASK, 0, 0);
}

void _glfwWakeThreadWin32(_GLFWwin32ThreadContext* ctx)
{
    if (!ctx)
        return;

    if (ctx->wakeEvent)
        SetEvent(ctx->wakeEvent);

    if (ctx->dispatchWindow)
        PostMessageW(ctx->dispatchWindow, GLFW_WM_THREAD_WAKE, 0, 0);
}

static void wakeAllThreadsWin32(void)
{
    _GLFWwin32ThreadContext* ctx;

    _glfwPlatformLockMutex(_glfw.win32.threadLock);
    for (ctx = _glfw.win32.threadContexts; ctx; ctx = ctx->next)
    {
        if (ctx->wakeEvent)
            SetEvent(ctx->wakeEvent);
    }
    _glfwPlatformUnlockMutex(_glfw.win32.threadLock);
}

// Returns the window style for the specified window
//
static DWORD getWindowStyle(const _GLFWwindow* window)
{
    DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    if (window->monitor)
        style |= WS_POPUP;
    else
    {
        style |= WS_SYSMENU | WS_MINIMIZEBOX;

        if (window->decorated)
        {
            style |= WS_CAPTION;

            if (window->resizable || window->win32.snapLayout)
                style |= WS_MAXIMIZEBOX | WS_THICKFRAME;
        }
        else
            style |= WS_POPUP;
    }

    return style;
}

// Returns the extended window style for the specified window
//
static DWORD getWindowExStyle(const _GLFWwindow* window)
{
    DWORD style = WS_EX_APPWINDOW;

    if (window->monitor || window->floating)
        style |= WS_EX_TOPMOST;

    return style;
}

// Returns the image whose area most closely matches the desired one
//
static const GLFWimage* chooseImage(int count, const GLFWimage* images,
                                    int width, int height)
{
    int i, leastDiff = INT_MAX;
    const GLFWimage* closest = NULL;

    for (i = 0;  i < count;  i++)
    {
        const int currDiff = abs(images[i].width * images[i].height -
                                 width * height);
        if (currDiff < leastDiff)
        {
            closest = images + i;
            leastDiff = currDiff;
        }
    }

    return closest;
}

// Creates an RGBA icon or cursor
//
static HICON createIcon(const GLFWimage* image, int xhot, int yhot, GLFWbool icon)
{
    int i;
    HDC dc;
    HICON handle;
    HBITMAP color, mask;
    BITMAPV5HEADER bi;
    ICONINFO ii;
    unsigned char* target = NULL;
    unsigned char* source = image->pixels;

    ZeroMemory(&bi, sizeof(bi));
    bi.bV5Size        = sizeof(bi);
    bi.bV5Width       = image->width;
    bi.bV5Height      = -image->height;
    bi.bV5Planes      = 1;
    bi.bV5BitCount    = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask     = 0x00ff0000;
    bi.bV5GreenMask   = 0x0000ff00;
    bi.bV5BlueMask    = 0x000000ff;
    bi.bV5AlphaMask   = 0xff000000;

    dc = GetDC(NULL);
    color = CreateDIBSection(dc,
                             (BITMAPINFO*) &bi,
                             DIB_RGB_COLORS,
                             (void**) &target,
                             NULL,
                             (DWORD) 0);
    ReleaseDC(NULL, dc);

    if (!color)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                             "Win32: Failed to create RGBA bitmap");
        return NULL;
    }

    mask = CreateBitmap(image->width, image->height, 1, 1, NULL);
    if (!mask)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                             "Win32: Failed to create mask bitmap");
        DeleteObject(color);
        return NULL;
    }

    for (i = 0;  i < image->width * image->height;  i++)
    {
        target[0] = source[2];
        target[1] = source[1];
        target[2] = source[0];
        target[3] = source[3];
        target += 4;
        source += 4;
    }

    ZeroMemory(&ii, sizeof(ii));
    ii.fIcon    = icon;
    ii.xHotspot = xhot;
    ii.yHotspot = yhot;
    ii.hbmMask  = mask;
    ii.hbmColor = color;

    handle = CreateIconIndirect(&ii);

    DeleteObject(color);
    DeleteObject(mask);

    if (!handle)
    {
        if (icon)
        {
            _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                                 "Win32: Failed to create icon");
        }
        else
        {
            _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                                 "Win32: Failed to create cursor");
        }
    }

    return handle;
}

// Enforce the content area aspect ratio based on which edge is being dragged
//
static void applyAspectRatio(_GLFWwindow* window, int edge, RECT* area)
{
    RECT frame = {0};
    const float ratio = (float) window->numer / (float) window->denom;
    const DWORD style = getWindowStyle(window);
    const DWORD exStyle = getWindowExStyle(window);

    if (_glfwIsWindows10Version1607OrGreaterWin32())
    {
        AdjustWindowRectExForDpi(&frame, style, FALSE, exStyle,
                                 GetDpiForWindow(window->win32.handle));
    }
    else
        AdjustWindowRectEx(&frame, style, FALSE, exStyle);

    if (edge == WMSZ_LEFT  || edge == WMSZ_BOTTOMLEFT ||
        edge == WMSZ_RIGHT || edge == WMSZ_BOTTOMRIGHT)
    {
        area->bottom = area->top + (frame.bottom - frame.top) +
            (int) (((area->right - area->left) - (frame.right - frame.left)) / ratio);
    }
    else if (edge == WMSZ_TOPLEFT || edge == WMSZ_TOPRIGHT)
    {
        area->top = area->bottom - (frame.bottom - frame.top) -
            (int) (((area->right - area->left) - (frame.right - frame.left)) / ratio);
    }
    else if (edge == WMSZ_TOP || edge == WMSZ_BOTTOM)
    {
        area->right = area->left + (frame.right - frame.left) +
            (int) (((area->bottom - area->top) - (frame.bottom - frame.top)) * ratio);
    }
}

// Updates the cursor image according to its cursor mode
//
static void updateCursorImage(_GLFWwindow* window)
{
    if (window->cursorMode == GLFW_CURSOR_NORMAL ||
        window->cursorMode == GLFW_CURSOR_CAPTURED)
    {
        if (window->cursor)
            SetCursor(window->cursor->win32.handle);
        else
            SetCursor(LoadCursorW(NULL, IDC_ARROW));
    }
    else
    {
        // NOTE: Via Remote Desktop, setting the cursor to NULL does not hide it.
        // HACK: When running locally, it is set to NULL, but when connected via Remote
        //       Desktop, this is a transparent cursor.
        SetCursor(_glfw.win32.blankCursor);
    }
}

// Sets the cursor clip rect to the window content area
//
static void captureCursor(_GLFWwindow* window)
{
    RECT clipRect;
    GetClientRect(window->win32.handle, &clipRect);
    ClientToScreen(window->win32.handle, (POINT*) &clipRect.left);
    ClientToScreen(window->win32.handle, (POINT*) &clipRect.right);
    ClipCursor(&clipRect);
    _glfw.win32.capturedCursorWindow = window;
}

// Disabled clip cursor
//
static void releaseCursor(void)
{
    ClipCursor(NULL);
    _glfw.win32.capturedCursorWindow = NULL;
}

// Enables WM_INPUT messages for the mouse for the specified window
//
static void enableRawMouseMotion(_GLFWwindow* window)
{
    const RAWINPUTDEVICE rid = { 0x01, 0x02, 0, window->win32.handle };

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                             "Win32: Failed to register raw input device");
    }
}

// Disables WM_INPUT messages for the mouse
//
static void disableRawMouseMotion(_GLFWwindow* window)
{
    const RAWINPUTDEVICE rid = { 0x01, 0x02, RIDEV_REMOVE, NULL };

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                             "Win32: Failed to remove raw input device");
    }
}

// Apply disabled cursor mode to a focused window
//
static void disableCursor(_GLFWwindow* window)
{
    _glfw.win32.disabledCursorWindow = window;
    _glfwGetCursorPosWin32(window,
                           &_glfw.win32.restoreCursorPosX,
                           &_glfw.win32.restoreCursorPosY);
    updateCursorImage(window);
    _glfwCenterCursorInContentArea(window);
    captureCursor(window);

    if (window->rawMouseMotion)
        enableRawMouseMotion(window);
}

// Exit disabled cursor mode for the specified window
//
static void enableCursor(_GLFWwindow* window)
{
    if (window->rawMouseMotion)
        disableRawMouseMotion(window);

    _glfw.win32.disabledCursorWindow = NULL;
    releaseCursor();
    _glfwSetCursorPosWin32(window,
                           _glfw.win32.restoreCursorPosX,
                           _glfw.win32.restoreCursorPosY);
    updateCursorImage(window);
}

// Returns whether the cursor is in the content area of the specified window
//
static GLFWbool cursorInContentArea(_GLFWwindow* window)
{
    RECT area;
    POINT pos;

    if (!GetCursorPos(&pos))
        return GLFW_FALSE;

    if (WindowFromPoint(pos) != window->win32.handle)
        return GLFW_FALSE;

    GetClientRect(window->win32.handle, &area);
    ClientToScreen(window->win32.handle, (POINT*) &area.left);
    ClientToScreen(window->win32.handle, (POINT*) &area.right);

    return PtInRect(&area, pos);
}

// Update native window styles to match attributes
//
static void updateWindowStyles(const _GLFWwindow* window)
{
    RECT rect;
    DWORD style = GetWindowLongW(window->win32.handle, GWL_STYLE);
    style &= ~(WS_OVERLAPPEDWINDOW | WS_POPUP);
    style |= getWindowStyle(window);

    GetClientRect(window->win32.handle, &rect);

    if (_glfwIsWindows10Version1607OrGreaterWin32())
    {
        AdjustWindowRectExForDpi(&rect, style, FALSE,
                                 getWindowExStyle(window),
                                 GetDpiForWindow(window->win32.handle));
    }
    else
        AdjustWindowRectEx(&rect, style, FALSE, getWindowExStyle(window));

    ClientToScreen(window->win32.handle, (POINT*) &rect.left);
    ClientToScreen(window->win32.handle, (POINT*) &rect.right);
    SetWindowLongW(window->win32.handle, GWL_STYLE, style);
    SetWindowPos(window->win32.handle, HWND_TOP,
                 rect.left, rect.top,
                 rect.right - rect.left, rect.bottom - rect.top,
                 SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOZORDER);
}

// Update window framebuffer transparency
//
static void updateFramebufferTransparency(const _GLFWwindow* window)
{
    BOOL composition, opaque;
    DWORD color;

    if (!IsWindowsVistaOrGreater())
        return;

    if (FAILED(DwmIsCompositionEnabled(&composition)) || !composition)
       return;

    if (IsWindows8OrGreater() ||
        (SUCCEEDED(DwmGetColorizationColor(&color, &opaque)) && !opaque))
    {
        HRGN region = CreateRectRgn(0, 0, -1, -1);
        DWM_BLURBEHIND bb = {0};
        bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
        bb.hRgnBlur = region;
        bb.fEnable = TRUE;

        DwmEnableBlurBehindWindow(window->win32.handle, &bb);
        DeleteObject(region);
    }
    else
    {
        // HACK: Disable framebuffer transparency on Windows 7 when the
        //       colorization color is opaque, because otherwise the window
        //       contents is blended additively with the previous frame instead
        //       of replacing it
        DWM_BLURBEHIND bb = {0};
        bb.dwFlags = DWM_BB_ENABLE;
        DwmEnableBlurBehindWindow(window->win32.handle, &bb);
    }
}

// Retrieves and translates modifier keys
//
static int getKeyMods(void)
{
    int mods = 0;

    if (GetKeyState(VK_SHIFT) & 0x8000)
        mods |= GLFW_MOD_SHIFT;
    if (GetKeyState(VK_CONTROL) & 0x8000)
        mods |= GLFW_MOD_CONTROL;
    if (GetKeyState(VK_MENU) & 0x8000)
        mods |= GLFW_MOD_ALT;
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000)
        mods |= GLFW_MOD_SUPER;
    if (GetKeyState(VK_CAPITAL) & 1)
        mods |= GLFW_MOD_CAPS_LOCK;
    if (GetKeyState(VK_NUMLOCK) & 1)
        mods |= GLFW_MOD_NUM_LOCK;

    return mods;
}

static void fitToMonitor(_GLFWwindow* window)
{
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(window->monitor->win32.handle, &mi);
    SetWindowPos(window->win32.handle, HWND_TOPMOST,
                 mi.rcMonitor.left,
                 mi.rcMonitor.top,
                 mi.rcMonitor.right - mi.rcMonitor.left,
                 mi.rcMonitor.bottom - mi.rcMonitor.top,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
}

// Make the specified window and its video mode active on its monitor
//
static void acquireMonitorWin32(_GLFWwindow* window)
{
    if (!_glfw.win32.acquiredMonitorCount)
    {
        SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);

        // HACK: When mouse trails are enabled the cursor becomes invisible when
        //       the OpenGL ICD switches to page flipping
        SystemParametersInfoW(SPI_GETMOUSETRAILS, 0, &_glfw.win32.mouseTrailSize, 0);
        SystemParametersInfoW(SPI_SETMOUSETRAILS, 0, 0, 0);
    }

    if (!window->monitor->window)
        _glfw.win32.acquiredMonitorCount++;

    _glfwSetVideoModeWin32(window->monitor, &window->videoMode);
    _glfwInputMonitorWindow(window->monitor, window);
}

// Remove the window and restore the original video mode
//
static void releaseMonitorWin32(_GLFWwindow* window)
{
    if (window->monitor->window != window)
        return;

    _glfw.win32.acquiredMonitorCount--;
    if (!_glfw.win32.acquiredMonitorCount)
    {
        SetThreadExecutionState(ES_CONTINUOUS);

        // HACK: Restore mouse trail length saved in acquireMonitorWin32
        SystemParametersInfoW(SPI_SETMOUSETRAILS, _glfw.win32.mouseTrailSize, 0, 0);
    }

    _glfwInputMonitorWindow(window->monitor, NULL);
    _glfwRestoreVideoModeWin32(window->monitor);
}

// Manually maximize the window, for when SW_MAXIMIZE cannot be used
//
static void maximizeWindowManually(_GLFWwindow* window)
{
    RECT rect;
    DWORD style;
    MONITORINFO mi = { sizeof(mi) };

    GetMonitorInfoW(MonitorFromWindow(window->win32.handle,
                                      MONITOR_DEFAULTTONEAREST), &mi);

    rect = mi.rcWork;

    if (window->maxwidth != GLFW_DONT_CARE && window->maxheight != GLFW_DONT_CARE)
    {
        rect.right = _glfw_min(rect.right, rect.left + window->maxwidth);
        rect.bottom = _glfw_min(rect.bottom, rect.top + window->maxheight);
    }

    style = GetWindowLongW(window->win32.handle, GWL_STYLE);
    style |= WS_MAXIMIZE;
    SetWindowLongW(window->win32.handle, GWL_STYLE, style);

    if (window->decorated)
    {
        const DWORD exStyle = GetWindowLongW(window->win32.handle, GWL_EXSTYLE);

        if (_glfwIsWindows10Version1607OrGreaterWin32())
        {
            const UINT dpi = GetDpiForWindow(window->win32.handle);
            AdjustWindowRectExForDpi(&rect, style, FALSE, exStyle, dpi);
            OffsetRect(&rect, 0, GetSystemMetricsForDpi(SM_CYCAPTION, dpi));
        }
        else
        {
            AdjustWindowRectEx(&rect, style, FALSE, exStyle);
            OffsetRect(&rect, 0, GetSystemMetrics(SM_CYCAPTION));
        }

        rect.bottom = _glfw_min(rect.bottom, mi.rcWork.bottom);
    }

    SetWindowPos(window->win32.handle, HWND_TOP,
                 rect.left,
                 rect.top,
                 rect.right - rect.left,
                 rect.bottom - rect.top,
                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

// Window procedure for user-created windows
//
static LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    _GLFWwindow* window = GetPropW(hWnd, L"GLFW");
    if (!window)
    {
        if (uMsg == WM_NCCREATE)
        {
            if (_glfwIsWindows10Version1607OrGreaterWin32())
            {
                const CREATESTRUCTW* cs = (const CREATESTRUCTW*) lParam;
                const _GLFWwndconfig* wndconfig = cs->lpCreateParams;

                // On per-monitor DPI aware V1 systems, only enable
                // non-client scaling for windows that scale the client area
                // We need WM_GETDPISCALEDSIZE from V2 to keep the client
                // area static when the non-client area is scaled
                if (wndconfig && wndconfig->scaleToMonitor)
                    EnableNonClientDpiScaling(hWnd);
            }
        }

        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    // Raylib: allow cross-thread dispatch of small operations onto the HWND owner thread.
    // This runs *before* user hooks to keep the internal control channel private.
    if (uMsg == _glfw.win32.raylibDispatchMsg)
    {
        // wParam: function pointer, lParam: user pointer
        // Signature: LRESULT (*fn)(GLFWwindow* window, HWND hWnd, void* user)
        if (wParam)
            return ((LRESULT (*)(GLFWwindow*, HWND, void*)) wParam)((GLFWwindow*) window, hWnd, (void*) lParam);
        return 0;
    }

    // Give user-registered hooks a chance to handle the message first.
    // Hooks are copied while holding a lock so they can be safely removed
    // concurrently without risking use-after-free.
    if (window->win32.messageHooks)
    {
        typedef struct _HookCall
        {
            int (*fn)(GLFWwindow* window,
              HWND hWnd, unsigned int uMsg, uintptr_t wParam, intptr_t lParam,
              intptr_t* result, void* user);
            void* user;
        } _HookCall;

        _HookCall stackCalls[16];
        _HookCall* calls = stackCalls;
        size_t count = 0;
        size_t cap = sizeof(stackCalls) / sizeof(stackCalls[0]);
        _GLFWwin32MessageHook* it;
        intptr_t hookResult = 0;
        GLFWbool handled = GLFW_FALSE;

        if (_glfw.win32.hookLock)
            _glfwPlatformLockMutex(_glfw.win32.hookLock);
        window->win32.hookDispatchDepth++;

        for (it = window->win32.messageHooks; it; it = it->next)
        {
            if (count == cap)
            {
                const size_t newCap = cap * 2;
                _HookCall* newCalls = _glfw_calloc(newCap, sizeof(_HookCall));
                if (!newCalls)
                    break;
                memcpy(newCalls, calls, cap * sizeof(_HookCall));
                if (calls != stackCalls)
                    _glfw_free(calls);
                calls = newCalls;
                cap = newCap;
            }
            calls[count].fn = it->fn;
            calls[count].user = it->user;
            count++;
        }

        if (_glfw.win32.hookLock)
            _glfwPlatformUnlockMutex(_glfw.win32.hookLock);

        for (size_t i = 0; i < count; i++)
        {
            if (calls[i].fn && calls[i].fn((GLFWwindow*) window, hWnd, uMsg, wParam, lParam, &hookResult, calls[i].user))
            {
                handled = GLFW_TRUE;
                break;
            }
        }

        if (calls != stackCalls)
            _glfw_free(calls);

        if (_glfw.win32.hookLock)
            _glfwPlatformLockMutex(_glfw.win32.hookLock);

        window->win32.hookDispatchDepth--;
        if (window->win32.hookDispatchDepth <= 0 && window->win32.pendingHookFrees)
        {
            _GLFWwin32MessageHook* pit = window->win32.pendingHookFrees;
            window->win32.pendingHookFrees = NULL;
            while (pit)
            {
                _GLFWwin32MessageHook* next = pit->next;
                _glfw_free(pit);
                pit = next;
            }
        }

        if (_glfw.win32.hookLock)
            _glfwPlatformUnlockMutex(_glfw.win32.hookLock);

        if (handled)
        return (LRESULT) hookResult;
    }

    switch (uMsg)
    {
        case WM_NCHITTEST:
        {
            // Allow Windows 11 Snap Layout affordances while preventing
            // interactive border resizing when the window is not resizable.
            LRESULT hit = DefWindowProcW(hWnd, uMsg, wParam, lParam);
            if (window->win32.snapLayout && !window->resizable)
            {
                switch (hit)
                {
                    case HTLEFT: case HTRIGHT: case HTTOP: case HTBOTTOM:
                    case HTTOPLEFT: case HTTOPRIGHT: case HTBOTTOMLEFT: case HTBOTTOMRIGHT:
                        return HTCLIENT;
                    default: break;
                }
            }
            return hit;
        }

        case WM_MOUSEACTIVATE:
        {
            // HACK: Postpone cursor disabling when the window was activated by
            //       clicking a caption button
            if (HIWORD(lParam) == WM_LBUTTONDOWN)
            {
                if (LOWORD(lParam) != HTCLIENT)
                    window->win32.frameAction = GLFW_TRUE;
            }

            break;
        }

        case WM_CAPTURECHANGED:
        {
            // HACK: Disable the cursor once the caption button action has been
            //       completed or cancelled
            if (lParam == 0 && window->win32.frameAction)
            {
                if (window->cursorMode == GLFW_CURSOR_DISABLED)
                    disableCursor(window);
                else if (window->cursorMode == GLFW_CURSOR_CAPTURED)
                    captureCursor(window);

                window->win32.frameAction = GLFW_FALSE;
            }

            break;
        }

        case WM_SETFOCUS:
        {
            _glfwInputWindowFocus(window, GLFW_TRUE);

            // HACK: Do not disable cursor while the user is interacting with
            //       a caption button
            if (window->win32.frameAction)
                break;

            if (window->cursorMode == GLFW_CURSOR_DISABLED)
                disableCursor(window);
            else if (window->cursorMode == GLFW_CURSOR_CAPTURED)
                captureCursor(window);

            return 0;
        }

        case WM_KILLFOCUS:
        {
            if (window->cursorMode == GLFW_CURSOR_DISABLED)
                enableCursor(window);
            else if (window->cursorMode == GLFW_CURSOR_CAPTURED)
                releaseCursor();

            if (window->monitor && window->autoIconify)
                _glfwIconifyWindowWin32(window);

            _glfwInputWindowFocus(window, GLFW_FALSE);
            return 0;
        }

        case WM_SYSCOMMAND:
        {
            switch (wParam & 0xfff0)
            {
                case SC_SCREENSAVE:
                case SC_MONITORPOWER:
                {
                    if (window->monitor)
                    {
                        // We are running in full screen mode, so disallow
                        // screen saver and screen blanking
                        return 0;
                    }
                    else
                        break;
                }
                case SC_SIZE:
                {
                    if (window->win32.snapLayout && !window->resizable)
                        return 0;
                    break;
                }

                // User trying to access application menu using ALT?
                case SC_KEYMENU:
                {
                    if (!window->win32.keymenu)
                        return 0;

                    break;
                }
            }
            break;
        }

        case WM_CLOSE:
        {
            _glfwInputWindowCloseRequest(window);
            return 0;
        }

        case WM_INPUTLANGCHANGE:
        {
            _glfwUpdateKeyNamesWin32();
            break;
        }

        case WM_CHAR:
        case WM_SYSCHAR:
        {
            if (wParam >= 0xd800 && wParam <= 0xdbff)
                window->win32.highSurrogate = (WCHAR) wParam;
            else
            {
                uint32_t codepoint = 0;

                if (wParam >= 0xdc00 && wParam <= 0xdfff)
                {
                    if (window->win32.highSurrogate)
                    {
                        codepoint += (window->win32.highSurrogate - 0xd800) << 10;
                        codepoint += (WCHAR) wParam - 0xdc00;
                        codepoint += 0x10000;
                    }
                }
                else
                    codepoint = (WCHAR) wParam;

                window->win32.highSurrogate = 0;
                _glfwInputChar(window, codepoint, getKeyMods(), uMsg != WM_SYSCHAR);
            }

            if (uMsg == WM_SYSCHAR && window->win32.keymenu)
                break;

            return 0;
        }

        case WM_UNICHAR:
        {
            if (wParam == UNICODE_NOCHAR)
            {
                // WM_UNICHAR is not sent by Windows, but is sent by some
                // third-party input method engine
                // Returning TRUE here announces support for this message
                return TRUE;
            }

            _glfwInputChar(window, (uint32_t) wParam, getKeyMods(), GLFW_TRUE);
            return 0;
        }

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            int key, scancode;
            const int action = (HIWORD(lParam) & KF_UP) ? GLFW_RELEASE : GLFW_PRESS;
            const int mods = getKeyMods();

            scancode = (HIWORD(lParam) & (KF_EXTENDED | 0xff));
            if (!scancode)
            {
                // NOTE: Some synthetic key messages have a scancode of zero
                // HACK: Map the virtual key back to a usable scancode
                scancode = MapVirtualKeyW((UINT) wParam, MAPVK_VK_TO_VSC);
            }

            // HACK: Alt+PrtSc has a different scancode than just PrtSc
            if (scancode == 0x54)
                scancode = 0x137;

            // HACK: Ctrl+Pause has a different scancode than just Pause
            if (scancode == 0x146)
                scancode = 0x45;

            // HACK: CJK IME sets the extended bit for right Shift
            if (scancode == 0x136)
                scancode = 0x36;

            key = _glfw.win32.keycodes[scancode];

            // The Ctrl keys require special handling
            if (wParam == VK_CONTROL)
            {
                if (HIWORD(lParam) & KF_EXTENDED)
                {
                    // Right side keys have the extended key bit set
                    key = GLFW_KEY_RIGHT_CONTROL;
                }
                else
                {
                    // NOTE: Alt Gr sends Left Ctrl followed by Right Alt
                    // HACK: We only want one event for Alt Gr, so if we detect
                    //       this sequence we discard this Left Ctrl message now
                    //       and later report Right Alt normally
                    MSG next;
                    const DWORD time = GetMessageTime();

                    if (PeekMessageW(&next, NULL, 0, 0, PM_NOREMOVE))
                    {
                        if (next.message == WM_KEYDOWN ||
                            next.message == WM_SYSKEYDOWN ||
                            next.message == WM_KEYUP ||
                            next.message == WM_SYSKEYUP)
                        {
                            if (next.wParam == VK_MENU &&
                                (HIWORD(next.lParam) & KF_EXTENDED) &&
                                next.time == time)
                            {
                                // Next message is Right Alt down so discard this
                                break;
                            }
                        }
                    }

                    // This is a regular Left Ctrl message
                    key = GLFW_KEY_LEFT_CONTROL;
                }
            }
            else if (wParam == VK_PROCESSKEY)
            {
                // IME notifies that keys have been filtered by setting the
                // virtual key-code to VK_PROCESSKEY
                break;
            }

            if (action == GLFW_RELEASE && wParam == VK_SHIFT)
            {
                // HACK: Release both Shift keys on Shift up event, as when both
                //       are pressed the first release does not emit any event
                // NOTE: The other half of this is in _glfwPollEventsWin32
                _glfwInputKey(window, GLFW_KEY_LEFT_SHIFT, scancode, action, mods);
                _glfwInputKey(window, GLFW_KEY_RIGHT_SHIFT, scancode, action, mods);
            }
            else if (wParam == VK_SNAPSHOT)
            {
                // HACK: Key down is not reported for the Print Screen key
                _glfwInputKey(window, key, scancode, GLFW_PRESS, mods);
                _glfwInputKey(window, key, scancode, GLFW_RELEASE, mods);
            }
            else
                _glfwInputKey(window, key, scancode, action, mods);

            break;
        }

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        {
            int i, button, action;

            if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP)
                button = GLFW_MOUSE_BUTTON_LEFT;
            else if (uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP)
                button = GLFW_MOUSE_BUTTON_RIGHT;
            else if (uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP)
                button = GLFW_MOUSE_BUTTON_MIDDLE;
            else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
                button = GLFW_MOUSE_BUTTON_4;
            else
                button = GLFW_MOUSE_BUTTON_5;

            if (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN ||
                uMsg == WM_MBUTTONDOWN || uMsg == WM_XBUTTONDOWN)
            {
                action = GLFW_PRESS;
            }
            else
                action = GLFW_RELEASE;

            for (i = 0;  i <= GLFW_MOUSE_BUTTON_LAST;  i++)
            {
                if (window->mouseButtons[i] == GLFW_PRESS)
                    break;
            }

            if (i > GLFW_MOUSE_BUTTON_LAST)
                SetCapture(hWnd);

            _glfwInputMouseClick(window, button, action, getKeyMods());

            for (i = 0;  i <= GLFW_MOUSE_BUTTON_LAST;  i++)
            {
                if (window->mouseButtons[i] == GLFW_PRESS)
                    break;
            }

            if (i > GLFW_MOUSE_BUTTON_LAST)
                ReleaseCapture();

            if (uMsg == WM_XBUTTONDOWN || uMsg == WM_XBUTTONUP)
                return TRUE;

            return 0;
        }

        case WM_MOUSEMOVE:
        {
            const int x = GET_X_LPARAM(lParam);
            const int y = GET_Y_LPARAM(lParam);

            if (!window->win32.cursorTracked)
            {
                TRACKMOUSEEVENT tme;
                ZeroMemory(&tme, sizeof(tme));
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = window->win32.handle;
                TrackMouseEvent(&tme);

                window->win32.cursorTracked = GLFW_TRUE;
                _glfwInputCursorEnter(window, GLFW_TRUE);
            }

            if (window->cursorMode == GLFW_CURSOR_DISABLED)
            {
                const int dx = x - window->win32.lastCursorPosX;
                const int dy = y - window->win32.lastCursorPosY;

                if (_glfw.win32.disabledCursorWindow != window)
                    break;
                if (window->rawMouseMotion)
                    break;

                _glfwInputCursorPos(window,
                                    window->virtualCursorPosX + dx,
                                    window->virtualCursorPosY + dy);
            }
            else
                _glfwInputCursorPos(window, x, y);

            window->win32.lastCursorPosX = x;
            window->win32.lastCursorPosY = y;

            return 0;
        }

        case WM_INPUT:
        {
            UINT size = 0;
            HRAWINPUT ri = (HRAWINPUT) lParam;
            RAWINPUT* data = NULL;
            int dx, dy;

            if (_glfw.win32.disabledCursorWindow != window)
                break;
            if (!window->rawMouseMotion)
                break;

            GetRawInputData(ri, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
            if (size > (UINT) _glfw.win32.rawInputSize)
            {
                _glfw_free(_glfw.win32.rawInput);
                _glfw.win32.rawInput = _glfw_calloc(size, 1);
                _glfw.win32.rawInputSize = size;
            }

            size = _glfw.win32.rawInputSize;
            if (GetRawInputData(ri, RID_INPUT,
                                _glfw.win32.rawInput, &size,
                                sizeof(RAWINPUTHEADER)) == (UINT) -1)
            {
                _glfwInputError(GLFW_PLATFORM_ERROR,
                                "Win32: Failed to retrieve raw input data");
                break;
            }

            data = _glfw.win32.rawInput;
            if (data->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
            {
                POINT pos = {0};
                int width, height;

                if (data->data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP)
                {
                    pos.x += GetSystemMetrics(SM_XVIRTUALSCREEN);
                    pos.y += GetSystemMetrics(SM_YVIRTUALSCREEN);
                    width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                    height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
                }
                else
                {
                    width = GetSystemMetrics(SM_CXSCREEN);
                    height = GetSystemMetrics(SM_CYSCREEN);
                }

                pos.x += (int) ((data->data.mouse.lLastX / 65535.f) * width);
                pos.y += (int) ((data->data.mouse.lLastY / 65535.f) * height);
                ScreenToClient(window->win32.handle, &pos);

                dx = pos.x - window->win32.lastCursorPosX;
                dy = pos.y - window->win32.lastCursorPosY;
            }
            else
            {
                dx = data->data.mouse.lLastX;
                dy = data->data.mouse.lLastY;
            }

            _glfwInputCursorPos(window,
                                window->virtualCursorPosX + dx,
                                window->virtualCursorPosY + dy);

            window->win32.lastCursorPosX += dx;
            window->win32.lastCursorPosY += dy;
            break;
        }

        case WM_MOUSELEAVE:
        {
            window->win32.cursorTracked = GLFW_FALSE;
            _glfwInputCursorEnter(window, GLFW_FALSE);
            return 0;
        }

        case WM_MOUSEWHEEL:
        {
            _glfwInputScroll(window, 0.0, (SHORT) HIWORD(wParam) / (double) WHEEL_DELTA);
            return 0;
        }

        case WM_MOUSEHWHEEL:
        {
            // This message is only sent on Windows Vista and later
            // NOTE: The X-axis is inverted for consistency with macOS and X11
            _glfwInputScroll(window, -((SHORT) HIWORD(wParam) / (double) WHEEL_DELTA), 0.0);
            return 0;
        }

        case WM_TIMER:
        {
            if (wParam == GLFW_TIMER_REFRESH)
            {
                // Drive refresh callbacks while in modal move/size loops so that
                // rendering can be woken even when DefWindowProc enters a modal loop.
                if (window->callbacks.refresh)
                    _glfwInputWindowDamage(window);
                return 0;
            }

            break;
        }

        case WM_ENTERSIZEMOVE:
        case WM_ENTERMENULOOP:
        {
            if (window->win32.frameAction)
                break;

            // HACK: Enable the cursor while the user is moving or
            //       resizing the window or using the window menu
            if (window->cursorMode == GLFW_CURSOR_DISABLED)
                enableCursor(window);
            else if (window->cursorMode == GLFW_CURSOR_CAPTURED)
                releaseCursor();

            // Optional: While inside Win32 modal loops (interactive move/size or menu tracking),
            // generate periodic refresh so that applications can keep repainting.
            if (window->callbacks.refresh && !window->win32.refreshTimerId)
                window->win32.refreshTimerId = SetTimer(window->win32.handle, GLFW_TIMER_REFRESH, 16, NULL);

            break;
        }

        case WM_EXITSIZEMOVE:
        case WM_EXITMENULOOP:
        {
            if (window->win32.frameAction)
                break;

            // HACK: Disable the cursor once the user is done moving or
            //       resizing the window or using the menu
            if (window->cursorMode == GLFW_CURSOR_DISABLED)
                disableCursor(window);
            else if (window->cursorMode == GLFW_CURSOR_CAPTURED)
                captureCursor(window);

            if (window->win32.refreshTimerId)
            {
                KillTimer(window->win32.handle, GLFW_TIMER_REFRESH);
                window->win32.refreshTimerId = 0;
                if (window->callbacks.refresh)
                    _glfwInputWindowDamage(window);
            }

            break;
        }

        case WM_SIZE:
        {
            const int width = LOWORD(lParam);
            const int height = HIWORD(lParam);
            const GLFWbool iconified = wParam == SIZE_MINIMIZED;
            const GLFWbool maximized = wParam == SIZE_MAXIMIZED ||
                                       (window->win32.maximized &&
                                        wParam != SIZE_RESTORED);

            if (_glfw.win32.capturedCursorWindow == window)
                captureCursor(window);

            if (window->win32.iconified != iconified)
                _glfwInputWindowIconify(window, iconified);

            if (window->win32.maximized != maximized)
                _glfwInputWindowMaximize(window, maximized);

            if (width != window->win32.width || height != window->win32.height)
            {
                window->win32.width = width;
                window->win32.height = height;

                _glfwInputFramebufferSize(window, width, height);
                _glfwInputWindowSize(window, width, height);
            }

            if (window->monitor && window->win32.iconified != iconified)
            {
                if (iconified)
                    releaseMonitorWin32(window);
                else
                {
                    acquireMonitorWin32(window);
                    fitToMonitor(window);
                }
            }

            window->win32.iconified = iconified;
            window->win32.maximized = maximized;
            return 0;
        }

        case WM_MOVE:
        {
            if (_glfw.win32.capturedCursorWindow == window)
                captureCursor(window);

            // NOTE: This cannot use LOWORD/HIWORD recommended by MSDN, as
            // those macros do not handle negative window positions correctly
            _glfwInputWindowPos(window,
                                GET_X_LPARAM(lParam),
                                GET_Y_LPARAM(lParam));
            return 0;
        }

        case WM_SIZING:
        {
            if (window->numer == GLFW_DONT_CARE ||
                window->denom == GLFW_DONT_CARE)
            {
                break;
            }

            applyAspectRatio(window, (int) wParam, (RECT*) lParam);
            return TRUE;
        }

        case WM_GETMINMAXINFO:
        {
            RECT frame = {0};
            MINMAXINFO* mmi = (MINMAXINFO*) lParam;
            const DWORD style = getWindowStyle(window);
            const DWORD exStyle = getWindowExStyle(window);

            if (window->monitor)
                break;

            if (_glfwIsWindows10Version1607OrGreaterWin32())
            {
                AdjustWindowRectExForDpi(&frame, style, FALSE, exStyle,
                                         GetDpiForWindow(window->win32.handle));
            }
            else
                AdjustWindowRectEx(&frame, style, FALSE, exStyle);

            if (window->minwidth != GLFW_DONT_CARE &&
                window->minheight != GLFW_DONT_CARE)
            {
                mmi->ptMinTrackSize.x = window->minwidth + frame.right - frame.left;
                mmi->ptMinTrackSize.y = window->minheight + frame.bottom - frame.top;
            }

            if (window->maxwidth != GLFW_DONT_CARE &&
                window->maxheight != GLFW_DONT_CARE)
            {
                mmi->ptMaxTrackSize.x = window->maxwidth + frame.right - frame.left;
                mmi->ptMaxTrackSize.y = window->maxheight + frame.bottom - frame.top;
            }

            if (!window->decorated)
            {
                MONITORINFO mi;
                const HMONITOR mh = MonitorFromWindow(window->win32.handle,
                                                      MONITOR_DEFAULTTONEAREST);

                ZeroMemory(&mi, sizeof(mi));
                mi.cbSize = sizeof(mi);
                GetMonitorInfoW(mh, &mi);

                mmi->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
                mmi->ptMaxPosition.y = mi.rcWork.top - mi.rcMonitor.top;
                mmi->ptMaxSize.x = mi.rcWork.right - mi.rcWork.left;
                mmi->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
            }

            return 0;
        }

        case WM_PAINT:
        {
            _glfwInputWindowDamage(window);
            break;
        }

        case WM_ERASEBKGND:
        {
            return TRUE;
        }

        case WM_NCACTIVATE:
        case WM_NCPAINT:
        {
            // Prevent title bar from being drawn after restoring a minimized
            // undecorated window
            if (!window->decorated)
                return TRUE;

            break;
        }

        case WM_DWMCOMPOSITIONCHANGED:
        case WM_DWMCOLORIZATIONCOLORCHANGED:
        {
            if (window->win32.transparent)
                updateFramebufferTransparency(window);
            return 0;
        }

        case WM_GETDPISCALEDSIZE:
        {
            if (window->win32.scaleToMonitor)
                break;

            // Adjust the window size to keep the content area size constant
            if (_glfwIsWindows10Version1703OrGreaterWin32())
            {
                RECT source = {0}, target = {0};
                SIZE* size = (SIZE*) lParam;

                AdjustWindowRectExForDpi(&source, getWindowStyle(window),
                                         FALSE, getWindowExStyle(window),
                                         GetDpiForWindow(window->win32.handle));
                AdjustWindowRectExForDpi(&target, getWindowStyle(window),
                                         FALSE, getWindowExStyle(window),
                                         LOWORD(wParam));

                size->cx += (target.right - target.left) -
                            (source.right - source.left);
                size->cy += (target.bottom - target.top) -
                            (source.bottom - source.top);
                return TRUE;
            }

            break;
        }

        case WM_DPICHANGED:
        {
            const float xscale = HIWORD(wParam) / (float) USER_DEFAULT_SCREEN_DPI;
            const float yscale = LOWORD(wParam) / (float) USER_DEFAULT_SCREEN_DPI;

            // Resize windowed mode windows that either permit rescaling or that
            // need it to compensate for non-client area scaling
            if (!window->monitor &&
                (window->win32.scaleToMonitor ||
                 _glfwIsWindows10Version1703OrGreaterWin32()))
            {
                RECT* suggested = (RECT*) lParam;
                SetWindowPos(window->win32.handle, HWND_TOP,
                             suggested->left,
                             suggested->top,
                             suggested->right - suggested->left,
                             suggested->bottom - suggested->top,
                             SWP_NOACTIVATE | SWP_NOZORDER);
            }

            _glfwInputWindowContentScale(window, xscale, yscale);
            break;
        }

        case WM_SETCURSOR:
        {
            if (LOWORD(lParam) == HTCLIENT)
            {
                updateCursorImage(window);
                return TRUE;
            }

            break;
        }

        case WM_DROPFILES:
        {
            HDROP drop = (HDROP) wParam;
            POINT pt;
            int i;

            const int count = DragQueryFileW(drop, 0xffffffff, NULL, 0);
            char** paths = _glfw_calloc(count, sizeof(char*));

            // Move the mouse to the position of the drop
            DragQueryPoint(drop, &pt);
            _glfwInputCursorPos(window, pt.x, pt.y);

            for (i = 0;  i < count;  i++)
            {
                const UINT length = DragQueryFileW(drop, i, NULL, 0);
                WCHAR* buffer = _glfw_calloc((size_t) length + 1, sizeof(WCHAR));

                DragQueryFileW(drop, i, buffer, length + 1);
                paths[i] = _glfwCreateUTF8FromWideStringWin32(buffer);

                _glfw_free(buffer);
            }

            _glfwInputDrop(window, count, (const char**) paths);

            for (i = 0;  i < count;  i++)
                _glfw_free(paths[i]);
            _glfw_free(paths);

            DragFinish(drop);
            return 0;
        }
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

// Creates the GLFW window
//
static int createNativeWindow(_GLFWwindow* window,
                              const _GLFWwndconfig* wndconfig,
                              const _GLFWfbconfig* fbconfig)
{
    int frameX, frameY, frameWidth, frameHeight;
    WCHAR* wideTitle;

    // NOTE: Some style decisions (e.g. Snap Layout support) depend on Win32-specific
    // per-window state. Ensure it is populated from wndconfig BEFORE computing the
    // initial styles / creating the native window.
    window->win32.scaleToMonitor = wndconfig->scaleToMonitor;
    window->win32.keymenu = wndconfig->win32.keymenu;
    window->win32.showDefault = wndconfig->win32.showDefault;
    window->win32.snapLayout = wndconfig->win32.snapLayout;



    DWORD style = getWindowStyle(window);
    DWORD exStyle = getWindowExStyle(window);

    window->win32.windowClass = _glfwAcquireWindowClassWin32(wndconfig);
    if (!window->win32.windowClass)
        return GLFW_FALSE;
    if (!_glfwEnsureWindowClassRegisteredWin32(window->win32.windowClass))
    {
        _glfwReleaseWindowClassWin32(window->win32.windowClass);
        window->win32.windowClass = NULL;
        return GLFW_FALSE;
    }

    if (GetSystemMetrics(SM_REMOTESESSION))
    {
        // NOTE: On Remote Desktop, setting the cursor to NULL does not hide it
        // HACK: Create a transparent cursor and always set that instead of NULL
        //       When not on Remote Desktop, this handle is NULL and normal hiding is used
        if (!_glfw.win32.blankCursor)
        {
            const int cursorWidth = GetSystemMetrics(SM_CXCURSOR);
            const int cursorHeight = GetSystemMetrics(SM_CYCURSOR);

            unsigned char* cursorPixels = _glfw_calloc(cursorWidth * cursorHeight, 4);
            if (!cursorPixels)
                return GLFW_FALSE;

            // NOTE: Windows checks whether the image is fully transparent and if so
            //       just ignores the alpha channel and makes the whole cursor opaque
            // HACK: Make one pixel slightly less transparent
            cursorPixels[3] = 1;

            const GLFWimage cursorImage = { cursorWidth, cursorHeight, cursorPixels };
            _glfw.win32.blankCursor = createIcon(&cursorImage, 0, 0, FALSE);
            _glfw_free(cursorPixels);

            if (!_glfw.win32.blankCursor)
                return GLFW_FALSE;
        }
    }

    if (window->monitor)
    {
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfoW(window->monitor->win32.handle, &mi);

        // NOTE: This window placement is temporary and approximate, as the
        //       correct position and size cannot be known until the monitor
        //       video mode has been picked in _glfwSetVideoModeWin32
        frameX = mi.rcMonitor.left;
        frameY = mi.rcMonitor.top;
        frameWidth  = mi.rcMonitor.right - mi.rcMonitor.left;
        frameHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;
    }
    else
    {
        RECT rect = { 0, 0, wndconfig->width, wndconfig->height };

        window->win32.maximized = wndconfig->maximized;
        if (wndconfig->maximized)
            style |= WS_MAXIMIZE;

        AdjustWindowRectEx(&rect, style, FALSE, exStyle);

        if (wndconfig->xpos == GLFW_ANY_POSITION && wndconfig->ypos == GLFW_ANY_POSITION)
        {
            frameX = CW_USEDEFAULT;
            frameY = CW_USEDEFAULT;
        }
        else
        {
            frameX = wndconfig->xpos + rect.left;
            frameY = wndconfig->ypos + rect.top;
        }

        frameWidth  = rect.right - rect.left;
        frameHeight = rect.bottom - rect.top;
    }

    wideTitle = _glfwCreateWideStringFromUTF8Win32(wndconfig->title);
    if (!wideTitle)
    {
        _glfwReleaseWindowClassWin32(window->win32.windowClass);
        window->win32.windowClass = NULL;
        return GLFW_FALSE;
    }

    window->win32.handle = CreateWindowExW(exStyle,
                                           window->win32.windowClass->name,
                                           wideTitle,
                                           style,
                                           frameX, frameY,
                                           frameWidth, frameHeight,
                                           NULL, // No parent window
                                           NULL, // No window menu
                                           _glfw.win32.instance,
                                           (LPVOID) wndconfig);

    _glfw_free(wideTitle);

    if (!window->win32.handle)
    {
        _glfwReleaseWindowClassWin32(window->win32.windowClass);
        window->win32.windowClass = NULL;
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                             "Win32: Failed to create window");
        return GLFW_FALSE;
    }

    SetPropW(window->win32.handle, L"GLFW", window);

    if (IsWindows7OrGreater())
    {
        ChangeWindowMessageFilterEx(window->win32.handle,
                                    WM_DROPFILES, MSGFLT_ALLOW, NULL);
        ChangeWindowMessageFilterEx(window->win32.handle,
                                    WM_COPYDATA, MSGFLT_ALLOW, NULL);
        ChangeWindowMessageFilterEx(window->win32.handle,
                                    WM_COPYGLOBALDATA, MSGFLT_ALLOW, NULL);
    }


    if (!window->monitor)
    {
        RECT rect = { 0, 0, wndconfig->width, wndconfig->height };
        WINDOWPLACEMENT wp = { sizeof(wp) };
        const HMONITOR mh = MonitorFromWindow(window->win32.handle,
                                              MONITOR_DEFAULTTONEAREST);

        // Adjust window rect to account for DPI scaling of the window frame and
        // (if enabled) DPI scaling of the content area
        // This cannot be done until we know what monitor the window was placed on
        // Only update the restored window rect as the window may be maximized

        if (wndconfig->scaleToMonitor)
        {
            float xscale, yscale;
            _glfwGetHMONITORContentScaleWin32(mh, &xscale, &yscale);

            if (xscale > 0.f && yscale > 0.f)
            {
                rect.right = (int) (rect.right * xscale);
                rect.bottom = (int) (rect.bottom * yscale);
            }
        }

        if (_glfwIsWindows10Version1607OrGreaterWin32())
        {
            AdjustWindowRectExForDpi(&rect, style, FALSE, exStyle,
                                     GetDpiForWindow(window->win32.handle));
        }
        else
            AdjustWindowRectEx(&rect, style, FALSE, exStyle);

        GetWindowPlacement(window->win32.handle, &wp);
        OffsetRect(&rect,
                   wp.rcNormalPosition.left - rect.left,
                   wp.rcNormalPosition.top - rect.top);

        wp.rcNormalPosition = rect;
        wp.showCmd = SW_HIDE;
        SetWindowPlacement(window->win32.handle, &wp);

        // Adjust rect of maximized undecorated window, because by default Windows will
        // make such a window cover the whole monitor instead of its workarea

        if (wndconfig->maximized && !wndconfig->decorated)
        {
            MONITORINFO mi = { sizeof(mi) };
            GetMonitorInfoW(mh, &mi);

            SetWindowPos(window->win32.handle, HWND_TOP,
                         mi.rcWork.left,
                         mi.rcWork.top,
                         mi.rcWork.right - mi.rcWork.left,
                         mi.rcWork.bottom - mi.rcWork.top,
                         SWP_NOACTIVATE | SWP_NOZORDER);
        }
    }

    DragAcceptFiles(window->win32.handle, TRUE);

    if (fbconfig->transparent)
    {
        updateFramebufferTransparency(window);
        window->win32.transparent = GLFW_TRUE;
    }

    _glfwGetWindowSizeWin32(window, &window->win32.width, &window->win32.height);

    return GLFW_TRUE;
}

GLFWbool _glfwCreateWindowWin32(_GLFWwindow* window,
                                const _GLFWwndconfig* wndconfig,
                                const _GLFWctxconfig* ctxconfig,
                                const _GLFWfbconfig* fbconfig)
{
    if (!createNativeWindow(window, wndconfig, fbconfig))
        return GLFW_FALSE;

    if (ctxconfig->client != GLFW_NO_API)
    {
        if (ctxconfig->source == GLFW_NATIVE_CONTEXT_API)
        {
            if (!_glfwInitWGL())
                return GLFW_FALSE;
            if (!_glfwCreateContextWGL(window, ctxconfig, fbconfig))
                return GLFW_FALSE;
        }
        else if (ctxconfig->source == GLFW_EGL_CONTEXT_API)
        {
            if (!_glfwInitEGL())
                return GLFW_FALSE;
            if (!_glfwCreateContextEGL(window, ctxconfig, fbconfig))
                return GLFW_FALSE;
        }
        else if (ctxconfig->source == GLFW_OSMESA_CONTEXT_API)
        {
            if (!_glfwInitOSMesa())
                return GLFW_FALSE;
            if (!_glfwCreateContextOSMesa(window, ctxconfig, fbconfig))
                return GLFW_FALSE;
        }

        if (!_glfwRefreshContextAttribs(window, ctxconfig))
            return GLFW_FALSE;
    }

    if (wndconfig->mousePassthrough)
        _glfwSetWindowMousePassthroughWin32(window, GLFW_TRUE);

    if (window->monitor)
    {
        _glfwShowWindowWin32(window);
        _glfwFocusWindowWin32(window);
        acquireMonitorWin32(window);
        fitToMonitor(window);

        if (wndconfig->centerCursor)
            _glfwCenterCursorInContentArea(window);
    }
    else
    {
        if (wndconfig->visible)
        {
            _glfwShowWindowWin32(window);
            if (wndconfig->focused)
                _glfwFocusWindowWin32(window);
        }
    }

    return GLFW_TRUE;
}

void _glfwDestroyWindowWin32(_GLFWwindow* window)
{
    if (window->monitor)
        releaseMonitorWin32(window);

    if (window->context.destroy)
        window->context.destroy(window);

    if (_glfw.win32.disabledCursorWindow == window)
        enableCursor(window);

    if (_glfw.win32.capturedCursorWindow == window)
        releaseCursor();

    if (window->win32.handle)
    {
        RemovePropW(window->win32.handle, L"GLFW");
        DestroyWindow(window->win32.handle);
        window->win32.handle = NULL;
    }

    // Free any registered message hooks
    if (window->win32.messageHooks || window->win32.pendingHookFrees)
    {
        _GLFWwin32MessageHook* hooks;
        _GLFWwin32MessageHook* pending;

        if (_glfw.win32.hookLock)
            _glfwPlatformLockMutex(_glfw.win32.hookLock);

        hooks = window->win32.messageHooks;
        pending = window->win32.pendingHookFrees;
        window->win32.messageHooks = NULL;
        window->win32.pendingHookFrees = NULL;
        window->win32.hookDispatchDepth = 0;

        if (_glfw.win32.hookLock)
            _glfwPlatformUnlockMutex(_glfw.win32.hookLock);

        while (hooks)
        {
            _GLFWwin32MessageHook* next = hooks->next;
            _glfw_free(hooks);
            hooks = next;
        }
        while (pending)
        {
            _GLFWwin32MessageHook* next = pending->next;
            _glfw_free(pending);
            pending = next;
        }
    }

    if (window->win32.windowClass)
    {
        _glfwReleaseWindowClassWin32(window->win32.windowClass);
        window->win32.windowClass = NULL;
    }

    if (window->win32.bigIcon)
        DestroyIcon(window->win32.bigIcon);

    if (window->win32.smallIcon)
        DestroyIcon(window->win32.smallIcon);
}

void _glfwSetWindowTitleWin32(_GLFWwindow* window, const char* title)
{
    WCHAR* wideTitle = _glfwCreateWideStringFromUTF8Win32(title);
    if (!wideTitle)
        return;

    SetWindowTextW(window->win32.handle, wideTitle);
    _glfw_free(wideTitle);
}

void _glfwSetWindowIconWin32(_GLFWwindow* window, int count, const GLFWimage* images)
{
    HICON bigIcon = NULL, smallIcon = NULL;

    if (count)
    {
        const GLFWimage* bigImage = chooseImage(count, images,
                                                GetSystemMetrics(SM_CXICON),
                                                GetSystemMetrics(SM_CYICON));
        const GLFWimage* smallImage = chooseImage(count, images,
                                                  GetSystemMetrics(SM_CXSMICON),
                                                  GetSystemMetrics(SM_CYSMICON));

        bigIcon = createIcon(bigImage, 0, 0, GLFW_TRUE);
        smallIcon = createIcon(smallImage, 0, 0, GLFW_TRUE);
    }
    else
    {
        bigIcon = (HICON) GetClassLongPtrW(window->win32.handle, GCLP_HICON);
        smallIcon = (HICON) GetClassLongPtrW(window->win32.handle, GCLP_HICONSM);
    }

    SendMessageW(window->win32.handle, WM_SETICON, ICON_BIG, (LPARAM) bigIcon);
    SendMessageW(window->win32.handle, WM_SETICON, ICON_SMALL, (LPARAM) smallIcon);

    if (window->win32.bigIcon)
        DestroyIcon(window->win32.bigIcon);

    if (window->win32.smallIcon)
        DestroyIcon(window->win32.smallIcon);

    if (count)
    {
        window->win32.bigIcon = bigIcon;
        window->win32.smallIcon = smallIcon;
    }
}

void _glfwGetWindowPosWin32(_GLFWwindow* window, int* xpos, int* ypos)
{
    POINT pos = { 0, 0 };
    ClientToScreen(window->win32.handle, &pos);

    if (xpos)
        *xpos = pos.x;
    if (ypos)
        *ypos = pos.y;
}

void _glfwSetWindowPosWin32(_GLFWwindow* window, int xpos, int ypos)
{
    RECT rect = { xpos, ypos, xpos, ypos };

    if (_glfwIsWindows10Version1607OrGreaterWin32())
    {
        AdjustWindowRectExForDpi(&rect, getWindowStyle(window),
                                 FALSE, getWindowExStyle(window),
                                 GetDpiForWindow(window->win32.handle));
    }
    else
    {
        AdjustWindowRectEx(&rect, getWindowStyle(window),
                           FALSE, getWindowExStyle(window));
    }

    SetWindowPos(window->win32.handle, NULL, rect.left, rect.top, 0, 0,
                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
}

void _glfwGetWindowSizeWin32(_GLFWwindow* window, int* width, int* height)
{
    RECT area;
    GetClientRect(window->win32.handle, &area);

    if (width)
        *width = area.right;
    if (height)
        *height = area.bottom;
}

void _glfwSetWindowSizeWin32(_GLFWwindow* window, int width, int height)
{
    if (window->monitor)
    {
        if (window->monitor->window == window)
        {
            acquireMonitorWin32(window);
            fitToMonitor(window);
        }
    }
    else
    {
        RECT rect = { 0, 0, width, height };

        if (_glfwIsWindows10Version1607OrGreaterWin32())
        {
            AdjustWindowRectExForDpi(&rect, getWindowStyle(window),
                                     FALSE, getWindowExStyle(window),
                                     GetDpiForWindow(window->win32.handle));
        }
        else
        {
            AdjustWindowRectEx(&rect, getWindowStyle(window),
                               FALSE, getWindowExStyle(window));
        }

        SetWindowPos(window->win32.handle, HWND_TOP,
                     0, 0, rect.right - rect.left, rect.bottom - rect.top,
                     SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
    }
}

void _glfwSetWindowSizeLimitsWin32(_GLFWwindow* window,
                                   int minwidth, int minheight,
                                   int maxwidth, int maxheight)
{
    RECT area;

    if ((minwidth == GLFW_DONT_CARE || minheight == GLFW_DONT_CARE) &&
        (maxwidth == GLFW_DONT_CARE || maxheight == GLFW_DONT_CARE))
    {
        return;
    }

    GetWindowRect(window->win32.handle, &area);
    MoveWindow(window->win32.handle,
               area.left, area.top,
               area.right - area.left,
               area.bottom - area.top, TRUE);
}

void _glfwSetWindowAspectRatioWin32(_GLFWwindow* window, int numer, int denom)
{
    RECT area;

    if (numer == GLFW_DONT_CARE || denom == GLFW_DONT_CARE)
        return;

    GetWindowRect(window->win32.handle, &area);
    applyAspectRatio(window, WMSZ_BOTTOMRIGHT, &area);
    MoveWindow(window->win32.handle,
               area.left, area.top,
               area.right - area.left,
               area.bottom - area.top, TRUE);
}

void _glfwGetFramebufferSizeWin32(_GLFWwindow* window, int* width, int* height)
{
    _glfwGetWindowSizeWin32(window, width, height);
}

void _glfwGetWindowFrameSizeWin32(_GLFWwindow* window,
                                  int* left, int* top,
                                  int* right, int* bottom)
{
    RECT rect;
    int width, height;

    _glfwGetWindowSizeWin32(window, &width, &height);
    SetRect(&rect, 0, 0, width, height);

    if (_glfwIsWindows10Version1607OrGreaterWin32())
    {
        AdjustWindowRectExForDpi(&rect, getWindowStyle(window),
                                 FALSE, getWindowExStyle(window),
                                 GetDpiForWindow(window->win32.handle));
    }
    else
    {
        AdjustWindowRectEx(&rect, getWindowStyle(window),
                           FALSE, getWindowExStyle(window));
    }

    if (left)
        *left = -rect.left;
    if (top)
        *top = -rect.top;
    if (right)
        *right = rect.right - width;
    if (bottom)
        *bottom = rect.bottom - height;
}

void _glfwGetWindowContentScaleWin32(_GLFWwindow* window, float* xscale, float* yscale)
{
    const HANDLE handle = MonitorFromWindow(window->win32.handle,
                                            MONITOR_DEFAULTTONEAREST);
    _glfwGetHMONITORContentScaleWin32(handle, xscale, yscale);
}

void _glfwIconifyWindowWin32(_GLFWwindow* window)
{
    ShowWindow(window->win32.handle, SW_MINIMIZE);
}

void _glfwRestoreWindowWin32(_GLFWwindow* window)
{
    ShowWindow(window->win32.handle, SW_RESTORE);
}

void _glfwMaximizeWindowWin32(_GLFWwindow* window)
{
    if (IsWindowVisible(window->win32.handle))
        ShowWindow(window->win32.handle, SW_MAXIMIZE);
    else
        maximizeWindowManually(window);
}

void _glfwShowWindowWin32(_GLFWwindow* window)
{
    int showCommand = SW_SHOWNA;

    if (window->win32.showDefault)
    {
        // NOTE: GLFW windows currently do not seem to match the Windows 10 definition of
        //       a main window, so even SW_SHOWDEFAULT does nothing
        //       This definition is undocumented and can change (source: Raymond Chen)
        // HACK: Apply the STARTUPINFO show command manually if available
        STARTUPINFOW si = { sizeof(si) };
        GetStartupInfoW(&si);
        if (si.dwFlags & STARTF_USESHOWWINDOW)
            showCommand = si.wShowWindow;

        window->win32.showDefault = GLFW_FALSE;
    }

    ShowWindow(window->win32.handle, showCommand);
}

void _glfwHideWindowWin32(_GLFWwindow* window)
{
    ShowWindow(window->win32.handle, SW_HIDE);
}

void _glfwRequestWindowAttentionWin32(_GLFWwindow* window)
{
    FlashWindow(window->win32.handle, TRUE);
}

void _glfwFocusWindowWin32(_GLFWwindow* window)
{
    BringWindowToTop(window->win32.handle);
    SetForegroundWindow(window->win32.handle);
    SetFocus(window->win32.handle);
}

void _glfwSetWindowMonitorWin32(_GLFWwindow* window,
                                _GLFWmonitor* monitor,
                                int xpos, int ypos,
                                int width, int height,
                                int refreshRate)
{
    if (window->monitor == monitor)
    {
        if (monitor)
        {
            if (monitor->window == window)
            {
                acquireMonitorWin32(window);
                fitToMonitor(window);
            }
        }
        else
        {
            RECT rect = { xpos, ypos, xpos + width, ypos + height };

            if (_glfwIsWindows10Version1607OrGreaterWin32())
            {
                AdjustWindowRectExForDpi(&rect, getWindowStyle(window),
                                         FALSE, getWindowExStyle(window),
                                         GetDpiForWindow(window->win32.handle));
            }
            else
            {
                AdjustWindowRectEx(&rect, getWindowStyle(window),
                                   FALSE, getWindowExStyle(window));
            }

            SetWindowPos(window->win32.handle, HWND_TOP,
                         rect.left, rect.top,
                         rect.right - rect.left, rect.bottom - rect.top,
                         SWP_NOCOPYBITS | SWP_NOACTIVATE | SWP_NOZORDER);
        }

        return;
    }

    if (window->monitor)
        releaseMonitorWin32(window);

    _glfwInputWindowMonitor(window, monitor);

    if (window->monitor)
    {
        MONITORINFO mi = { sizeof(mi) };
        UINT flags = SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS;

        if (window->decorated)
        {
            DWORD style = GetWindowLongW(window->win32.handle, GWL_STYLE);
            style &= ~WS_OVERLAPPEDWINDOW;
            style |= getWindowStyle(window);
            SetWindowLongW(window->win32.handle, GWL_STYLE, style);
            flags |= SWP_FRAMECHANGED;
        }

        acquireMonitorWin32(window);

        GetMonitorInfoW(window->monitor->win32.handle, &mi);
        SetWindowPos(window->win32.handle, HWND_TOPMOST,
                     mi.rcMonitor.left,
                     mi.rcMonitor.top,
                     mi.rcMonitor.right - mi.rcMonitor.left,
                     mi.rcMonitor.bottom - mi.rcMonitor.top,
                     flags);
    }
    else
    {
        HWND after;
        RECT rect = { xpos, ypos, xpos + width, ypos + height };
        DWORD style = GetWindowLongW(window->win32.handle, GWL_STYLE);
        UINT flags = SWP_NOACTIVATE | SWP_NOCOPYBITS;

        if (window->decorated)
        {
            style &= ~WS_POPUP;
            style |= getWindowStyle(window);
            SetWindowLongW(window->win32.handle, GWL_STYLE, style);

            flags |= SWP_FRAMECHANGED;
        }

        if (window->floating)
            after = HWND_TOPMOST;
        else
            after = HWND_NOTOPMOST;

        if (_glfwIsWindows10Version1607OrGreaterWin32())
        {
            AdjustWindowRectExForDpi(&rect, getWindowStyle(window),
                                     FALSE, getWindowExStyle(window),
                                     GetDpiForWindow(window->win32.handle));
        }
        else
        {
            AdjustWindowRectEx(&rect, getWindowStyle(window),
                               FALSE, getWindowExStyle(window));
        }

        SetWindowPos(window->win32.handle, after,
                     rect.left, rect.top,
                     rect.right - rect.left, rect.bottom - rect.top,
                     flags);
    }
}

GLFWbool _glfwWindowFocusedWin32(_GLFWwindow* window)
{
    return window->win32.handle == GetActiveWindow();
}

GLFWbool _glfwWindowIconifiedWin32(_GLFWwindow* window)
{
    return IsIconic(window->win32.handle);
}

GLFWbool _glfwWindowVisibleWin32(_GLFWwindow* window)
{
    return IsWindowVisible(window->win32.handle);
}

GLFWbool _glfwWindowMaximizedWin32(_GLFWwindow* window)
{
    return IsZoomed(window->win32.handle);
}

GLFWbool _glfwWindowHoveredWin32(_GLFWwindow* window)
{
    return cursorInContentArea(window);
}

GLFWbool _glfwFramebufferTransparentWin32(_GLFWwindow* window)
{
    BOOL composition, opaque;
    DWORD color;

    if (!window->win32.transparent)
        return GLFW_FALSE;

    if (!IsWindowsVistaOrGreater())
        return GLFW_FALSE;

    if (FAILED(DwmIsCompositionEnabled(&composition)) || !composition)
        return GLFW_FALSE;

    if (!IsWindows8OrGreater())
    {
        // HACK: Disable framebuffer transparency on Windows 7 when the
        //       colorization color is opaque, because otherwise the window
        //       contents is blended additively with the previous frame instead
        //       of replacing it
        if (FAILED(DwmGetColorizationColor(&color, &opaque)) || opaque)
            return GLFW_FALSE;
    }

    return GLFW_TRUE;
}

void _glfwSetWindowResizableWin32(_GLFWwindow* window, GLFWbool enabled)
{
    updateWindowStyles(window);
}

void _glfwSetWindowSnapLayoutWin32(_GLFWwindow* window, GLFWbool enabled)
{
    if (window->win32.snapLayout == enabled)
        return;

    window->win32.snapLayout = enabled;

    // Only affects windowed mode. Fullscreen uses monitor and ignores frame styles.
    if (!window->monitor)
        updateWindowStyles(window);
}


void _glfwSetWindowDecoratedWin32(_GLFWwindow* window, GLFWbool enabled)
{
    updateWindowStyles(window);
}

void _glfwSetWindowFloatingWin32(_GLFWwindow* window, GLFWbool enabled)
{
    const HWND after = enabled ? HWND_TOPMOST : HWND_NOTOPMOST;
    SetWindowPos(window->win32.handle, after, 0, 0, 0, 0,
                 SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

void _glfwSetWindowMousePassthroughWin32(_GLFWwindow* window, GLFWbool enabled)
{
    COLORREF key = 0;
    BYTE alpha = 0;
    DWORD flags = 0;
    DWORD exStyle = GetWindowLongW(window->win32.handle, GWL_EXSTYLE);

    if (exStyle & WS_EX_LAYERED)
        GetLayeredWindowAttributes(window->win32.handle, &key, &alpha, &flags);

    if (enabled)
        exStyle |= (WS_EX_TRANSPARENT | WS_EX_LAYERED);
    else
    {
        exStyle &= ~WS_EX_TRANSPARENT;
        // NOTE: Window opacity also needs the layered window style so do not
        //       remove it if the window is alpha blended
        if (exStyle & WS_EX_LAYERED)
        {
            if (!(flags & LWA_ALPHA))
                exStyle &= ~WS_EX_LAYERED;
        }
    }

    SetWindowLongW(window->win32.handle, GWL_EXSTYLE, exStyle);

    if (enabled)
        SetLayeredWindowAttributes(window->win32.handle, key, alpha, flags);
}

float _glfwGetWindowOpacityWin32(_GLFWwindow* window)
{
    BYTE alpha;
    DWORD flags;

    if ((GetWindowLongW(window->win32.handle, GWL_EXSTYLE) & WS_EX_LAYERED) &&
        GetLayeredWindowAttributes(window->win32.handle, NULL, &alpha, &flags))
    {
        if (flags & LWA_ALPHA)
            return alpha / 255.f;
    }

    return 1.f;
}

void _glfwSetWindowOpacityWin32(_GLFWwindow* window, float opacity)
{
    LONG exStyle = GetWindowLongW(window->win32.handle, GWL_EXSTYLE);
    if (opacity < 1.f || (exStyle & WS_EX_TRANSPARENT))
    {
        const BYTE alpha = (BYTE) (255 * opacity);
        exStyle |= WS_EX_LAYERED;
        SetWindowLongW(window->win32.handle, GWL_EXSTYLE, exStyle);
        SetLayeredWindowAttributes(window->win32.handle, 0, alpha, LWA_ALPHA);
    }
    else if (exStyle & WS_EX_TRANSPARENT)
    {
        SetLayeredWindowAttributes(window->win32.handle, 0, 0, 0);
    }
    else
    {
        exStyle &= ~WS_EX_LAYERED;
        SetWindowLongW(window->win32.handle, GWL_EXSTYLE, exStyle);
    }
}

void _glfwSetRawMouseMotionWin32(_GLFWwindow *window, GLFWbool enabled)
{
    if (_glfw.win32.disabledCursorWindow != window)
        return;

    if (enabled)
        enableRawMouseMotion(window);
    else
        disableRawMouseMotion(window);
}

GLFWbool _glfwRawMouseMotionSupportedWin32(void)
{
    return GLFW_TRUE;
}

void _glfwPollEventsWin32(void)
{
    MSG msg;
    HWND handle;
    _GLFWwindow* window;

    // Ensure this thread is registered for event wake-ups
    (void) _glfwGetThreadContextWin32();

    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            // NOTE: While GLFW does not itself post WM_QUIT, other processes
            //       may post it to this one, for example Task Manager
            // NOTE: Treat WM_QUIT as a close request for windows owned by this thread

            const DWORD tid = GetCurrentThreadId();
            int count = 0, i = 0;
            _GLFWwindow** list = NULL;

            _glfwPlatformLockMutex(&_glfw.windowListLock);
            for (window = _glfw.windowListHead; window; window = window->next)
            {
                if (window->win32.handle &&
                    GetWindowThreadProcessId(window->win32.handle, NULL) == tid)
                {
                    count++;
                }
            }
            _glfwPlatformUnlockMutex(&_glfw.windowListLock);

            if (count > 0)
                list = _glfw_calloc(count, sizeof(*list));

            if (list)
            {
                _glfwPlatformLockMutex(&_glfw.windowListLock);
                for (window = _glfw.windowListHead; window; window = window->next)
                {
                    if (window->win32.handle &&
                        GetWindowThreadProcessId(window->win32.handle, NULL) == tid)
                    {
                        if (i < count)
                            list[i++] = window;
                    }
                }
                _glfwPlatformUnlockMutex(&_glfw.windowListLock);

                for (i = 0; i < count; i++)
                    _glfwInputWindowCloseRequest(list[i]);

                _glfw_free(list);
            }
            else
            {
                // Fallback: best-effort without snapshot (avoid holding the list lock across callbacks)
                _glfwPlatformLockMutex(&_glfw.windowListLock);
                for (window = _glfw.windowListHead; window; window = window->next)
                {
                    if (!window->win32.handle)
                        continue;
                    if (GetWindowThreadProcessId(window->win32.handle, NULL) != tid)
                        continue;
                    // Mark close request without invoking user callback directly
                    window->shouldClose = GLFW_TRUE;
                }
                _glfwPlatformUnlockMutex(&_glfw.windowListLock);
            }
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    // HACK: Release modifier keys that the system did not emit KEYUP for
    // NOTE: Shift keys on Windows tend to "stick" when both are pressed as
    //       no key up message is generated by the first key release
    // NOTE: Windows key is not reported as released by the Win+V hotkey
    //       Other Win hotkeys are handled implicitly by _glfwInputWindowFocus
    //       because they change the input focus
    // NOTE: The other half of this is in the WM_*KEY* handler in windowProc
    handle = GetActiveWindow();
    if (handle)
    {
        window = GetPropW(handle, L"GLFW");
        if (window)
        {
            int i;
            const int keys[4][2] =
            {
                { VK_LSHIFT, GLFW_KEY_LEFT_SHIFT },
                { VK_RSHIFT, GLFW_KEY_RIGHT_SHIFT },
                { VK_LWIN, GLFW_KEY_LEFT_SUPER },
                { VK_RWIN, GLFW_KEY_RIGHT_SUPER }
            };

            for (i = 0;  i < 4;  i++)
            {
                const int vk = keys[i][0];
                const int key = keys[i][1];
                const int scancode = _glfw.win32.scancodes[key];

                if ((GetKeyState(vk) & 0x8000))
                    continue;
                if (window->keys[key] != GLFW_PRESS)
                    continue;

                _glfwInputKey(window, key, scancode, GLFW_RELEASE, getKeyMods());
            }
        }
    }

    window = _glfw.win32.disabledCursorWindow;
    if (window)
    {
        int width, height;
        _glfwGetWindowSizeWin32(window, &width, &height);

        // NOTE: Re-center the cursor only if it has moved since the last call,
        //       to avoid breaking glfwWaitEvents with WM_MOUSEMOVE
        // The re-center is required in order to prevent the mouse cursor stopping at the edges of the screen.
        if (window->win32.lastCursorPosX != width / 2 ||
            window->win32.lastCursorPosY != height / 2)
        {
            _glfwSetCursorPosWin32(window, width / 2, height / 2);
        }
    }
}

void _glfwWaitEventsWin32(void)
{
    _GLFWwin32ThreadContext* ctx = _glfwGetThreadContextWin32();
    HANDLE handle = ctx ? ctx->wakeEvent : NULL;

    if (handle)
        MsgWaitForMultipleObjectsEx(1, &handle, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    else
        WaitMessage();

    _glfwPollEventsWin32();
}

void _glfwWaitEventsTimeoutWin32(double timeout)
{
    _GLFWwin32ThreadContext* ctx = _glfwGetThreadContextWin32();
    HANDLE handle = ctx ? ctx->wakeEvent : NULL;
    DWORD millis = (DWORD) (timeout * 1e3);

    if (handle)
        MsgWaitForMultipleObjectsEx(1, &handle, millis, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    else
        MsgWaitForMultipleObjects(0, NULL, FALSE, millis, QS_ALLINPUT);

    _glfwPollEventsWin32();
}

void _glfwPostEmptyEventWin32(void)
{
    // Wake all threads currently pumping GLFW events
    wakeAllThreadsWin32();

    // Also poke the helper window message queue (legacy behavior)
    PostMessageW(_glfw.win32.helperWindowHandle, WM_NULL, 0, 0);
}

void _glfwGetCursorPosWin32(_GLFWwindow* window, double* xpos, double* ypos)
{
    POINT pos;

    if (GetCursorPos(&pos))
    {
        ScreenToClient(window->win32.handle, &pos);

        if (xpos)
            *xpos = pos.x;
        if (ypos)
            *ypos = pos.y;
    }
}

void _glfwSetCursorPosWin32(_GLFWwindow* window, double xpos, double ypos)
{
    POINT pos = { (int) xpos, (int) ypos };

    // Store the new position so it can be recognized later
    window->win32.lastCursorPosX = pos.x;
    window->win32.lastCursorPosY = pos.y;

    ClientToScreen(window->win32.handle, &pos);
    SetCursorPos(pos.x, pos.y);
}

void _glfwSetCursorModeWin32(_GLFWwindow* window, int mode)
{
    if (_glfwWindowFocusedWin32(window))
    {
        if (mode == GLFW_CURSOR_DISABLED)
        {
            _glfwGetCursorPosWin32(window,
                                   &_glfw.win32.restoreCursorPosX,
                                   &_glfw.win32.restoreCursorPosY);
            _glfwCenterCursorInContentArea(window);
            if (window->rawMouseMotion)
                enableRawMouseMotion(window);
        }
        else if (_glfw.win32.disabledCursorWindow == window)
        {
            if (window->rawMouseMotion)
                disableRawMouseMotion(window);
        }

        if (mode == GLFW_CURSOR_DISABLED || mode == GLFW_CURSOR_CAPTURED)
            captureCursor(window);
        else
            releaseCursor();

        if (mode == GLFW_CURSOR_DISABLED)
            _glfw.win32.disabledCursorWindow = window;
        else if (_glfw.win32.disabledCursorWindow == window)
        {
            _glfw.win32.disabledCursorWindow = NULL;
            _glfwSetCursorPosWin32(window,
                                   _glfw.win32.restoreCursorPosX,
                                   _glfw.win32.restoreCursorPosY);
        }
    }

    if (cursorInContentArea(window))
        updateCursorImage(window);
}

const char* _glfwGetScancodeNameWin32(int scancode)
{
    if (scancode < 0 || scancode > (KF_EXTENDED | 0xff))
    {
        _glfwInputError(GLFW_INVALID_VALUE, "Invalid scancode %i", scancode);
        return NULL;
    }

    const int key = _glfw.win32.keycodes[scancode];
    if (key == GLFW_KEY_UNKNOWN)
        return NULL;

    return _glfw.win32.keynames[key];
}

int _glfwGetKeyScancodeWin32(int key)
{
    return _glfw.win32.scancodes[key];
}

GLFWbool _glfwCreateCursorWin32(_GLFWcursor* cursor,
                                const GLFWimage* image,
                                int xhot, int yhot)
{
    cursor->win32.handle = (HCURSOR) createIcon(image, xhot, yhot, GLFW_FALSE);
    if (!cursor->win32.handle)
        return GLFW_FALSE;

    return GLFW_TRUE;
}

GLFWbool _glfwCreateStandardCursorWin32(_GLFWcursor* cursor, int shape)
{
    int id = 0;

    switch (shape)
    {
        case GLFW_ARROW_CURSOR:
            id = OCR_NORMAL;
            break;
        case GLFW_IBEAM_CURSOR:
            id = OCR_IBEAM;
            break;
        case GLFW_CROSSHAIR_CURSOR:
            id = OCR_CROSS;
            break;
        case GLFW_POINTING_HAND_CURSOR:
            id = OCR_HAND;
            break;
        case GLFW_RESIZE_EW_CURSOR:
            id = OCR_SIZEWE;
            break;
        case GLFW_RESIZE_NS_CURSOR:
            id = OCR_SIZENS;
            break;
        case GLFW_RESIZE_NWSE_CURSOR:
            id = OCR_SIZENWSE;
            break;
        case GLFW_RESIZE_NESW_CURSOR:
            id = OCR_SIZENESW;
            break;
        case GLFW_RESIZE_ALL_CURSOR:
            id = OCR_SIZEALL;
            break;
        case GLFW_NOT_ALLOWED_CURSOR:
            id = OCR_NO;
            break;
        default:
            _glfwInputError(GLFW_PLATFORM_ERROR, "Win32: Unknown standard cursor");
            return GLFW_FALSE;
    }

    cursor->win32.handle = LoadImageW(NULL,
                                      MAKEINTRESOURCEW(id), IMAGE_CURSOR, 0, 0,
                                      LR_DEFAULTSIZE | LR_SHARED);
    if (!cursor->win32.handle)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                             "Win32: Failed to create standard cursor");
        return GLFW_FALSE;
    }

    return GLFW_TRUE;
}

void _glfwDestroyCursorWin32(_GLFWcursor* cursor)
{
    if (cursor->win32.handle)
        DestroyIcon((HICON) cursor->win32.handle);
}

void _glfwSetCursorWin32(_GLFWwindow* window, _GLFWcursor* cursor)
{
    if (cursorInContentArea(window))
        updateCursorImage(window);
}

void _glfwSetClipboardStringWin32(const char* string)
{
    int characterCount, tries = 0;
    HANDLE object;
    WCHAR* buffer;

    characterCount = MultiByteToWideChar(CP_UTF8, 0, string, -1, NULL, 0);
    if (!characterCount)
        return;

    object = GlobalAlloc(GMEM_MOVEABLE, characterCount * sizeof(WCHAR));
    if (!object)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                             "Win32: Failed to allocate global handle for clipboard");
        return;
    }

    buffer = GlobalLock(object);
    if (!buffer)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                             "Win32: Failed to lock global handle");
        GlobalFree(object);
        return;
    }

    MultiByteToWideChar(CP_UTF8, 0, string, -1, buffer, characterCount);
    GlobalUnlock(object);

    // NOTE: Retry clipboard opening a few times as some other application may have it
    //       open and also the Windows Clipboard History reads it after each update
    while (!OpenClipboard(_glfw.win32.helperWindowHandle))
    {
        Sleep(1);
        tries++;

        if (tries == 3)
        {
            _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                                 "Win32: Failed to open clipboard");
            GlobalFree(object);
            return;
        }
    }

    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, object);
    CloseClipboard();
}

const char* _glfwGetClipboardStringWin32(void)
{
    HANDLE object;
    WCHAR* buffer;
    int tries = 0;

    // NOTE: Retry clipboard opening a few times as some other application may have it
    //       open and also the Windows Clipboard History reads it after each update
    while (!OpenClipboard(_glfw.win32.helperWindowHandle))
    {
        Sleep(1);
        tries++;

        if (tries == 3)
        {
            _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                                 "Win32: Failed to open clipboard");
            return NULL;
        }
    }

    object = GetClipboardData(CF_UNICODETEXT);
    if (!object)
    {
        _glfwInputErrorWin32(GLFW_FORMAT_UNAVAILABLE,
                             "Win32: Failed to convert clipboard to string");
        CloseClipboard();
        return NULL;
    }

    buffer = GlobalLock(object);
    if (!buffer)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                             "Win32: Failed to lock global handle");
        CloseClipboard();
        return NULL;
    }

    _glfw_free(_glfw.win32.clipboardString);
    _glfw.win32.clipboardString = _glfwCreateUTF8FromWideStringWin32(buffer);

    GlobalUnlock(object);
    CloseClipboard();

    return _glfw.win32.clipboardString;
}

EGLenum _glfwGetEGLPlatformWin32(EGLint** attribs)
{
    if (_glfw.egl.ANGLE_platform_angle)
    {
        int type = 0;

        if (_glfw.egl.ANGLE_platform_angle_opengl)
        {
            if (_glfw.hints.init.angleType == GLFW_ANGLE_PLATFORM_TYPE_OPENGL)
                type = EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE;
            else if (_glfw.hints.init.angleType == GLFW_ANGLE_PLATFORM_TYPE_OPENGLES)
                type = EGL_PLATFORM_ANGLE_TYPE_OPENGLES_ANGLE;
        }

        if (_glfw.egl.ANGLE_platform_angle_d3d)
        {
            if (_glfw.hints.init.angleType == GLFW_ANGLE_PLATFORM_TYPE_D3D9)
                type = EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE;
            else if (_glfw.hints.init.angleType == GLFW_ANGLE_PLATFORM_TYPE_D3D11)
                type = EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE;
        }

        if (_glfw.egl.ANGLE_platform_angle_vulkan)
        {
            if (_glfw.hints.init.angleType == GLFW_ANGLE_PLATFORM_TYPE_VULKAN)
                type = EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE;
        }

        if (type)
        {
            *attribs = _glfw_calloc(3, sizeof(EGLint));
            (*attribs)[0] = EGL_PLATFORM_ANGLE_TYPE_ANGLE;
            (*attribs)[1] = type;
            (*attribs)[2] = EGL_NONE;
            return EGL_PLATFORM_ANGLE_ANGLE;
        }
    }

    return 0;
}

EGLNativeDisplayType _glfwGetEGLNativeDisplayWin32(void)
{
    return GetDC(_glfw.win32.helperWindowHandle);
}

EGLNativeWindowType _glfwGetEGLNativeWindowWin32(_GLFWwindow* window)
{
    return window->win32.handle;
}

void _glfwGetRequiredInstanceExtensionsWin32(char** extensions)
{
    if (!_glfw.vk.KHR_surface || !_glfw.vk.KHR_win32_surface)
        return;

    extensions[0] = "VK_KHR_surface";
    extensions[1] = "VK_KHR_win32_surface";
}

GLFWbool _glfwGetPhysicalDevicePresentationSupportWin32(VkInstance instance,
                                                        VkPhysicalDevice device,
                                                        uint32_t queuefamily)
{
    PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR
        vkGetPhysicalDeviceWin32PresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR)
        vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR");
    if (!vkGetPhysicalDeviceWin32PresentationSupportKHR)
    {
        _glfwInputError(GLFW_API_UNAVAILABLE,
                        "Win32: Vulkan instance missing VK_KHR_win32_surface extension");
        return GLFW_FALSE;
    }

    return vkGetPhysicalDeviceWin32PresentationSupportKHR(device, queuefamily);
}

VkResult _glfwCreateWindowSurfaceWin32(VkInstance instance,
                                       _GLFWwindow* window,
                                       const VkAllocationCallbacks* allocator,
                                       VkSurfaceKHR* surface)
{
    VkResult err;
    VkWin32SurfaceCreateInfoKHR sci;
    PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;

    vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)
        vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
    if (!vkCreateWin32SurfaceKHR)
    {
        _glfwInputError(GLFW_API_UNAVAILABLE,
                        "Win32: Vulkan instance missing VK_KHR_win32_surface extension");
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    memset(&sci, 0, sizeof(sci));
    sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    sci.hinstance = _glfw.win32.instance;
    sci.hwnd = window->win32.handle;

    err = vkCreateWin32SurfaceKHR(instance, &sci, allocator, surface);
    if (err)
    {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "Win32: Failed to create Vulkan surface: %s",
                        _glfwGetVulkanResultString(err));
    }

    return err;
}

GLFWAPI HWND glfwGetWin32Window(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*) handle;
    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);

    if (_glfw.platform.platformID != GLFW_PLATFORM_WIN32)
    {
        _glfwInputError(GLFW_PLATFORM_UNAVAILABLE,
                        "Win32: Platform not initialized");
        return NULL;
    }

    return window->win32.handle;
}

GLFWAPI int glfwWin32SetWindowProp(GLFWwindow* handle, const char* name, void* value)
{
    _GLFWwindow* window = (_GLFWwindow*) handle;
    WCHAR* wideName;

    _GLFW_REQUIRE_INIT_OR_RETURN(GLFW_FALSE);
    if (!window || !name)
        return GLFW_FALSE;

    wideName = _glfwCreateWideStringFromUTF8Win32(name);
    if (!wideName)
        return GLFW_FALSE;

    const BOOL ok = SetPropW(window->win32.handle, wideName, (HANDLE) value);
    _glfw_free(wideName);
    return ok ? GLFW_TRUE : GLFW_FALSE;
}

GLFWAPI void* glfwWin32GetWindowProp(GLFWwindow* handle, const char* name)
{
    _GLFWwindow* window = (_GLFWwindow*) handle;
    WCHAR* wideName;

    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);
    if (!window || !name)
        return NULL;

    wideName = _glfwCreateWideStringFromUTF8Win32(name);
    if (!wideName)
        return NULL;

    const HANDLE value = GetPropW(window->win32.handle, wideName);
    _glfw_free(wideName);
    return (void*) value;
}

GLFWAPI void* glfwWin32RemoveWindowProp(GLFWwindow* handle, const char* name)
{
    _GLFWwindow* window = (_GLFWwindow*) handle;
    WCHAR* wideName;

    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);
    if (!window || !name)
        return NULL;

    wideName = _glfwCreateWideStringFromUTF8Win32(name);
    if (!wideName)
        return NULL;

    const HANDLE value = RemovePropW(window->win32.handle, wideName);
    _glfw_free(wideName);
    return (void*) value;
}

GLFWAPI void* glfwWin32AddMessageHook(GLFWwindow* handle,
                                     int (*hook)(GLFWwindow* window,
                                                 HWND hWnd, unsigned int uMsg, uintptr_t wParam, intptr_t lParam,
                                                 intptr_t* result, void* user),
                                     void* user)
{
    _GLFWwindow* window = (_GLFWwindow*) handle;
    _GLFWwin32MessageHook* node;
    _GLFWwin32MessageHook** pp;

    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);
    if (!window || !hook)
        return NULL;

    node = _glfw_calloc(1, sizeof(*node));
    if (!node)
    {
        _glfwInputError(GLFW_OUT_OF_MEMORY, "Win32: Failed to allocate message hook");
        return NULL;
    }

    node->fn = hook;
    node->user = user;
    node->next = NULL;

    if (_glfw.win32.hookLock)
        _glfwPlatformLockMutex(_glfw.win32.hookLock);

    // Append (preserve registration order)
    pp = &window->win32.messageHooks;
    while (*pp)
        pp = &(*pp)->next;
    *pp = node;

    if (_glfw.win32.hookLock)
        _glfwPlatformUnlockMutex(_glfw.win32.hookLock);

    return (void*) node;
}

GLFWAPI int glfwWin32RemoveMessageHook(GLFWwindow* handle, void* token)
{
    _GLFWwindow* window = (_GLFWwindow*) handle;
    _GLFWwin32MessageHook** pp;
    _GLFWwin32MessageHook* node;

    _GLFW_REQUIRE_INIT_OR_RETURN(GLFW_FALSE);
    if (!window || !token)
        return GLFW_FALSE;

    if (_glfw.win32.hookLock)
        _glfwPlatformLockMutex(_glfw.win32.hookLock);

    pp = &window->win32.messageHooks;
    while (*pp && *pp != (_GLFWwin32MessageHook*) token)
        pp = &(*pp)->next;

    if (!*pp)
    {
        if (_glfw.win32.hookLock)
            _glfwPlatformUnlockMutex(_glfw.win32.hookLock);
        return GLFW_FALSE;
    }

    node = *pp;
    *pp = node->next;
    node->next = window->win32.pendingHookFrees;
    window->win32.pendingHookFrees = node;

    // If we are not dispatching hooks, free immediately.
    if (window->win32.hookDispatchDepth <= 0)
    {
        _GLFWwin32MessageHook* it = window->win32.pendingHookFrees;
        window->win32.pendingHookFrees = NULL;
        while (it)
        {
            _GLFWwin32MessageHook* next = it->next;
            _glfw_free(it);
            it = next;
        }
    }

    if (_glfw.win32.hookLock)
        _glfwPlatformUnlockMutex(_glfw.win32.hookLock);

    return GLFW_TRUE;
}

#endif // _GLFW_WIN32

