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

typedef int (*RLWin32PathDirectoryVisitor)(const char *entryPathUtf8, int isDirectory, void *userData);

void RLWin32PathErrorReset(RLWin32PathError *err, const char *inputUtf8);
void RLWin32PathErrorSet(RLWin32PathError *err, const char *stage, unsigned long win32Error, int crtError, int requiredChars);

FILE *RLWin32PathOpenFileForModeUtf8(const char *fileName, const char *mode, RLWin32PathError *err);

int RLWin32PathGetPathAttributesUtf8(const char *pathUtf8, unsigned long *outAttrs);
int RLWin32PathSetCurrentDirectoryUtf8(const char *pathUtf8);
int RLWin32PathCreateDirectoryUtf8(const char *pathUtf8);
int RLWin32PathMakeDirectoryTreeUtf8(const char *pathUtf8);
int RLWin32PathGetCurrentDirectoryUtf8(char *outUtf8, int outUtf8Capacity, int *requiredCharsOut);
int RLWin32PathGetFileModTimeUtf8(const char *pathUtf8, long *outUnixSeconds);
int RLWin32PathIsFileUtf8(const char *pathUtf8, int *outIsFile);
int RLWin32PathDeleteFileUtf8(const char *pathUtf8);
int RLWin32PathCopyFileUtf8(const char *srcPathUtf8, const char *dstPathUtf8, int overwriteExisting);
int RLWin32PathMoveFileUtf8(const char *srcPathUtf8, const char *dstPathUtf8, int replaceExisting, int allowCopy);
int RLWin32PathEnumerateDirectoryUtf8(const char *basePathUtf8, int scanSubdirs, RLWin32PathDirectoryVisitor visitor, void *userData);

#endif // _WIN32

#endif // RL_WIN32_PATH_H
