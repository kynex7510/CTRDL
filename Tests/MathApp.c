
#include <dlfcn.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*DoMathFn)(int, int);

void __register_frame_info(const void*, struct object*) {}
void *__deregister_frame_info(const void *) {}

static void enumerateCallback(void* handle) {
    CTRDLInfo info;
    printf("Handle value: 0x%08lx\n", (u32)handle);
    if (ctrdlInfo(handle, &info)) {
        printf("- Path: %s\n", info.path ? info.path : "(unknown)");
        printf("- Base address: 0x%08lx\n", info.base);
        printf("- Size: 0x%08x\n", info.size);
        ctrdlFreeInfo(&info);
    } else {
        printf("ctrdlInfo() failed: %s\n", dlerror());
    }

    printf("---------------\n");
}

int main(int argc, char* argv[]) {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    
    srand(time(NULL));

    printf("Loading library...\n");
    void* h = dlopen("sdmc:/libmath.so", RTLD_NOW);
    if (!h)
        goto fail;

    ctrdlEnumerate(enumerateCallback);

    printf("Looking for symbol...\n");
    DoMathFn doMath = (DoMathFn)dlsym(h, "doMath");
    if (!doMath)
        goto fail;

    printf("- Address of doMath(): 0x%08x\n", doMath);  
    doMath(rand() % 14492, rand() % 9572);

    printf("Unloading library...\n");
    if (dlclose(h))
        goto fail;

    printf("Success!\n");
    goto end;

fail:
    printf("ERROR: %s.\n", dlerror());

end:
    while (aptMainLoop()) {
        gspWaitForVBlank();
        gfxSwapBuffers();
        hidScanInput();

        u32 kDown = hidKeysDown();
        if (kDown & KEY_START)
            break;
    }

    gfxExit();
    return 0;
}