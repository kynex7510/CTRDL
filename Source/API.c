/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CTRL/App.h>
#include <CTRL/Memory.h>

#include "Handle.h"
#include "Error.h"
#include "Loader.h"
#include "Symbol.h"

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

__attribute__((weak)) void* ctrdlProgramResolver(const char* symName) { return NULL; }

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
const char* dlerror(void) { return ctrdl_getLastError(); }

int dlclose(void* handle) {
    if (handle != CTRDL_MAIN_HANDLE)
        return !ctrdl_unlockHandle((CTRDLHandle*)handle);

    return 0;
}

void* dlsym(void* handle, const char* name) {
    if (!handle) {
        ctrdl_setLastError("Handle is NULL");
        return NULL;
    }

    if (!name) {
        ctrdl_setLastError("Name is NULL");
        return NULL;
    }

    // Handle main handle (load order).
    if (handle == CTRDL_MAIN_HANDLE) {
        // Look into program symbols.
        void* addr = ctrdlProgramResolver(name);

        if (!addr) {
            // Look into global objects.
            ctrdl_acquireHandleMtx();

            for (size_t i = 0; i < ctrdl_unsafeNumHandles(); ++i) {
                CTRDLHandle* h = ctrdl_unsafeGetHandleByIndex(i);
                if (h->flags & RTLD_GLOBAL) {
                    const Elf32_Sym* sym = ctrdl_symNameLookupSingle(h, name);
                    if (sym) {
                        addr = (void*)(ctrlPageIndexToAddr(h->basePage) + sym->st_value);
                        break;
                    }
                }
            }

            ctrdl_releaseHandleMtx();
        }

        if (!addr)
            ctrdl_setLastError("Not found");

        return addr;
    }

    // Handle other handles (dep order).
    CTRDLHandle* h = (CTRDLHandle*)handle;
    const Elf32_Sym* sym = ctrdl_symNameLookupDepOrder(h, name);
    if (sym)
        return (void*)(ctrlPageIndexToAddr(h->basePage) + sym->st_value);

    ctrdl_setLastError("Not found");
    return NULL;
}

int dladdr(const void* address, Dl_info* info) {
    if (!info)
        return 0;

    const u32 addr = (u32)address;
    CTRDLHandle* h = ctrdlHandleByAddress(addr);
    if (h) {
        const u32 base = ctrlPageIndexToAddr(h->basePage);

        info->dli_fname = h->path;
        info->dli_fbase = (void*)base;

        const Elf32_Sym* sym = ctrdl_symValueLookupSingle(h, addr - base);
        if (sym) {
            info->dli_sname = &h->stringTable[sym->st_name];
            info->dli_saddr = (void*)(base + sym->st_value);
        } else {
            info->dli_sname = NULL;
            info->dli_saddr = NULL;
        }

        ctrdl_unlockHandle(h);
    }

    return info->dli_fbase != NULL;
}

void* ctrdlOpen(const char* path, int flags, CTRDLResolverFn resolver, void* resolverUserData) {
    if (!ctrdl_checkFlags(flags)) {
        ctrdl_setLastError("Invalid flags");
        return NULL;
    }

    if (!path)
        return CTRDL_MAIN_HANDLE;

    // Avoid reading if already open.
    ctrdl_acquireHandleMtx();
    CTRDLHandle* handle = ctrdl_unsafeFindHandleByName(path);

    if (handle) {
        ctrdl_lockHandle(handle);

        // Update flags.
        // Once GLOBAL, forever GLOBAL.
        if (handle->flags & RTLD_GLOBAL)
            flags &= ~(RTLD_LOCAL);

        handle->flags = (flags & ~(RTLD_NOLOAD));
        ctrdl_releaseHandleMtx();
        return (void*)handle;
    }

    ctrdl_releaseHandleMtx();

    if (flags & RTLD_NOLOAD) {
        ctrdl_setLastError("Module is not loaded");
        return NULL;
    }

    // Open file for reading.
    FILE* f = fopen(path, "rb");
    if (!f) {
        ctrdl_setLastError("File open failed");
        return NULL;
    }

    CTRDLStream stream;
    ctrdl_makeFileStream(&stream, f);
    handle = ctrdl_loadObject(path, flags, &stream, resolver, resolverUserData);

    fclose(f);
    return handle;
}

void* ctrdlFOpen(FILE* f, int flags, CTRDLResolverFn resolver, void* resolverUserData) {
    CTRDLStream stream;
    ctrdl_makeFileStream(&stream, f);
    return ctrdlStreamOpen(&stream, flags, resolver, resolverUserData);
}

void* ctrdlStreamOpen(CTRDLStream* stream, int flags, CTRDLResolverFn resolver, void* resolverUserData) {
    if (!stream) {
        ctrdl_setLastError("Stream is NULL");
        return NULL;
    }
    
    if (!ctrdl_checkFlags(flags) || (flags & RTLD_NOLOAD)) {
        ctrdl_setLastError("Invalid flags");
        return NULL;
    }

    return ctrdl_loadObject(NULL, flags, stream, resolver, resolverUserData);
}

void* ctrdlMap(const void* buffer, size_t size, int flags, CTRDLResolverFn resolver, void* resolverUserData) {
    if (!buffer) {
        ctrdl_setLastError("Buffer is NULL");
        return NULL;
    }

    if (!size) {
        ctrdl_setLastError("Size is NULL");
        return NULL;
    }
    
    if (!ctrdl_checkFlags(flags) || (flags & RTLD_NOLOAD)) {
        ctrdl_setLastError("Invalid flags");
        return NULL;
    }

    CTRDLStream stream;
    ctrdl_makeMemStream(&stream, buffer, size);
    return ctrdl_loadObject(NULL, flags, &stream, resolver, resolverUserData);
}

void* ctrdlHandleByAddress(u32 addr) {
    ctrdl_acquireHandleMtx();
    CTRDLHandle* handle = ctrdl_unsafeFindHandleByAddr(addr);
    if (handle) {
        ctrdl_lockHandle(handle);
    } else {
        ctrdl_setLastError("Not found");
    }
    ctrdl_releaseHandleMtx();
    return handle;
}

void* ctrdlThisHandle(void) {
    void* h = ctrdlHandleByAddress((u32)__builtin_extract_return_addr(__builtin_return_address(0)));

    // If this is not being called by an object, return the pseudo handle for the process.
    if (!h)
        h = CTRDL_MAIN_HANDLE;

    return h;
}

void ctrdlEnumerate(CTRDLEnumerateFn callback) {
    if (!callback) {
        ctrdl_setLastError("Callback is NULL");
        return;
    }

    callback(CTRDL_MAIN_HANDLE);

    ctrdl_acquireHandleMtx();

    for (size_t i = 0; i < ctrdl_unsafeNumHandles(); ++i)
        callback(ctrdl_unsafeGetHandleByIndex(i));

    ctrdl_releaseHandleMtx();
}

bool ctrdlInfo(void* handle, CTRDLInfo* info) {
    if (!handle) {
        ctrdl_setLastError("Handle is NULL");
        return false;
    }

    if (!handle || !info) {
        ctrdl_setLastError("Info is NULL");
        return false;
    }

    if (handle == CTRDL_MAIN_HANDLE) {
        const CTRLAppSectionInfo* appSectionInfo = ctrlAppSectionInfo();
        info->path = NULL;
        info->pathSize = 0;
        info->base = appSectionInfo->textAddr;
        info->size = appSectionInfo->textSize + appSectionInfo->rodataSize + appSectionInfo->dataSize;
        return true;
    }

    CTRDLHandle* h = (CTRDLHandle*)handle;
    ctrdl_lockHandle(h);

    bool success = true;
    if (h->path) {
        info->pathSize = strlen(h->path);
        info->path = malloc(info->pathSize + 1);
        if (info->path) {
            memcpy(info->path, h->path, info->pathSize);
            info->path[info->pathSize] = '\0';
        } else {
            ctrdl_setLastError("Path allocation failed");
            success = false;
        }
    } else {
        info->path = NULL;
        info->pathSize = 0;
    }

    info->base = ctrlPageIndexToAddr(h->basePage);
    info->size = ctrlNumPagesToSize(h->numPages);

    ctrdl_unlockHandle(h);
    return success;
}

void ctrdlFreeInfo(CTRDLInfo* info) {
    if (info)
        free(info->path);
}