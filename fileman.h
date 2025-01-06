/* 
 * taken form https://github.com/asivery/rmpp-xovi-extensions/blob/master/fileman/src/fileman.h
 */

extern "C"{
#include <stdio.h>
#define FILEMAN_MATCH_WHOLE         1
#define FILEMAN_MATCH_START         2
#define FILEMAN_MATCH_END           3

#define FILEMAN_HANDLE_DIRECT       1
#define FILEMAN_HANDLE_NAME_MAP     2
#define FILEMAN_HANDLE_FILE_MAP     3
#define FILEMAN_HANDLE_FD_MAP       4

#define FILEMAN_SIMPLE_ILLEGAL      0
#define FILEMAN_SIMPLE_READ         1
#define FILEMAN_SIMPLE_WRITE        2

union OverrideHandler {
    const char *directRemap;
    const char *(*fileNameRemap)(const char *filePath, const char *mode, int flags, int modeI);
    FILE *(*fileRemap)(const char *filePath, const char *mode, int flags, int modeI);
    int(*fdRemap)(const char *filePath, const char *mode, int flags, int modeI);
};

struct FilemanOverride {
    const char *name;
    int nameMatchType;
    int handlerType;
    union OverrideHandler handler;

    struct FilemanOverride *next;
};

extern void fileman$registerOverride(struct FilemanOverride *override);
extern int fileman$fopenToOpenFlags(const char *mode);
extern int fileman$anyFlagsToSyscall(const char *mode, int flags, int iMode);
extern int fileman$anyFlagsToSimple(const char *mode, int flags, int iMode);
}
