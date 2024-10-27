#include "Handle.h"
#include "Error.h"
#include "Loader.h"
#include "Symbol.h"

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#define RTLD_LAZY 0x0001
#define RTLD_DEEPBIND 0x0008
#define RTLD_NODELETE 0x1000

static bool ctrdl_checkFlags(int flags) {
    // Unsupported flags.
    if (flags & (RTLD_LAZY | RTLD_DEEPBIND | RTLD_NODELETE))
        return false;

    // Required flags.
    if (!(flags & RTLD_NOW))
        return false;

    return true;
}

void* dlopen(const char* path, int flags) { return ctrdlOpen(path, flags, NULL, NULL); }
const char* dlerror(void) { return ctrdl_getErrorAsString(ctrdl_getLastError()); }

int dlclose(void* handle) { return !ctrdl_unlockHandle((CTRDLHandle*)handle); }

void* dlsym(void* handle, const char* symbol) {
    if (!handle || !symbol) {
        ctrdl_setLastError(Err_InvalidParam);
        return NULL;
    }

    u32 addr = ctrdl_findSymbolValue((CTRDLHandle*)handle, symbol);
    if (!addr)
        ctrdl_setLastError(Err_NotFound);

    return (void*)addr;
}

int dladdr(const void* addr, Dl_info* info) {
    if (!info)
        return 0;

    CTRDLHandle* h = ctrdlHandleByAddress((u32)addr);
    if (h) {
        info->dli_fname = h->path;
        info->dli_fbase = (void*)h->base;
            
        // TODO
        info->dli_sname = NULL;
        info->dli_saddr = NULL;

        ctrdl_unlockHandle(h);
    }

    // We dont have to provide error values for this function.
    ctrdl_clearLastError();
    return info->dli_fbase != NULL;
}

void* ctrdlOpen(const char* path, int flags, CTRDLResolverFn resolver, void* resolverUserData) {
    // We don't support the NULL pseudo handle.
    if (!path || !ctrdl_checkFlags(flags)) {
        ctrdl_setLastError(Err_InvalidParam);
        return NULL;
    }

    // Avoid reading if already open.
    ctrdl_acquireHandleMtx();
    CTRDLHandle* handle = ctrdl_unsafeFindHandleByName(path);
    ctrdl_lockHandle(handle);
    ctrdl_releaseHandleMtx();

    if (handle) {
        // Update flags.
        handle->flags = flags;
        return (void*)handle;
    }

    if (flags & RTLD_NOLOAD) {
        ctrdl_setLastError(Err_NotFound);
        return NULL;
    }

    // Open file for reading.
    FILE* f = fopen(path, "rb");
    if (!f) {
        ctrdl_setLastError(Err_InvalidParam);
        return NULL;
    }

    CTRDLStream stream;
    ctrdl_makeFileStream(&stream, f);
    handle = ctrdl_loadObject(path, flags, &stream, resolver, resolverUserData);

    fclose(f);
    return handle;
}

void* ctrdlHandleByAddress(u32 addr) {
    ctrdl_acquireHandleMtx();
    CTRDLHandle* handle = ctrdl_unsafeFindHandleByAddr(addr);
    if (handle) {
        ctrdl_lockHandle(handle);
    } else {
        ctrdl_setLastError(Err_NotFound);
    }
    ctrdl_releaseHandleMtx();
    return handle;
}

void* ctrdlThisHandle(void) { return ctrdlHandleByAddress((u32)__builtin_extract_return_addr(__builtin_return_address(0))); }

void ctrdlIterate(CTRDLIterateFn callback) {
    if (!callback) {
        ctrdl_setLastError(Err_InvalidParam);
        return;
    }

    ctrdl_acquireHandleMtx();

    for (size_t i = 0; i < CTRDL_MAX_HANDLES; ++i) {
        CTRDLHandle* h = ctrdl_unsafeGetHandleByIndex(i);
        if (h->refc)
            callback(h);
    }

    ctrdl_releaseHandleMtx();
}

bool ctrdlInfo(void* handle, CTRDLInfo* info) {
    if (!handle || !info) {
        ctrdl_setLastError(Err_InvalidParam);
        return false;
    }

    CTRDLHandle* h = (CTRDLHandle*)handle;
    ctrdl_lockHandle(h);

    bool err = false;
    if (h->path) {
        info->pathSize = strlen(h->path);;
        info->path = malloc(info->pathSize + 1);
        if (info->path) {
            memcpy(info->path, h->path, info->pathSize);
            info->path[info->pathSize] = '\0';
        } else {
            ctrdl_setLastError(Err_NoMemory);
            err = true;
        }
    } else {
        info->path = NULL;
        info->pathSize = 0;
    }

    info->base = h->base;
    info->size = h->size;

    ctrdl_unlockHandle(h);
    return err;
}

void ctrdlFreeExtInfo(CTRDLInfo* info) {
    if (info)
        free(info->path);
}