#ifndef RL_WIN32_PATH_H
#define RL_WIN32_PATH_H

#include <stdio.h>

#if defined(_WIN32)
#include <wchar.h>

typedef struct RLWin32PathError
{
    const char *stage;
    unsigned long win32Error;
    int crtError;
    int requiredChars;
    const char *inputUtf8;
} RLWin32PathError;

void RLWin32PathErrorReset(RLWin32PathError *err, const char *inputUtf8);
void RLWin32PathErrorSet(RLWin32PathError *err, const char *stage, unsigned long win32Error, int crtError, int requiredChars);

FILE *RLWin32PathOpenFileForModeUtf8(const char *fileName, const char *mode, RLWin32PathError *err);

int RLWin32PathGetPathAttributesUtf8(const char *pathUtf8, unsigned long *outAttrs);
int RLWin32PathSetCurrentDirectoryUtf8(const char *pathUtf8);
int RLWin32PathCreateDirectoryUtf8(const char *pathUtf8);
int RLWin32PathMakeDirectoryTreeUtf8(const char *pathUtf8);
int RLWin32PathGetCurrentDirectoryUtf8(char *outUtf8, int outUtf8Capacity, int *requiredCharsOut);
int RLWin32PathGetFileModTimeUtf8(const char *pathUtf8, long *outUnixSeconds);

#endif // _WIN32

#endif // RL_WIN32_PATH_H
