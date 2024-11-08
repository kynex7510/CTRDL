#ifndef _CTRDL_DLFCN_H
#define _CTRDL_DLFCN_H

#include <3ds.h>
#include <sys/types.h>
#include <stdio.h>

#define RTLD_LOCAL 0x0000
#define RTLD_LAZY 0x0001     // Unsupported.
#define RTLD_NOW 0x0002
#define RTLD_NOLOAD 0x0004
#define RTLD_DEEPBIND 0x0008 // Unsupported.
#define RTLD_GLOBAL 0x0100
#define RTLD_NODELETE 0x1000 // Unsupported.

typedef void*(*CTRDLResolverFn)(const char* sym, void* userData);
typedef void(*CTRDLEnumerateFn)(void* handle);

typedef struct {
    const char* dli_fname; // Object path.
    void* dli_fbase;       // Object base address.
    const char* dli_sname; // Symbol which overlaps the address.
    void* dli_saddr;       // Actual address for the symbol.
} Dl_info;

typedef struct {
    char* path;      // Path.
    size_t pathSize; // Path size.
    u32 base;        // Base address.
    size_t size;     // Size.
} CTRDLInfo;

#if defined(__cplusplus)
extern "C" {
#endif

void* dlopen(const char* path, int flags);
int dlclose(void* handle);
void* dlsym(void* handle, const char* name);
int dladdr(const void* addr, Dl_info* info);
const char* dlerror(void);

void* ctrdlOpen(const char* path, int flags, CTRDLResolverFn resolver, void* resolverUserData);
void* ctrdlFOpen(FILE* f, int flags, CTRDLResolverFn resolver, void* resolverUserData);
void* ctrdlMap(const void* buffer, size_t size, int flags, CTRDLResolverFn resolver, void* resolverUserData);
void* ctrdlHandleByAddress(u32 addr);
void* ctrdlThisHandle(void);
void ctrdlEnumerate(CTRDLEnumerateFn callback);
bool ctrdlInfo(void* handle, CTRDLInfo* info);
void ctrdlFreeInfo(CTRDLInfo* info);

#if defined(__cplusplus)
}
#endif

#endif /* _CTRDL_DLFCN_H */