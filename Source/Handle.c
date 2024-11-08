#include "Handle.h"
#include "Error.h"
#include "Loader.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    CTRDLHandle** handles;
    size_t size;
    size_t capacity;
} HandleList;

static HandleList g_HandleList = {};
static RecursiveLock g_Mtx;

static void ctrdl_handleMtxLazyInit(void) {
    static u8 initialized = 0;

    if (!__ldrexb(&initialized)) {
        RecursiveLock_Init(&g_Mtx);

        while (__strexb(&initialized, 1))
            __ldrexb(&initialized);
    } else {
        __clrex();
    }
}

void ctrdl_acquireHandleMtx(void) {
    ctrdl_handleMtxLazyInit();
    RecursiveLock_Lock(&g_Mtx);
}

void ctrdl_releaseHandleMtx(void) {
    ctrdl_handleMtxLazyInit();
    RecursiveLock_Unlock(&g_Mtx);
}

static CTRL_INLINE bool ctrdl_handleListIsFull(void) { return g_HandleList.size >= CTRDL_MAX_HANDLES; }
static CTRL_INLINE bool ctrdl_handleListIsEmpty(void) { return g_HandleList.size == 0; }

// Assumes the list is not full.
static bool ctrdl_handleListInsert(CTRDLHandle* handle) {
    // Initialize if not done already.
    if (!g_HandleList.handles) {
        g_HandleList.handles = malloc(sizeof(CTRDLHandle*));
        if (!g_HandleList.handles)
            return false;

        g_HandleList.size = 0;
        g_HandleList.capacity = 1;
    }

    // Increase capacity if required.
    if (g_HandleList.size >= g_HandleList.capacity) {
        size_t newCapacity = (g_HandleList.capacity * 2);
        if (newCapacity > CTRDL_MAX_HANDLES)
            newCapacity = CTRDL_MAX_HANDLES;

        void* p = realloc(g_HandleList.handles, sizeof(CTRDLHandle*) * newCapacity);
        if (!p)
            return false;

        g_HandleList.handles = p;
        g_HandleList.capacity = newCapacity;
    }

    g_HandleList.handles[g_HandleList.size++] = handle;
    return true;
}

static void ctrdl_handleListRemove(CTRDLHandle* handle) {
    if (!g_HandleList.handles)
        return;

    // Find handle to remove.
    size_t index = 0;
    while (index < g_HandleList.size) {
        if (g_HandleList.handles[index] == handle)
            break;

        ++index;
    }

    if (index >= g_HandleList.size)
        return;

    // Ensure the buffer remains contiguous.
    while (index < (g_HandleList.size - 1)) {
        g_HandleList.handles[index] = g_HandleList.handles[index + 1];
        ++index;
    }

    --g_HandleList.size;

    // Resize if possible.
    if (g_HandleList.size <= (g_HandleList.capacity / 2)) {
        if (!g_HandleList.size) {
            free(g_HandleList.handles);
            g_HandleList.handles = NULL;
            g_HandleList.capacity = 0;
            return;
        }

        const size_t newCapacity = g_HandleList.capacity / 2;
        void* p = realloc(g_HandleList.handles, sizeof(CTRDLHandle*) * newCapacity);
        if (p) {
            g_HandleList.handles = p;
            g_HandleList.capacity = newCapacity;
        }
    }
}

CTRDLHandle* ctrdl_createHandle(const char* path, size_t flags) {
    size_t pathSize = 0;
    char* pathCopy = NULL;
    if (path) {
        pathSize = strlen(path);
        pathCopy = malloc(pathSize + 1);
        if (!pathCopy) {
            ctrdl_setLastError(Err_NoMemory);
            return NULL;
        }
    }

    ctrdl_acquireHandleMtx();

    if (ctrdl_handleListIsFull()) {
        ctrdl_setLastError(Err_HandleLimit);
        ctrdl_releaseHandleMtx();
        return NULL;
    }

    // Create and initialize handle.
    CTRDLHandle* handle = malloc(sizeof(CTRDLHandle));
    if (!handle) {
        ctrdl_releaseHandleMtx();
        ctrdl_setLastError(Err_NoMemory);
        free(pathCopy);
        return NULL;
    }

    if (pathCopy) {
        memcpy(pathCopy, path, pathSize);
        pathCopy[pathSize] = '\0';
    }

    handle->path = pathCopy;
    handle->flags = flags;
    handle->refc = 1;

    // Insert handle in list.
    if (!ctrdl_handleListInsert(handle)) {
        ctrdl_setLastError(Err_NoMemory);
        free(handle);
        free(pathCopy);
    }

    ctrdl_releaseHandleMtx();
    return handle;
}

void ctrdl_lockHandle(CTRDLHandle* handle) {
    if (handle && (handle != CTRDL_MAIN_HANDLE)) {
        ctrdl_acquireHandleMtx();
        ++handle->refc;
        ctrdl_releaseHandleMtx();
    }
}

bool ctrdl_unlockHandle(CTRDLHandle* handle) {
    bool ret = true;

    if (handle && (handle != CTRDL_MAIN_HANDLE)) {
        ctrdl_acquireHandleMtx();

        if (handle->refc)
            --handle->refc;

        if (!handle->refc) {
            ret = ctrdl_unloadObject(handle);
            if (ret) {
                ctrdl_handleListRemove(handle);
                free(handle->path);
                free(handle->symBuckets);
                free(handle->symChains);
                free(handle->symEntries);
                free(handle->stringTable);
                free(handle);
            }
        }

        ctrdl_releaseHandleMtx();
    } else {
        ctrdl_setLastError(Err_InvalidParam);
        ret = false;
    }

    return ret;
}

size_t ctrdl_unsafeNumHandles(void) { return g_HandleList.size; }

CTRDLHandle* ctrdl_unsafeGetHandleByIndex(size_t index) {
    if (index < g_HandleList.size)
        return g_HandleList.handles[index];
    
    return NULL;
}

CTRDLHandle* ctrdl_unsafeFindHandleByName(const char* name) {
    CTRDLHandle* found = NULL;

    for (size_t i = 0; i < ctrdl_unsafeNumHandles(); ++i) {
        CTRDLHandle* h = ctrdl_unsafeGetHandleByIndex(i);
        if (h->path && strstr(h->path, name)) {
            found = h;
            break;
        }
    }

    return found;
}

CTRDLHandle* ctrdl_unsafeFindHandleByAddr(u32 addr) {
    CTRDLHandle* found = NULL;

    for (size_t i = 0; i < ctrdl_unsafeNumHandles(); ++i) {
        CTRDLHandle* h = ctrdl_unsafeGetHandleByIndex(i);
        if ((addr >= h->base) && (addr <= (h->base + h->size))) {
            found = h;
            break;
        }
    }

    return found;
}