#include "win32_path.h"

#if defined(_WIN32)

#include "../raylib.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef MAX_FILEPATH_LENGTH
#define MAX_FILEPATH_LENGTH 4096
#endif

#ifndef CP_UTF8
#define CP_UTF8 65001
#endif

#ifndef MB_ERR_INVALID_CHARS
#define MB_ERR_INVALID_CHARS 0x00000008
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((unsigned long)0xFFFFFFFF)
#endif

#ifndef FILE_ATTRIBUTE_DIRECTORY
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#endif
#ifndef ERROR_ALREADY_EXISTS
#define ERROR_ALREADY_EXISTS 183u
#endif

__declspec(dllimport) unsigned long __stdcall GetFullPathNameW(const wchar_t *lpFileName, unsigned long nBufferLength, wchar_t *lpBuffer, wchar_t **lpFilePart);
__declspec(dllimport) unsigned long __stdcall GetCurrentDirectoryW(unsigned long nBufferLength, wchar_t *lpBuffer);
__declspec(dllimport) int __stdcall SetCurrentDirectoryW(const wchar_t *lpPathName);
__declspec(dllimport) int __stdcall CreateDirectoryW(const wchar_t *lpPathName, void *lpSecurityAttributes);
__declspec(dllimport) unsigned long __stdcall GetFileAttributesW(const wchar_t *lpFileName);
__declspec(dllimport) int __stdcall GetFileAttributesExW(const wchar_t *lpFileName, int fInfoLevelId, void *lpFileInformation);
__declspec(dllimport) unsigned long __stdcall GetLastError(void);
__declspec(dllimport) int __stdcall WideCharToMultiByte(unsigned int cp, unsigned long flags, const wchar_t *widestr, int cchwide, char *str, int cbmb, const char *defchar, int *used_default);
__declspec(dllimport) int __stdcall MultiByteToWideChar(unsigned int CodePage, unsigned long dwFlags, const char *lpMultiByteStr, int cbMultiByte, wchar_t *lpWideCharStr, int cchWideChar);

typedef struct RLWin32FileTime
{
    unsigned long dwLowDateTime;
    unsigned long dwHighDateTime;
} RLWin32FileTime;

typedef struct RLWin32FileAttributeData
{
    unsigned long dwFileAttributes;
    RLWin32FileTime ftCreationTime;
    RLWin32FileTime ftLastAccessTime;
    RLWin32FileTime ftLastWriteTime;
    unsigned long nFileSizeHigh;
    unsigned long nFileSizeLow;
} RLWin32FileAttributeData;

#ifndef RL_WIN32_GETFILEEXINFO_STANDARD
#define RL_WIN32_GETFILEEXINFO_STANDARD 0
#endif

void RLWin32PathErrorReset(RLWin32PathError *err, const char *inputUtf8)
{
    if (err == NULL) return;
    err->stage = NULL;
    err->win32Error = 0;
    err->crtError = 0;
    err->requiredChars = 0;
    err->inputUtf8 = inputUtf8;
}

void RLWin32PathErrorSet(RLWin32PathError *err, const char *stage, unsigned long win32Error, int crtError, int requiredChars)
{
    if (err == NULL) return;
    err->stage = stage;
    err->win32Error = win32Error;
    err->crtError = crtError;
    err->requiredChars = requiredChars;
}

static wchar_t *RLWin32WideDup(const wchar_t *src)
{
    if (src == NULL) return NULL;
    size_t len = wcslen(src);
    wchar_t *dst = (wchar_t *)RL_CALLOC((unsigned int)(len + 1u), sizeof(wchar_t));
    if (dst == NULL) return NULL;
    memcpy(dst, src, (len + 1u)*sizeof(wchar_t));
    return dst;
}

static wchar_t *RLWin32Utf8ToWideAlloc(const char *utf8, unsigned long flags)
{
    if (utf8 == NULL) return NULL;
    int requiredChars = MultiByteToWideChar(CP_UTF8, flags, utf8, -1, NULL, 0);
    if (requiredChars <= 0) return NULL;

    wchar_t *wide = (wchar_t *)RL_CALLOC((unsigned int)requiredChars, sizeof(wchar_t));
    if (wide == NULL) return NULL;

    int convertedChars = MultiByteToWideChar(CP_UTF8, flags, utf8, -1, wide, requiredChars);
    if (convertedChars <= 0)
    {
        RL_FREE(wide);
        return NULL;
    }

    return wide;
}

static wchar_t *RLWin32GetFullPathAlloc(const wchar_t *path)
{
    if (path == NULL) return NULL;

    unsigned long cap = MAX_FILEPATH_LENGTH;
    while (cap <= 65536u)
    {
        wchar_t *buffer = (wchar_t *)RL_CALLOC((unsigned int)cap, sizeof(wchar_t));
        if (buffer == NULL) return NULL;

        unsigned long written = GetFullPathNameW(path, cap, buffer, NULL);
        if ((written > 0u) && (written < cap))
        {
            return buffer;
        }

        RL_FREE(buffer);
        if (written == 0u) return NULL;
        cap = written + 1u;
    }

    return NULL;
}

static int RLWin32PathHasExtendedPrefix(const wchar_t *path);

static int RLWin32PathIsAbsolute(const wchar_t *path)
{
    if (path == NULL) return 0;
    if (RLWin32PathHasExtendedPrefix(path)) return 1;

    // Drive absolute path: X:\... or X:/...
    if (((path[0] >= L'A' && path[0] <= L'Z') || (path[0] >= L'a' && path[0] <= L'z')) &&
        (path[1] == L':') &&
        ((path[2] == L'\\') || (path[2] == L'/'))) return 1;

    // UNC path: \\server\share\...
    if ((path[0] == L'\\') && (path[1] == L'\\')) return 1;

    return 0;
}

static void RLWin32NormalizeSlashesInPlace(wchar_t *path)
{
    if (path == NULL) return;
    if (RLWin32PathHasExtendedPrefix(path)) return;

    for (wchar_t *cursor = path; *cursor != 0; cursor++)
    {
        if (*cursor == L'/') *cursor = L'\\';
    }
}

static wchar_t *RLWin32NormalizeAbsolutePathAlloc(const wchar_t *path, int *requiredCharsOut, unsigned long *win32ErrorOut)
{
    if (requiredCharsOut != NULL) *requiredCharsOut = 0;
    if (win32ErrorOut != NULL) *win32ErrorOut = 0;
    if (path == NULL) return NULL;

    if (RLWin32PathIsAbsolute(path))
    {
        wchar_t *absolute = RLWin32WideDup(path);
        RLWin32NormalizeSlashesInPlace(absolute);
        return absolute;
    }

    unsigned long required = GetFullPathNameW(path, 0, NULL, NULL);
    wchar_t *absolute = RLWin32GetFullPathAlloc(path);
    if (absolute == NULL)
    {
        if (requiredCharsOut != NULL) *requiredCharsOut = (int)required;
        if (win32ErrorOut != NULL) *win32ErrorOut = GetLastError();
        return NULL;
    }

    RLWin32NormalizeSlashesInPlace(absolute);
    return absolute;
}

static int RLWin32PathHasExtendedPrefix(const wchar_t *path)
{
    return (path != NULL) && (path[0] == L'\\') && (path[1] == L'\\') && (path[2] == L'?') && (path[3] == L'\\');
}

static int RLWin32PathIsUnc(const wchar_t *path)
{
    return (path != NULL) && (path[0] == L'\\') && (path[1] == L'\\');
}

static wchar_t *RLWin32ToExtendedPathAlloc(const wchar_t *absolutePath)
{
    if (absolutePath == NULL) return NULL;
    if (RLWin32PathHasExtendedPrefix(absolutePath)) return RLWin32WideDup(absolutePath);

    static const wchar_t *prefixLocal = L"\\\\?\\";
    static const wchar_t *prefixUnc = L"\\\\?\\UNC\\";

    size_t pathLen = wcslen(absolutePath);
    size_t prefixLen = RLWin32PathIsUnc(absolutePath) ? wcslen(prefixUnc) : wcslen(prefixLocal);
    size_t skip = RLWin32PathIsUnc(absolutePath) ? 2u : 0u;
    size_t outLen = prefixLen + (pathLen - skip);

    wchar_t *out = (wchar_t *)RL_CALLOC((unsigned int)(outLen + 1u), sizeof(wchar_t));
    if (out == NULL) return NULL;

    memcpy(out, RLWin32PathIsUnc(absolutePath) ? prefixUnc : prefixLocal, prefixLen*sizeof(wchar_t));
    memcpy(out + prefixLen, absolutePath + skip, (pathLen - skip + 1u)*sizeof(wchar_t));
    return out;
}

static int RLWin32ConvertModeToWide(const char *mode, wchar_t *modeWide, int modeWideCapacity, RLWin32PathError *err)
{
    if ((mode == NULL) || (modeWide == NULL) || (modeWideCapacity <= 0))
    {
        RLWin32PathErrorSet(err, "mode-invalid-args", 0, 0, 0);
        return 0;
    }

    int requiredChars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, mode, -1, NULL, 0);
    if (requiredChars <= 0)
    {
        RLWin32PathErrorSet(err, "mode-utf8_to_wide", GetLastError(), 0, 0);
        return 0;
    }

    if (requiredChars > modeWideCapacity)
    {
        RLWin32PathErrorSet(err, "mode-too-long", 0, 0, requiredChars);
        return 0;
    }

    int convertedChars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, mode, -1, modeWide, modeWideCapacity);
    if (convertedChars <= 0)
    {
        RLWin32PathErrorSet(err, "mode-utf8_to_wide", GetLastError(), 0, requiredChars);
        return 0;
    }

    return 1;
}

static FILE *RLWin32OpenFileUtf8Ex(const char *fileName, const wchar_t *modeW, RLWin32PathError *err)
{
    RLWin32PathErrorReset(err, fileName);
    if ((fileName == NULL) || (modeW == NULL))
    {
        RLWin32PathErrorSet(err, "invalid-args", 0, 0, 0);
        return NULL;
    }

    int requiredWideChars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, fileName, -1, NULL, 0);
    if (requiredWideChars <= 0)
    {
        RLWin32PathErrorSet(err, "utf8_to_wide", GetLastError(), 0, 0);
        return NULL;
    }

    wchar_t *pathWide = RLWin32Utf8ToWideAlloc(fileName, MB_ERR_INVALID_CHARS);
    if (pathWide == NULL)
    {
        RLWin32PathErrorSet(err, "utf8_to_wide", GetLastError(), 0, requiredWideChars);
        return NULL;
    }

    int requiredAbs = 0;
    unsigned long normalizeError = 0;
    wchar_t *absoluteWide = RLWin32NormalizeAbsolutePathAlloc(pathWide, &requiredAbs, &normalizeError);
    RL_FREE(pathWide);
    if (absoluteWide == NULL)
    {
        RLWin32PathErrorSet(err, "fullpath", normalizeError, 0, requiredAbs);
        return NULL;
    }

    wchar_t *extendedWide = RLWin32ToExtendedPathAlloc(absoluteWide);
    if (extendedWide == NULL)
    {
        RL_FREE(absoluteWide);
        RLWin32PathErrorSet(err, "prefix", 0, 0, 0);
        return NULL;
    }

    FILE *file = NULL;
#if defined(_MSC_VER)
    errno_t openErr = _wfopen_s(&file, extendedWide, modeW);
    if ((openErr != 0) || (file == NULL))
    {
        openErr = _wfopen_s(&file, absoluteWide, modeW);
        if ((openErr != 0) || (file == NULL)) RLWin32PathErrorSet(err, "open", GetLastError(), (int)openErr, 0);
    }
#else
    file = _wfopen(extendedWide, modeW);
    if (file == NULL)
    {
        file = _wfopen(absoluteWide, modeW);
        if (file == NULL) RLWin32PathErrorSet(err, "open", GetLastError(), errno, 0);
    }
#endif

    RL_FREE(extendedWide);
    RL_FREE(absoluteWide);
    return file;
}

FILE *RLWin32PathOpenFileForModeUtf8(const char *fileName, const char *mode, RLWin32PathError *err)
{
    wchar_t modeWide[64] = { 0 };
    RLWin32PathErrorReset(err, fileName);

    if (!RLWin32ConvertModeToWide(mode, modeWide, (int)(sizeof(modeWide)/sizeof(modeWide[0])), err))
    {
        return NULL;
    }

    return RLWin32OpenFileUtf8Ex(fileName, modeWide, err);
}

int RLWin32PathGetPathAttributesUtf8(const char *pathUtf8, unsigned long *outAttrs)
{
    if ((pathUtf8 == NULL) || (outAttrs == NULL)) return 0;

    wchar_t *pathWide = RLWin32Utf8ToWideAlloc(pathUtf8, MB_ERR_INVALID_CHARS);
    if (pathWide == NULL) return 0;

    wchar_t *absoluteWide = RLWin32NormalizeAbsolutePathAlloc(pathWide, NULL, NULL);
    RL_FREE(pathWide);
    if (absoluteWide == NULL) return 0;

    wchar_t *extendedWide = RLWin32ToExtendedPathAlloc(absoluteWide);
    if (extendedWide == NULL)
    {
        RL_FREE(absoluteWide);
        return 0;
    }

    unsigned long attrs = GetFileAttributesW(extendedWide);
    if (attrs == INVALID_FILE_ATTRIBUTES) attrs = GetFileAttributesW(absoluteWide);

    RL_FREE(extendedWide);
    RL_FREE(absoluteWide);

    if (attrs == INVALID_FILE_ATTRIBUTES) return 0;
    *outAttrs = attrs;
    return 1;
}

int RLWin32PathSetCurrentDirectoryUtf8(const char *pathUtf8)
{
    if (pathUtf8 == NULL) return 0;
    wchar_t *pathWide = RLWin32Utf8ToWideAlloc(pathUtf8, MB_ERR_INVALID_CHARS);
    if (pathWide == NULL) return 0;

    wchar_t *absoluteWide = RLWin32NormalizeAbsolutePathAlloc(pathWide, NULL, NULL);
    RL_FREE(pathWide);
    if (absoluteWide == NULL) return 0;

    wchar_t *extendedWide = RLWin32ToExtendedPathAlloc(absoluteWide);
    if (extendedWide == NULL)
    {
        RL_FREE(absoluteWide);
        return 0;
    }

    int ok = SetCurrentDirectoryW(extendedWide);
    if (!ok) ok = SetCurrentDirectoryW(absoluteWide);

    RL_FREE(extendedWide);
    RL_FREE(absoluteWide);
    return ok ? 1 : 0;
}

int RLWin32PathCreateDirectoryUtf8(const char *pathUtf8)
{
    if (pathUtf8 == NULL) return 0;
    wchar_t *pathWide = RLWin32Utf8ToWideAlloc(pathUtf8, MB_ERR_INVALID_CHARS);
    if (pathWide == NULL)
    {
        RLTraceLog(RL_E_LOG_WARNING, "FILEIO: CreateDirectory utf8->wide failed path=%s err=%lu", (pathUtf8 != NULL)? pathUtf8 : "(null)", GetLastError());
        return 0;
    }

    wchar_t *absoluteWide = RLWin32NormalizeAbsolutePathAlloc(pathWide, NULL, NULL);
    RL_FREE(pathWide);
    if (absoluteWide == NULL)
    {
        RLTraceLog(RL_E_LOG_WARNING, "FILEIO: CreateDirectory normalize failed path=%s", (pathUtf8 != NULL)? pathUtf8 : "(null)");
        return 0;
    }

    wchar_t *extendedWide = RLWin32ToExtendedPathAlloc(absoluteWide);
    if (extendedWide == NULL)
    {
        RL_FREE(absoluteWide);
        RLTraceLog(RL_E_LOG_WARNING, "FILEIO: CreateDirectory to-extended failed path=%s", (pathUtf8 != NULL)? pathUtf8 : "(null)");
        return 0;
    }

    int ok = CreateDirectoryW(extendedWide, NULL);
    if (!ok)
    {
        unsigned long createErr = GetLastError();
        if (createErr == ERROR_ALREADY_EXISTS) ok = 1;
    }

    if (!ok)
    {
        ok = CreateDirectoryW(absoluteWide, NULL);
        if (!ok)
        {
            unsigned long createErr = GetLastError();
            if (createErr == ERROR_ALREADY_EXISTS) ok = 1;
        }
    }

    RL_FREE(extendedWide);
    RL_FREE(absoluteWide);
    if (!ok)
    {
        RLTraceLog(RL_E_LOG_WARNING, "FILEIO: CreateDirectory failed path=%s err=%lu", (pathUtf8 != NULL)? pathUtf8 : "(null)", GetLastError());
    }

    return ok ? 1 : 0;
}

int RLWin32PathMakeDirectoryTreeUtf8(const char *pathUtf8)
{
    if ((pathUtf8 == NULL) || (pathUtf8[0] == '\0')) return 0;

    wchar_t *pathWide = RLWin32Utf8ToWideAlloc(pathUtf8, MB_ERR_INVALID_CHARS);
    if (pathWide == NULL)
    {
        RLTraceLog(RL_E_LOG_WARNING, "FILEIO: MakeDirTree utf8->wide failed path=%s err=%lu", (pathUtf8 != NULL)? pathUtf8 : "(null)", GetLastError());
        return 0;
    }

    wchar_t *absoluteWide = RLWin32NormalizeAbsolutePathAlloc(pathWide, NULL, NULL);
    RL_FREE(pathWide);
    if (absoluteWide == NULL)
    {
        RLTraceLog(RL_E_LOG_WARNING, "FILEIO: MakeDirTree normalize failed path=%s err=%lu", (pathUtf8 != NULL)? pathUtf8 : "(null)", GetLastError());
        return 0;
    }

    wchar_t *extendedWide = RLWin32ToExtendedPathAlloc(absoluteWide);
    RL_FREE(absoluteWide);
    if (extendedWide == NULL)
    {
        RLTraceLog(RL_E_LOG_WARNING, "FILEIO: MakeDirTree to-extended failed path=%s", (pathUtf8 != NULL)? pathUtf8 : "(null)");
        return 0;
    }

    size_t startIndex = 0;
    if (wcsncmp(extendedWide, L"\\\\?\\UNC\\", 8) == 0)
    {
        // Skip UNC prefix and the "server/share" components.
        startIndex = 8;
        int separators = 0;
        while (extendedWide[startIndex] != 0)
        {
            if (extendedWide[startIndex] == L'\\')
            {
                separators++;
                if (separators == 2)
                {
                    startIndex++;
                    break;
                }
            }
            startIndex++;
        }
    }
    else if (wcsncmp(extendedWide, L"\\\\?\\", 4) == 0)
    {
        // Skip local extended-path root (drive prefix).
        startIndex = 7;
    }

    for (size_t i = startIndex; extendedWide[i] != 0; i++)
    {
        if (extendedWide[i] != L'\\') continue;

        wchar_t saved = extendedWide[i];
        extendedWide[i] = 0;

        if (!CreateDirectoryW(extendedWide, NULL))
        {
            unsigned long createErr = GetLastError();
            if (createErr != ERROR_ALREADY_EXISTS)
            {
                RLTraceLog(RL_E_LOG_WARNING, "FILEIO: MakeDirTree step-create failed path=%s err=%lu", (pathUtf8 != NULL)? pathUtf8 : "(null)", createErr);
                extendedWide[i] = saved;
                RL_FREE(extendedWide);
                return 0;
            }
        }

        extendedWide[i] = saved;
    }

    if (!CreateDirectoryW(extendedWide, NULL))
    {
        unsigned long createErr = GetLastError();
        if (createErr != ERROR_ALREADY_EXISTS)
        {
            RLTraceLog(RL_E_LOG_WARNING, "FILEIO: MakeDirTree final-create failed path=%s err=%lu", (pathUtf8 != NULL)? pathUtf8 : "(null)", createErr);
            RL_FREE(extendedWide);
            return 0;
        }
    }

    RL_FREE(extendedWide);
    return 1;
}

int RLWin32PathGetCurrentDirectoryUtf8(char *outUtf8, int outUtf8Capacity, int *requiredCharsOut)
{
    if ((outUtf8 == NULL) || (outUtf8Capacity <= 0)) return 0;
    outUtf8[0] = '\0';
    if (requiredCharsOut != NULL) *requiredCharsOut = 0;

    unsigned long cap = MAX_FILEPATH_LENGTH;
    while (cap <= 65536u)
    {
        wchar_t *wide = (wchar_t *)RL_CALLOC((unsigned int)cap, sizeof(wchar_t));
        if (wide == NULL) return 0;

        unsigned long len = GetCurrentDirectoryW(cap, wide);
        if ((len > 0u) && (len < cap))
        {
            int required = WideCharToMultiByte(CP_UTF8, 0, wide, (int)len, NULL, 0, NULL, NULL);
            if (requiredCharsOut != NULL) *requiredCharsOut = required;

            if (required > 0)
            {
                int writable = required;
                if (writable >= outUtf8Capacity) writable = outUtf8Capacity - 1;
                int written = WideCharToMultiByte(CP_UTF8, 0, wide, (int)len, outUtf8, writable, NULL, NULL);
                if (written < 0) written = 0;
                outUtf8[written] = '\0';
            }

            RL_FREE(wide);
            return 1;
        }

        RL_FREE(wide);
        if (len == 0u) return 0;
        cap = len + 1u;
    }

    return 0;
}

int RLWin32PathGetFileModTimeUtf8(const char *pathUtf8, long *outUnixSeconds)
{
    if ((pathUtf8 == NULL) || (outUnixSeconds == NULL)) return 0;

    wchar_t *pathWide = RLWin32Utf8ToWideAlloc(pathUtf8, MB_ERR_INVALID_CHARS);
    if (pathWide == NULL) return 0;

    wchar_t *absoluteWide = RLWin32NormalizeAbsolutePathAlloc(pathWide, NULL, NULL);
    RL_FREE(pathWide);
    if (absoluteWide == NULL) return 0;

    wchar_t *extendedWide = RLWin32ToExtendedPathAlloc(absoluteWide);
    if (extendedWide == NULL)
    {
        RL_FREE(absoluteWide);
        return 0;
    }

    RLWin32FileAttributeData data = { 0 };
    int ok = GetFileAttributesExW(extendedWide, RL_WIN32_GETFILEEXINFO_STANDARD, &data);
    if (!ok) ok = GetFileAttributesExW(absoluteWide, RL_WIN32_GETFILEEXINFO_STANDARD, &data);

    RL_FREE(extendedWide);
    RL_FREE(absoluteWide);

    if (!ok) return 0;

    {
        const unsigned long long winTicks =
            (((unsigned long long)data.ftLastWriteTime.dwHighDateTime) << 32) |
            ((unsigned long long)data.ftLastWriteTime.dwLowDateTime);
        const unsigned long long unixEpochTicks = 116444736000000000ULL; // 1601-01-01 to 1970-01-01 (100ns units)
        if (winTicks <= unixEpochTicks)
        {
            *outUnixSeconds = 0;
        }
        else
        {
            const unsigned long long unixSeconds = (winTicks - unixEpochTicks)/10000000ULL;
            if (unixSeconds > 0x7FFFFFFFULL) *outUnixSeconds = 2147483647L;
            else *outUnixSeconds = (long)unixSeconds;
        }
    }

    return 1;
}

#endif // _WIN32
