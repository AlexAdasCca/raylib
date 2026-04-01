/*******************************************************************************************
*
*   raylib [core] example - long path and UTF-8 file I/O validation
*
*   This console example validates Win32 UTF-8 and long-path handling used by raylib file APIs.
*   Optional arguments:
*     1) custom root path for the long-path probe
*     2) custom UNC root path for an additional UNC validation pass
*
********************************************************************************************/

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
    #include <windows.h>
    #ifndef CP_UTF8
        #define CP_UTF8 65001
    #endif
#endif

typedef struct LongPathUtf8ProbeResult {
    int ok;
    int uncEnabled;
    int uncOk;
    int rootLength;
    int textPathLength;
    int movedPathLength;
    int saveTextOk;
    int loadTextOk;
    int saveDataOk;
    int loadDataOk;
    int copyOk;
    int renameOk;
    int moveOk;
    int fileExistsOk;
    int dirExistsOk;
    int changeDirOk;
    int listCountOk;
    int fileNameOk;
    int fileStemOk;
    int dirPathOk;
    int fileLengthOk;
    int modTimeOk;
    int validFileNameOk;
    char failStage[64];
    char rootPath[4096];
    char textPath[4096];
} LongPathUtf8ProbeResult;

static const int kUtf8ProbePayload[] = {
    0x0055, 0x0054, 0x0046, 0x002d, 0x0038, 0x0020, 0x0070, 0x0072, 0x006f, 0x0062, 0x0065, 0x003a, 0x0020,
    0x4f60, 0x597d, 0x0020, 0x002f, 0x0020, 0x8def, 0x5f84, 0x68c0, 0x67e5, 0x0020, 0x002f, 0x0020,
    0x0070, 0x0061, 0x0074, 0x0068, 0x002d, 0x0063, 0x0068, 0x0065, 0x0063, 0x006b
};
static const int kSegmentWord[] = { 0x6bb5, 0x843d };
static const int kFsWord[] = { 0x6587, 0x4ef6, 0x7cfb, 0x7edf };
static const int kValidateWord[] = { 0x8def, 0x5f84, 0x9a8c, 0x8bc1 };
static const int kLongPathWord[] = { 0x957f, 0x8def, 0x5f84 };
static const int kTextWord[] = { 0x6d4b, 0x8bd5, 0x6587, 0x672c };
static const int kHelloWorldWord[] = { 0x4f60, 0x597d, 0x4e16, 0x754c };
static const int kMovedWord[] = { 0x5df2, 0x79fb, 0x52a8 };
static const int kPathWord[] = { 0x8def, 0x5f84 };
static const int kFileWord[] = { 0x6587, 0x4ef6 };
static const int kUncWord[] = { 0x8def, 0x5f84 };

static int ProbeFail(LongPathUtf8ProbeResult *result, const char *stageName)
{
    result->ok = 0;
    RLTextFormatTo(result->failStage, (int)sizeof(result->failStage), "%s", (stageName != NULL) ? stageName : "(unknown)");
    return 0;
}

static int ProbePass(LongPathUtf8ProbeResult *result)
{
    result->ok = 1;
    RLTextFormatTo(result->failStage, (int)sizeof(result->failStage), "ok");
    return 1;
}

static void BuildUtf8String(char *outText, int outTextSize, const int *codepoints, int codepointCount)
{
    if ((outText == NULL) || (outTextSize <= 0))
    {
        return;
    }

    outText[0] = '\0';
    if ((codepoints == NULL) || (codepointCount <= 0))
    {
        return;
    }

    char *utf8Text = RLLoadUTF8(codepoints, codepointCount);
    if (utf8Text != NULL)
    {
        RLTextFormatTo(outText, outTextSize, "%s", utf8Text);
        RLUnloadUTF8(utf8Text);
    }
}

static int BuildProbePaths(LongPathUtf8ProbeResult *result, const char *rootOverride)
{
#if defined(_WIN32)
    const char *sep = "\\";
#else
    const char *sep = "/";
#endif
    int required = 0;
    char utf8WordA[64] = { 0 };
    char utf8WordB[64] = { 0 };
    char utf8WordC[64] = { 0 };
    char utf8WordD[64] = { 0 };
    char utf8WordE[64] = { 0 };
    char utf8WordF[64] = { 0 };
    char utf8WordG[64] = { 0 };

    BuildUtf8String(utf8WordA, (int)sizeof(utf8WordA), kValidateWord, (int)(sizeof(kValidateWord)/sizeof(kValidateWord[0])));
    BuildUtf8String(utf8WordB, (int)sizeof(utf8WordB), kSegmentWord, (int)(sizeof(kSegmentWord)/sizeof(kSegmentWord[0])));
    BuildUtf8String(utf8WordC, (int)sizeof(utf8WordC), kFsWord, (int)(sizeof(kFsWord)/sizeof(kFsWord[0])));
    BuildUtf8String(utf8WordD, (int)sizeof(utf8WordD), kLongPathWord, (int)(sizeof(kLongPathWord)/sizeof(kLongPathWord[0])));
    BuildUtf8String(utf8WordE, (int)sizeof(utf8WordE), kTextWord, (int)(sizeof(kTextWord)/sizeof(kTextWord[0])));
    BuildUtf8String(utf8WordF, (int)sizeof(utf8WordF), kHelloWorldWord, (int)(sizeof(kHelloWorldWord)/sizeof(kHelloWorldWord[0])));
    BuildUtf8String(utf8WordG, (int)sizeof(utf8WordG), kMovedWord, (int)(sizeof(kMovedWord)/sizeof(kMovedWord[0])));

    if ((rootOverride != NULL) && (rootOverride[0] != '\0'))
    {
        required = RLTextFormatTo(result->rootPath, (int)sizeof(result->rootPath), "%s", rootOverride);
    }
    else
    {
        required = RLTextFormatTo(result->rootPath,
                                  (int)sizeof(result->rootPath),
                                  "%s%stemp%slongpath_utf8_probe_%s",
                                  RLGetWorkingDirectory(), sep, sep, utf8WordA);
    }

    if (required >= (int)sizeof(result->rootPath)) return ProbeFail(result, "format-root");

    char deepDir[4096] = { 0 };
    RLTextFormatTo(deepDir, (int)sizeof(deepDir), "%s", result->rootPath);

    for (int segmentIndex = 0; segmentIndex < 14; segmentIndex++)
    {
        char nextPath[4096] = { 0 };
        required = RLTextFormatTo(nextPath,
                                  (int)sizeof(nextPath),
                                  "%s%s%s_%02d_%s_%s_utf8_abcdefghijklmnopqrstuvwxyz0123456789",
                                  deepDir, sep, utf8WordB, segmentIndex, utf8WordC, utf8WordD);
        if (required >= (int)sizeof(nextPath)) return ProbeFail(result, "format-deep");
        RLTextFormatTo(deepDir, (int)sizeof(deepDir), "%s", nextPath);
    }

    required = RLTextFormatTo(result->textPath,
                              (int)sizeof(result->textPath),
                              "%s%s%s_%s_utf8_%s.txt",
                              deepDir, sep, utf8WordE, utf8WordD, utf8WordF);
    if (required >= (int)sizeof(result->textPath)) return ProbeFail(result, "format-text");

    result->rootLength = RLTextLength(result->rootPath);
    result->textPathLength = RLTextLength(result->textPath);
    result->movedPathLength = 0;

    return 1;
}

static int EnsureDeepDirectoryTree(LongPathUtf8ProbeResult *result)
{
    char directoryPath[4096] = { 0 };
    const char *lastSlash = strrchr(result->textPath, '/');
    const char *lastBackslash = strrchr(result->textPath, '\\');
    const char *lastSeparator = (lastBackslash != NULL) ? lastBackslash : lastSlash;
    if ((lastSlash != NULL) && (lastBackslash != NULL)) lastSeparator = (lastSlash > lastBackslash) ? lastSlash : lastBackslash;
    if (lastSeparator == NULL) return ProbeFail(result, "no-separator");

    RLTextFormatTo(directoryPath, (int)sizeof(directoryPath), "%s", result->textPath);
    directoryPath[lastSeparator - result->textPath] = '\0';

    if (RLMakeDirectory(result->rootPath) != 0) return ProbeFail(result, "make-root");
    if (RLMakeDirectory(directoryPath) != 0) return ProbeFail(result, "make-deep");
    return 1;
}

static int RunLongPathUtf8Probe(LongPathUtf8ProbeResult *result, const char *rootOverride, const char *uncRootOverride)
{
    static const unsigned char payloadData[] = { 0x42, 0x13, 0x24, 0x35, 0xaa, 0xbb, 0xcc, 0xdd };
#if defined(_WIN32)
    const char *sep = "\\";
#else
    const char *sep = "/";
#endif
    char payloadText[256] = { 0 };
    char dataPath[4096] = { 0 };
    char copyPath[4096] = { 0 };
    char renamedPath[4096] = { 0 };
    char movedDir[4096] = { 0 };
    char movedPath[4096] = { 0 };
    char renameLeaf[256] = { 0 };
    char uncDir[4096] = { 0 };
    char uncFile[4096] = { 0 };
    char utf8FileWord[64] = { 0 };
    char utf8PathWord[64] = { 0 };
    char utf8MovedWord[64] = { 0 };
    char utf8LongPathWord[64] = { 0 };
    int readSize = 0;

    memset(result, 0, sizeof(*result));
    result->uncOk = 1;

    BuildUtf8String(payloadText, (int)sizeof(payloadText), kUtf8ProbePayload, (int)(sizeof(kUtf8ProbePayload)/sizeof(kUtf8ProbePayload[0])));
    BuildUtf8String(utf8FileWord, (int)sizeof(utf8FileWord), kFileWord, (int)(sizeof(kFileWord)/sizeof(kFileWord[0])));
    BuildUtf8String(utf8PathWord, (int)sizeof(utf8PathWord), kPathWord, (int)(sizeof(kPathWord)/sizeof(kPathWord[0])));
    BuildUtf8String(utf8MovedWord, (int)sizeof(utf8MovedWord), kMovedWord, (int)(sizeof(kMovedWord)/sizeof(kMovedWord[0])));
    BuildUtf8String(utf8LongPathWord, (int)sizeof(utf8LongPathWord), kLongPathWord, (int)(sizeof(kLongPathWord)/sizeof(kLongPathWord[0])));

    if (!BuildProbePaths(result, rootOverride)) return 0;
    if (!EnsureDeepDirectoryTree(result)) return 0;

    RLTextFormatTo(dataPath, (int)sizeof(dataPath), "%s.bin", result->textPath);
    RLTextFormatTo(copyPath, (int)sizeof(copyPath), "%s.copy.txt", result->textPath);
    RLTextFormatTo(renameLeaf, (int)sizeof(renameLeaf), "%s_rename_%s_utf8.txt", utf8MovedWord, utf8LongPathWord);
    RLTextFormatTo(renamedPath, (int)sizeof(renamedPath), "%s%s%s", RLGetDirectoryPath(copyPath), sep, renameLeaf);
    RLTextFormatTo(movedDir, (int)sizeof(movedDir), "%s%smoved_%s_utf8", RLGetDirectoryPath(copyPath), sep, utf8PathWord);
    RLTextFormatTo(movedPath, (int)sizeof(movedPath), "%s%s%s_%s_utf8.txt", movedDir, sep, utf8MovedWord, utf8LongPathWord);
    result->saveTextOk = RLSaveFileText(result->textPath, payloadText) ? 1 : 0;
    if (!result->saveTextOk) return ProbeFail(result, "save-text");

    result->saveDataOk = RLSaveFileData(dataPath, (void *)payloadData, (int)sizeof(payloadData)) ? 1 : 0;
    if (!result->saveDataOk) return ProbeFail(result, "save-data");

    {
        char *loadedText = RLLoadFileText(result->textPath);
        result->loadTextOk = ((loadedText != NULL) && (strcmp(loadedText, payloadText) == 0)) ? 1 : 0;
        if (loadedText != NULL) RLUnloadFileText(loadedText);
        if (!result->loadTextOk) return ProbeFail(result, "load-text");
    }

    {
        unsigned char *loadedData = RLLoadFileData(dataPath, &readSize);
        result->loadDataOk = ((loadedData != NULL) &&
                              (readSize == (int)sizeof(payloadData)) &&
                              (memcmp(loadedData, payloadData, sizeof(payloadData)) == 0)) ? 1 : 0;
        if (loadedData != NULL) RLUnloadFileData(loadedData);
        if (!result->loadDataOk) return ProbeFail(result, "load-data");
    }

    {
        int copyResult = RLFileCopy(result->textPath, copyPath);
        result->copyOk = ((copyResult >= 0) && RLFileExists(copyPath)) ? 1 : 0;
    }
    if (!result->copyOk) return ProbeFail(result, "copy-text");

    {
        int renameResult = RLFileRename(copyPath, renameLeaf);
        result->renameOk = ((renameResult == 0) && RLFileExists(renamedPath) && !RLFileExists(copyPath)) ? 1 : 0;
    }
    if (!result->renameOk) return ProbeFail(result, "rename-text");

    if (RLMakeDirectory(movedDir) != 0) return ProbeFail(result, "make-moved-dir");

    {
        int moveResult = RLFileMove(renamedPath, movedPath);
        result->moveOk = ((moveResult == 0) && RLFileExists(movedPath) && !RLFileExists(renamedPath)) ? 1 : 0;
        result->movedPathLength = RLTextLength(movedPath);
    }
    if (!result->moveOk) return ProbeFail(result, "move-text");

    result->fileExistsOk = (RLFileExists(result->textPath) && RLFileExists(dataPath) && RLFileExists(movedPath)) ? 1 : 0;
    if (!result->fileExistsOk) return ProbeFail(result, "file-exists");

    result->dirExistsOk = RLDirectoryExists(RLGetDirectoryPath(result->textPath)) ? 1 : 0;
    if (!result->dirExistsOk) return ProbeFail(result, "dir-exists");

    result->changeDirOk = RLChangeDirectory(RLGetWorkingDirectory()) ? 1 : 0;
    if (!result->changeDirOk) return ProbeFail(result, "change-dir");

    {
        RLFilePathList files = RLLoadDirectoryFilesEx(RLGetDirectoryPath(result->textPath), ".txt;.bin", false);
        result->listCountOk = (files.count >= 2u) ? 1 : 0;
        RLUnloadDirectoryFiles(files);
    }
    if (!result->listCountOk) return ProbeFail(result, "list-dir");

    result->fileNameOk = (strcmp(RLGetFileName(result->textPath), "测试文本_长路径_utf8_你好世界.txt") == 0) ? 1 : 0;
    if (!result->fileNameOk) return ProbeFail(result, "file-name");

    result->fileStemOk = (strcmp(RLGetFileNameWithoutExt(result->textPath), "测试文本_长路径_utf8_你好世界") == 0) ? 1 : 0;
    if (!result->fileStemOk) return ProbeFail(result, "file-stem");

    result->dirPathOk = (strcmp(RLGetDirectoryPath(copyPath), RLGetDirectoryPath(result->textPath)) == 0) ? 1 : 0;
    if (!result->dirPathOk) return ProbeFail(result, "dir-path");

    result->fileLengthOk = (RLGetFileLength(result->textPath) > 0 && RLGetFileLength(dataPath) == (int)sizeof(payloadData)) ? 1 : 0;
    if (!result->fileLengthOk) return ProbeFail(result, "file-length");

    result->modTimeOk = (RLGetFileModTime(result->textPath) > 0) ? 1 : 0;
    if (!result->modTimeOk) return ProbeFail(result, "file-modtime");

    {
        char validName[64] = { 0 };
        RLTextFormatTo(validName, (int)sizeof(validName), "%s_%s_long.txt", utf8FileWord, utf8PathWord);
        result->validFileNameOk = RLIsFileNameValid(validName) ? 1 : 0;
        if (!result->validFileNameOk) return ProbeFail(result, "filename-valid");
    }

    if ((uncRootOverride != NULL) && (uncRootOverride[0] != '\0'))
    {
        char utf8UncWord[64] = { 0 };
#if defined(_WIN32)
        const char *sep = "\\";
#else
        const char *sep = "/";
#endif
        result->uncEnabled = 1;
        BuildUtf8String(utf8UncWord, (int)sizeof(utf8UncWord), kUncWord, (int)(sizeof(kUncWord)/sizeof(kUncWord[0])));
        RLTextFormatTo(uncDir, (int)sizeof(uncDir), "%s%sraylib_unc_%s", uncRootOverride, sep, utf8UncWord);
        RLTextFormatTo(uncFile, (int)sizeof(uncFile), "%s%sunc_utf8_%s.txt", uncDir, sep, utf8FileWord);

        if (!RLDirectoryExists(uncRootOverride)) result->uncOk = 0;
        if ((result->uncOk != 0) && (RLMakeDirectory(uncDir) != 0)) result->uncOk = 0;
        if ((result->uncOk != 0) && !RLSaveFileText(uncFile, "unc-ok")) result->uncOk = 0;
        if (result->uncOk != 0)
        {
            char *uncText = RLLoadFileText(uncFile);
            if ((uncText == NULL) || (strcmp(uncText, "unc-ok") != 0)) result->uncOk = 0;
            if (uncText != NULL) RLUnloadFileText(uncText);
        }
        if (result->uncOk == 0) return ProbeFail(result, "unc");
    }

    return ProbePass(result);
}

int main(int argc, char **argv)
{
    LongPathUtf8ProbeResult result = { 0 };
    const char *rootOverride = (argc >= 2) ? argv[1] : NULL;
    const char *uncRootOverride = (argc >= 3) ? argv[2] : NULL;

#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    (void)RunLongPathUtf8Probe(&result, rootOverride, uncRootOverride);

    printf("{\"ok\":%s,\"uncEnabled\":%s,\"uncOk\":%s,\"rootLen\":%d,\"textPathLen\":%d,\"movedPathLen\":%d,\"saveTextOk\":%d,"
           "\"loadTextOk\":%d,\"saveDataOk\":%d,\"loadDataOk\":%d,\"copyOk\":%d,\"renameOk\":%d,\"moveOk\":%d,"
           "\"fileExistsOk\":%d,\"dirExistsOk\":%d,\"changeDirOk\":%d,\"listCountOk\":%d,\"fileNameOk\":%d,"
           "\"fileStemOk\":%d,\"dirPathOk\":%d,\"fileLengthOk\":%d,\"modTimeOk\":%d,\"validFileNameOk\":%d,\"failStage\":\"%s\"}\n",
           result.ok ? "true" : "false",
           result.uncEnabled ? "true" : "false",
           result.uncOk ? "true" : "false",
           result.rootLength,
           result.textPathLength,
           result.movedPathLength,
           result.saveTextOk,
           result.loadTextOk,
           result.saveDataOk,
           result.loadDataOk,
           result.copyOk,
           result.renameOk,
           result.moveOk,
           result.fileExistsOk,
           result.dirExistsOk,
           result.changeDirOk,
           result.listCountOk,
           result.fileNameOk,
           result.fileStemOk,
           result.dirPathOk,
           result.fileLengthOk,
           result.modTimeOk,
           result.validFileNameOk,
           result.failStage);

    return result.ok ? 0 : 2;
}
