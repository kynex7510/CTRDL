#include <dlfcn.h>

#include <3ds.h>
#include <stdio.h>

extern void interop(const char* msg) {
    PrintConsole console;
    consoleInit(GFX_BOTTOM, &console);
    consoleSelect(&console);

    void* handle = ctrdlThisHandle();
    if (!handle) {
        printf("ERROR: %s.\n", dlerror());
        return;
    }

    CTRDLInfo info;
    if (!ctrdlInfo(handle, &info)) {
        printf("ERROR: %s.\n", dlerror());
        return;
    }

    printf("Hello, I'm \"%s\"\n", info.path);
    printf("Program says: %s\n", msg);

    ctrdlFreeInfo(&info);

    if (dlclose(handle))
        printf("ERROR: %s.\n", dlerror());
}