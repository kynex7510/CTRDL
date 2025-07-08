#include "Symbol.h"

typedef struct {
    CTRDLHandle* deps[CTRDL_MAX_HANDLES];
    size_t size;
    size_t index;
} DepQueue;

static inline void ctrdl_depQueueInit(DepQueue* q) {
    q->size = 0;
    q->index = 0;
}

static inline bool ctrdl_depQueueIsEmpty(DepQueue* q) { return q->size == 0; }
static inline bool ctrdl_depQueueIsFull(DepQueue* q) { return q->size >= CTRDL_MAX_HANDLES; }

static void ctrdl_depQueuePush(DepQueue* q, CTRDLHandle* handle) {
    if (handle && !ctrdl_depQueueIsFull(q)) {
        for (size_t i = 0; i < q->size; ++i) {
            if (q->deps[i] == handle)
                return;
        }

        q->deps[q->size % CTRDL_MAX_HANDLES] = handle;
        ++q->size;
    }
}

static CTRDLHandle* ctrdl_depQueuePop(DepQueue* q) {
    if (!ctrdl_depQueueIsEmpty(q)) {
        CTRDLHandle* h = q->deps[q->index];
        q->index = (q->index + 1) % CTRDL_MAX_HANDLES;
        --q->size;
        return h;
    }

    return NULL;
}

const Elf32_Sym* ctrdl_symNameLookupSingle(CTRDLHandle* handle, const char* name) {
    const Elf32_Sym* found = NULL;

    if (handle) {
        ctrdl_lockHandle(handle);

        if (handle->numSymChains) {
            const Elf32_Word hash = ctrdl_getELFSymNameHash(name);
            size_t chainIndex = handle->symBuckets[hash % handle->numSymBuckets];

            while (chainIndex != STN_UNDEF) {
                const Elf32_Sym* sym = &handle->symEntries[chainIndex];
                if (!strcmp(&handle->stringTable[sym->st_name], name)) {
                    found = sym;
                    break;
                }

                chainIndex = handle->symChains[chainIndex];
            }
        }

        ctrdl_unlockHandle(handle);
    }

    return found;
}

const Elf32_Sym* ctrdl_symNameLookupLoadOrder(CTRDLHandle* handle, const char* name) {
    const Elf32_Sym* found = NULL;

    if (handle) {
        ctrdl_lockHandle(handle);

        found = ctrdl_symNameLookupSingle(handle, name);
        if (!found) {
            for (size_t i = 0; i < CTRDL_MAX_DEPS; ++i) {
                found = ctrdl_symNameLookupLoadOrder(handle->deps[i], name);
                if (found)
                    break;
            }
        }

        ctrdl_unlockHandle(handle);
    }

    return found;
}

const Elf32_Sym* ctrdl_symNameLookupDepOrder(CTRDLHandle* handle, const char* name) {
    DepQueue q;
    const Elf32_Sym* found = NULL;

    if (handle) {
        ctrdl_lockHandle(handle);

        ctrdl_depQueueInit(&q);
        ctrdl_depQueuePush(&q, handle);

        while (!ctrdl_depQueueIsEmpty(&q)) {
            CTRDLHandle* h = ctrdl_depQueuePop(&q);
            found = ctrdl_symNameLookupSingle(h, name);
            if (found)
                break;

            for (size_t i = 0; i < CTRDL_MAX_DEPS; ++i)
                ctrdl_depQueuePush(&q, h->deps[i]);
        }

        ctrdl_unlockHandle(handle);
    }

    return found;
}

const Elf32_Sym* ctrdl_symValueLookupSingle(CTRDLHandle* handle, Elf32_Word value) {
    const Elf32_Sym* found = NULL;

    if (handle) {
        ctrdl_lockHandle(handle);

        for (size_t i = 0; i < handle->numSymChains; ++i) {
            if (i == STN_UNDEF)
                continue;

            const Elf32_Sym* sym = &handle->symEntries[i];
            if ((sym->st_value >= value) && (value < (sym->st_value + sym->st_size))) {
                found = sym;
                break;
            }
        }

        ctrdl_unlockHandle(handle);
    }

    return found;
}