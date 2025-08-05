/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Relocs.h"
#include "Symbol.h"

#include <string.h> // strcmp

typedef struct {
    CTRDLHandle* handle;
    CTRDLElf* elf;
    CTRDLResolverFn resolver;
    void* resolverUserData;
} RelContext;

typedef struct {
  uintptr_t offset;
  uintptr_t symbol;
  uint32_t addend;
  uint8_t type;
  bool isWeak;
} RelEntry;

// Relocations are processed in load order.
static u32 ctrdl_resolveSymbol(const RelContext* ctx, Elf32_Word index, bool* isWeak) {
    if (index == STN_UNDEF) {
        *isWeak = false;
        return 0;
    }

    u32 symBase = 0;
    const Elf32_Sym* symEntry = &ctx->elf->symEntries[index];
    const char* name = &ctx->elf->stringTable[symEntry->st_name];
    *isWeak = ELF32_ST_BIND(symEntry->st_info) == STB_WEAK;

    // If we were given a resolver, use it first.
    if (ctx->resolver) {
        u32 addr = (u32)ctx->resolver(name, ctx->resolverUserData);
        if (addr)
            return addr;
    }

    // Look into program symbols.
    u32 addr = (u32)ctrdlProgramResolver(name);
    if (addr)
        return addr;

    // Look into global objects.
    const Elf32_Sym* sym = NULL;
    ctrdl_acquireHandleMtx();

    for (size_t i = 0; i < ctrdl_unsafeNumHandles(); ++i) {
        CTRDLHandle* h = ctrdl_unsafeGetHandleByIndex(i);
        if (h->flags & RTLD_GLOBAL) {
            sym = ctrdl_symNameLookupSingle(h, name);
            if (sym) {
                symBase = h->base;
                break;
            }
        }
    }

    ctrdl_releaseHandleMtx();

    if (!sym) {
        // Look into ourselves.
        if (ctx->elf->numSymChains) {
            const Elf32_Word hash = ctrdl_getELFSymNameHash(name);
            size_t chainIndex = ctx->elf->symBuckets[hash % ctx->elf->numSymBuckets];

            while (chainIndex != STN_UNDEF) {
                const Elf32_Sym* candidate = &ctx->elf->symEntries[chainIndex];
                if (candidate != symEntry && !strcmp(&ctx->elf->stringTable[candidate->st_name], name)) {
                    sym = candidate;
                    symBase = ctx->handle->base;
                    break;
                }

                chainIndex = ctx->elf->symChains[chainIndex];
            }
        }
    }

    if (!sym) {
        // Look into dependencies.
        sym = ctrdl_symNameLookupLoadOrder(ctx->handle, name, &symBase);
    }

    return sym ? (symBase + sym->st_value) : 0;
}

static bool ctrdl_handleSingleReloc(RelContext* ctx, RelEntry* entry) {
    u32* dst = (u32*)entry->offset;
    switch (entry->type) {
        case R_ARM_RELATIVE:
            if (entry->addend) {
                *dst = ctx->handle->base + entry->addend;
            } else {
                *dst += ctx->handle->base;
            }
            return true;
        case R_ARM_ABS32:
        case R_ARM_GLOB_DAT:
        case R_ARM_JUMP_SLOT:
            if (entry->symbol) {
                *dst = entry->symbol + entry->addend;
                return true;
            } else if (entry->isWeak) {
                return true;
            }
            break;
    }

    return false;
}

static bool ctrdl_handleRel(RelContext* ctx) {
    const Elf32_Rel* relArray = ctx->elf->relArray;

    if (relArray) {
        const size_t size = ctx->elf->relArraySize;
        for (size_t i = 0; i < size; ++i) {
            RelEntry entry;
            const Elf32_Rel* rel = &relArray[i];

            entry.offset = ctx->handle->base + rel->r_offset;
            entry.symbol = ctrdl_resolveSymbol(ctx, ELF32_R_SYM(rel->r_info), &entry.isWeak);
            entry.addend = 0;
            entry.type = ELF32_R_TYPE(rel->r_info);

            if (!ctrdl_handleSingleReloc(ctx, &entry))
                return false;
        }
    }

    return true;
}

static bool ctrdl_handleRela(RelContext* ctx) {
    const Elf32_Rela* relaArray = ctx->elf->relaArray;

    if (relaArray) {
        const size_t size = ctx->elf->relaArraySize;

        for (size_t i = 0; i < size; ++i) {
            RelEntry entry;
            const Elf32_Rela* rela = &relaArray[i];

            entry.offset = ctx->handle->base + rela->r_offset;
            entry.symbol = ctrdl_resolveSymbol(ctx, ELF32_R_SYM(rela->r_info), &entry.isWeak);
            entry.addend = rela->r_addend;
            entry.type = ELF32_R_TYPE(rela->r_info);

            if (!ctrdl_handleSingleReloc(ctx, &entry))
                return false;
        }
    }

    return true;
}

bool ctrdl_handleRelocs(CTRDLHandle* handle, CTRDLElf* elf, CTRDLResolverFn resolver, void* resolverUserData) {
    RelContext ctx;
    ctx.handle = handle;
    ctx.elf = elf;
    ctx.resolver = resolver;
    ctx.resolverUserData = resolverUserData;
    return ctrdl_handleRel(&ctx) && ctrdl_handleRela(&ctx);
}