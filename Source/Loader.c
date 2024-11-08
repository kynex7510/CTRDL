#include "CTRL/Memory.h"

#include "Loader.h"
#include "Handle.h"
#include "ELFUtil.h"
#include "Relocs.h"

#include <stdlib.h>
#include <string.h>

#define CODE_BASE 0x100000
#define CODE_SIZE 0x3F00000

typedef struct {
    CTRDLHandle* handle;
    CTRDLStream* stream;
    CTRDLElf elf;
    CTRDLResolverFn resolver;
    void* resolverUserData;
} LdrData;

static MemPerm ctrdl_wrapPerms(Elf32_Word flags) {
    switch (flags) {
        case PF_R:
            return MEMPERM_READ;
        case PF_W:
            return MEMPERM_WRITE;
        case PF_X:
            return MEMPERM_EXECUTE;
        case PF_R | PF_W:
            return MEMPERM_READWRITE;
        case PF_R | PF_X:
            return MEMPERM_READEXECUTE;
        default:
            return (MemPerm)0;
    }
}

static char* ctrdl_getDepPath(const char* basePath, const char* name) {
    if (!basePath || !name)
        return NULL;

    const size_t baseSize = strlen(basePath);
    const size_t nameSize = strlen(name);
    char* buffer = malloc(baseSize + nameSize);
    if (buffer) {
        memcpy(buffer, basePath, baseSize);
        buffer[baseSize] = '\0';

        char* delim = strrchr(buffer, '/');
        if (delim) {
            ++delim;
            memcpy(delim, name, nameSize);
            delim[nameSize] = '\0';
        } else {
            memcpy(buffer, name, nameSize);
            buffer[nameSize] = '\0';
        }
    }

    return buffer;
}

static bool ctrdl_loadDeps(LdrData* ldrData) {
    const size_t depCount = ctrdl_getELFNumDynEntriesWithTag(&ldrData->elf, DT_NEEDED);
    if (depCount > CTRDL_MAX_DEPS) {
        ctrdl_setLastError(Err_DepsLimit);
        return false;
    }

    Elf32_Dyn depEntries[CTRDL_MAX_DEPS];
    const size_t actualDepCount = ctrdl_getELFDynEntriesWithTag(&ldrData->elf, DT_NEEDED, depEntries, CTRDL_MAX_DEPS);
    if (actualDepCount != depCount) {
        ctrdl_setLastError(Err_DepFailed);
        return false;
    }

    for (size_t i = 0; i < depCount; ++i) {
        char* depPath = ctrdl_getDepPath(ldrData->handle->path, ldrData->elf.stringTable + depEntries[i].d_un.d_ptr);
        void* depHandle = ctrdlOpen(depPath, RTLD_NOW | RTLD_LOCAL, ldrData->resolver, ldrData->resolverUserData);
        free(depPath);

        if (!depHandle) {
            ctrdl_setLastError(Err_DepFailed);
            return false;
        }

        ldrData->handle->deps[i] = depHandle;
    }

    return true;
}

static bool ctrdl_mapObject(LdrData* ldrData) {
    CTRDLHandle* handle = ldrData->handle;

    // Load dependencies.
    if (!ctrdl_loadDeps(ldrData)) {
        // References may be resolved by the user.
        if (!ldrData->resolver) {
            ctrdl_unloadObject(handle);
            return false;
        }
    }

    // Calculate allocation space for load segments.
    const size_t numSegments = ctrdl_getELFNumSegmentsByType(&ldrData->elf, PT_LOAD);
    if (!numSegments) {
        ctrdl_setLastError(Err_InvalidObject);
        ctrdl_unloadObject(handle);
        return false;
    }

    Elf32_Phdr* loadSegments = malloc(numSegments *  sizeof(Elf32_Phdr));
    if (!loadSegments) {
        ctrdl_setLastError(Err_NoMemory);
        ctrdl_unloadObject(handle);
        return false;
    }

    const size_t actualNumSegments = ctrdl_getELFSegmentsByType(&ldrData->elf, PT_LOAD, loadSegments, numSegments);
    if (actualNumSegments != numSegments) {
        ctrdl_setLastError(Err_InvalidObject);
        ctrdl_unloadObject(handle);
        free(loadSegments);
        return false;
    }

    for (size_t i = 0; i < numSegments; ++i) {
        const Elf32_Phdr* segment = &loadSegments[i];

        if (segment->p_memsz < segment->p_filesz) {
            ctrdl_setLastError(Err_InvalidObject);
            ctrdl_unloadObject(handle);
            free(loadSegments);
            return false;
        }

        if (segment->p_align > 1) {
            handle->size += ctrlAlignSize(segment->p_memsz, segment->p_align);
        } else {
            handle->size += segment->p_memsz;
        }
    }

    handle->size = ctrlAlignSize(handle->size, CTRL_PAGE_SIZE);
    
    // Allocate and map segments.
    handle->origin = (u32)aligned_alloc(CTRL_PAGE_SIZE, handle->size);
    if (!handle->origin) {
        ctrdl_setLastError(Err_NoMemory);
        ctrdl_unloadObject(handle);
        free(loadSegments);
        return false;
    }

    for (size_t i = 0; i < numSegments; ++i) {
        const Elf32_Phdr* segment = &loadSegments[i];

        if (!ldrData->stream->seek(ldrData->stream, segment->p_offset)) {
            ctrdl_setLastError(Err_ReadFailed);
            ctrdl_unloadObject(handle);
            free(loadSegments);
            return false;
        }

        if (!ldrData->stream->read(ldrData->stream, (void*)(handle->origin + segment->p_vaddr), segment->p_filesz)) {
            ctrdl_setLastError(Err_ReadFailed);
            ctrdl_unloadObject(handle);
            free(loadSegments);
            return false;
        }
    }

    size_t processedSize = 0;
    MemInfo memInfo;
    memInfo.base_addr = CODE_BASE;
    while (true) {
        if (R_FAILED(ctrlQueryRegion(memInfo.base_addr + processedSize, &memInfo))) {
            ctrdl_setLastError(Err_MapFailed);
            ctrdl_unloadObject(handle);
            free(loadSegments);
            return false;
        }

        if (memInfo.base_addr >= (CODE_BASE + CODE_SIZE)) {
            ctrdl_setLastError(Err_NoMemory);
            ctrdl_unloadObject(handle);
            free(loadSegments);
            return false;
        }

        if ((memInfo.state == MEMSTATE_FREE) && (memInfo.size >= handle->size))
            break;

        processedSize += memInfo.size;
    };

    handle->base = memInfo.base_addr;
    if (R_FAILED(ctrlMirror(handle->base, handle->origin, handle->size))) {
        ctrdl_setLastError(Err_MapFailed);
        ctrdl_unloadObject(handle);
        free(loadSegments);
        return false;
    }

    // Apply relocations.
    if (!ctrdl_handleRelocs(handle, &ldrData->elf, ldrData->resolver, ldrData->resolverUserData)) {
        ctrdl_setLastError(Err_RelocFailed);
        ctrdl_unloadObject(handle);
        free(loadSegments);
        return false;
    }

    // Set correct permissions.
    for (size_t i = 0; i < numSegments; ++i) {
        const Elf32_Phdr* segment = &loadSegments[i];
        const u32 base = handle->base + segment->p_vaddr;
        const size_t alignedSize = ctrlAlignSize(segment->p_memsz, segment->p_align);
        const MemPerm perms = ctrdl_wrapPerms(segment->p_flags);

        if (R_FAILED(ctrlChangePerms(base, alignedSize, perms))) {
            ctrdl_setLastError(Err_MapFailed);
            ctrdl_unloadObject(handle);
            free(loadSegments);
            return false;
        }
    }

    if (R_FAILED(ctrlFlushCache(CTRL_ICACHE | CTRL_DCACHE))) {
        ctrdl_setLastError(Err_MapFailed);
        ctrdl_unloadObject(handle);
        free(loadSegments);
        return false;
    }

    free(loadSegments);

    // Run initializers.
    Elf32_Dyn initEntry;
    const bool hasInitArr = ctrdl_getELFDynEntryWithTag(&ldrData->elf, DT_INIT_ARRAY, &initEntry);

    Elf32_Dyn initEntrySize;
    const bool hasInitSz = ctrdl_getELFDynEntryWithTag(&ldrData->elf, DT_INIT_ARRAYSZ, &initEntrySize);

    if (hasInitArr && hasInitSz) {
        const Elf32_Addr* initArray = (const Elf32_Addr*)(handle->base + initEntry.d_un.d_ptr);
        const size_t numEntries = initEntrySize.d_un.d_val / sizeof(Elf32_Addr);
        for (size_t i = 0; i < numEntries; ++i)
            ((InitFiniFn)(initArray[i]))();
    }

    // Fill additional data.
    Elf32_Dyn finiEntry;
    const bool hasFiniArr = ctrdl_getELFDynEntryWithTag(&ldrData->elf, DT_FINI_ARRAY, &finiEntry);

    Elf32_Dyn finiEntrySize;
    const bool hasFiniSz = ctrdl_getELFDynEntryWithTag(&ldrData->elf, DT_FINI_ARRAYSZ, &finiEntrySize);

    if (hasFiniArr && hasFiniSz) {
        handle->finiArray = (InitFiniFn*)(handle->base + finiEntry.d_un.d_ptr);
        handle->numFiniEntries = finiEntrySize.d_un.d_val / sizeof(Elf32_Addr);
    }

    handle->numSymBuckets = ldrData->elf.numSymBuckets;
    handle->symBuckets = ldrData->elf.symBuckets;
    handle->numSymChains = ldrData->elf.numSymChains;
    handle->symChains = ldrData->elf.symChains;
    handle->symEntries = ldrData->elf.symEntries;
    handle->stringTable = ldrData->elf.stringTable;
    ldrData->elf.symBuckets = NULL;
    ldrData->elf.symChains = NULL;
    ldrData->elf.symEntries = NULL;
    ldrData->elf.stringTable = NULL;
    return true;
}

CTRDLHandle* ctrdl_loadObject(const char* name, int flags, CTRDLStream* stream, CTRDLResolverFn resolver, void* resolverUserData) {
    LdrData ldrData;
    ldrData.handle = ctrdl_createHandle(name, flags);
    if (!ldrData.handle)
        return NULL;

    if (!ctrdl_parseELF(stream, &ldrData.elf)) {
        ctrdl_unlockHandle(ldrData.handle);
        return NULL;
    }

    ldrData.stream = stream;
    ldrData.resolver = resolver;
    ldrData.resolverUserData = resolverUserData;
    if (!ctrdl_mapObject(&ldrData)) {
        ctrdl_unlockHandle(ldrData.handle);
        ldrData.handle = NULL;
    }

    ctrdl_freeELF(&ldrData.elf);
    return ldrData.handle;
}

bool ctrdl_unloadObject(CTRDLHandle* handle) {
    // Run finalizers.
    // TODO: reverse order??
    if (handle->finiArray) {
        for (size_t i = 0; i < handle->numFiniEntries; ++i)
            handle->finiArray[i]();
    }

    // Unmap segments.
    if (handle->base) {
        if (R_FAILED(ctrlUnmirror(handle->base, handle->origin, handle->size))) {
            ctrdl_setLastError(Err_FreeFailed);
            return false;
        }

        handle->base = 0;
    }

    if (handle->origin) {
        free((void*)handle->origin);
        handle->origin = 0;
        handle->size = 0;
    }

    // Unload dependencies.
    for (size_t i = 0; i < CTRDL_MAX_DEPS; ++i) {
        CTRDLHandle* dep = (CTRDLHandle*)handle->deps[i];
        if (dep)
            ctrdl_unlockHandle(dep);
    }

    free(handle->symBuckets);
    free(handle->symChains);
    free(handle->symEntries);
    free(handle->stringTable);
    return true;
}