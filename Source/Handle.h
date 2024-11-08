#ifndef _CTRDL_HANDLE_H
#define _CTRDL_HANDLE_H

#include <dlfcn.h>

#include "ELFUtil.h"

#define CTRDL_MAX_HANDLES 16
#define CTRDL_MAX_DEPS 16

typedef void(*InitFiniFn)();

typedef struct {
    char* path;                 // Object path.
    u32 base;                   // Mirror address of mapped region.
    u32 origin;                 // Original address of mapped region.
    size_t size;                // Size of mapped region.
    size_t refc;                // Object refcount.
    size_t flags;               // Object flags.
    void* deps[CTRDL_MAX_DEPS]; // Object dependencies.
    InitFiniFn* finiArray;      // Fini array address.
    size_t numFiniEntries;      // Number of fini functions.
    size_t numSymBuckets;       // Number of symbol buckets;
    Elf32_Word* symBuckets;     // Symbol buckets.
    size_t numSymChains;        // Number of symbol chains (entries).
    Elf32_Word* symChains;      // Symbol chains.
    Elf32_Sym* symEntries;      // Symbol entries.
    char* stringTable;          // String table.
} CTRDLHandle;

void ctrdl_acquireHandleMtx(void);
void ctrdl_releaseHandleMtx(void);

CTRDLHandle* ctrdl_createHandle(const char* path, size_t flags);
void ctrdl_lockHandle(CTRDLHandle* handle);
bool ctrdl_unlockHandle(CTRDLHandle* handle);

CTRDLHandle* ctrdl_unsafeGetHandleByIndex(size_t index);
CTRDLHandle* ctrdl_unsafeFindHandleByName(const char* name);
CTRDLHandle* ctrdl_unsafeFindHandleByAddr(u32 addr);

#endif /* _CTRDL_HANDLE_H */