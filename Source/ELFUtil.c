#include "ELFUtil.h"

#include <stdlib.h>
#include <string.h>

Elf32_Word ctrdl_getELFSymNameHash(const char* name) {
    Elf32_Word h = 0;
    Elf32_Word g = 0;

    while (*name) {
        h = ((h << 4) + (u8)*name);
        g = h & 0xF0000000;

        if (g)
            h ^= g >> 24;

        h &= ~g;
        ++name;
    }

    return h;
}

bool ctrdl_parseELF(CTRDLStream* stream, CTRDLElf* out) {
    memset(out, 0, sizeof(CTRDLElf));

    // Read header.
    if (!stream->seek(stream, 0)) {
        ctrdl_setLastError(Err_ReadFailed);
        return false;
    }

    if (!stream->read(stream, &out->header, sizeof(Elf32_Ehdr))) {
        ctrdl_setLastError(Err_ReadFailed);
        return false;
    }

    if (memcmp(out->header.e_ident, ELFMAG, SELFMAG)) {
        ctrdl_setLastError(Err_InvalidObject);
        return false;
    }

    if (out->header.e_ident[EI_CLASS] != ELFCLASS32) {
        ctrdl_setLastError(Err_InvalidBit);
        return false;
    }

    if (out->header.e_ident[EI_DATA] != ELFDATA2LSB) {
        ctrdl_setLastError(Err_InvalidObject);
        return false;
    }

    if (out->header.e_type != ET_DYN) {
        ctrdl_setLastError(Err_NotSO);
        return false;
    }

    if (out->header.e_machine != EM_ARM) {
        ctrdl_setLastError(Err_InvalidArch);
        return false;
    }

    // Read program headers.
    if (!stream->seek(stream, out->header.e_phoff)) {
        ctrdl_setLastError(Err_ReadFailed);
        return false;
    }

    out->segments = malloc(out->header.e_phnum * sizeof(Elf32_Phdr));
    if (!out->segments) {
        ctrdl_setLastError(Err_NoMemory);
        return false;
    }

    for (size_t i = 0; i < out->header.e_phnum; ++i) {
        if (!stream->read(stream, &out->segments[i], sizeof(Elf32_Phdr))) {
            ctrdl_setLastError(Err_ReadFailed);
            ctrdl_freeELF(out);
            return false;
        }
    }

    // Read dyn entries.
    Elf32_Phdr dyn;
    if (!ctrdl_getELFSegmentByType(out, PT_DYNAMIC, &dyn)) {
        ctrdl_setLastError(Err_InvalidObject);
        ctrdl_freeELF(out);
        return false;
    }

    if (!stream->seek(stream, dyn.p_offset)) {
        ctrdl_setLastError(Err_InvalidObject);
        ctrdl_freeELF(out);
        return false;
    }

    out->dynEntries = malloc(dyn.p_filesz);
    if (!out->dynEntries) {
        ctrdl_setLastError(Err_NoMemory);
        ctrdl_freeELF(out);
        return false;
    }

    if (!stream->read(stream, out->dynEntries, dyn.p_filesz)) {
        ctrdl_setLastError(Err_InvalidObject);
        ctrdl_freeELF(out);
        return false;
    }

    // Read sym hash table.
    Elf32_Dyn hash;
    if (!ctrdl_getELFDynEntryWithTag(out, DT_HASH, &hash)) {
        ctrdl_setLastError(Err_InvalidObject);
        ctrdl_freeELF(out);
        return false;
    }

    if (!stream->seek(stream, hash.d_un.d_ptr)) {
        ctrdl_setLastError(Err_ReadFailed);
        ctrdl_freeELF(out);
        return false;
    }

    if (!stream->read(stream, &out->numSymBuckets, sizeof(Elf32_Word))) {
        ctrdl_setLastError(Err_ReadFailed);
        ctrdl_freeELF(out);
        return false;
    }

    if (!stream->read(stream, &out->numSymChains, sizeof(Elf32_Word))) {
        ctrdl_setLastError(Err_ReadFailed);
        ctrdl_freeELF(out);
        return false;
    }

    out->symBuckets = malloc(out->numSymBuckets * sizeof(Elf32_Word));
    if (!out->symBuckets) {
        ctrdl_setLastError(Err_NoMemory);
        ctrdl_freeELF(out);
        return false;
    }

    out->symChains = malloc(out->numSymChains * sizeof(Elf32_Word));
    if (!out->symChains) {
        ctrdl_setLastError(Err_NoMemory);
        ctrdl_freeELF(out);
        return false;
    }

    if (!stream->read(stream, out->symBuckets, out->numSymBuckets * sizeof(Elf32_Word))) {
        ctrdl_setLastError(Err_ReadFailed);
        ctrdl_freeELF(out);
        return false;
    }

    if (!stream->read(stream, out->symChains, out->numSymChains * sizeof(Elf32_Word))) {
        ctrdl_setLastError(Err_ReadFailed);
        ctrdl_freeELF(out);
        return false;
    }

    // Read sym entries.
    Elf32_Dyn symtab;
    if (!ctrdl_getELFDynEntryWithTag(out, DT_SYMTAB, &symtab)) {
        ctrdl_setLastError(Err_InvalidObject);
        ctrdl_freeELF(out);
        return false;
    }

    if (!stream->seek(stream, symtab.d_un.d_ptr)) {
        ctrdl_setLastError(Err_ReadFailed);
        ctrdl_freeELF(out);
        return false;
    }

    out->symEntries = malloc(out->numSymChains * sizeof(Elf32_Sym));
    if (!out->symEntries) {
        ctrdl_setLastError(Err_NoMemory);
        ctrdl_freeELF(out);
        return false;
    }

    if (!stream->read(stream, out->symEntries, out->numSymChains * sizeof(Elf32_Sym))) {
        ctrdl_setLastError(Err_ReadFailed);
        ctrdl_freeELF(out);
        return false;
    }

    // Read string table.
    Elf32_Dyn strtab;
    if (!ctrdl_getELFDynEntryWithTag(out, DT_STRTAB, &strtab)) {
        ctrdl_setLastError(Err_InvalidObject);
        ctrdl_freeELF(out);
        return false;
    }

    if (!stream->seek(stream, strtab.d_un.d_ptr)) {
        ctrdl_setLastError(Err_ReadFailed);
        ctrdl_freeELF(out);
        return false;
    }

    Elf32_Dyn strsz;
    if (!ctrdl_getELFDynEntryWithTag(out, DT_STRSZ, &strsz)) {
        ctrdl_setLastError(Err_InvalidObject);
        ctrdl_freeELF(out);
        return false;
    }

    out->stringTable = malloc(strsz.d_un.d_val);
    if (!out->stringTable) {
        ctrdl_setLastError(Err_NoMemory);
        ctrdl_freeELF(out);
        return false;
    }

    if (!stream->read(stream, out->stringTable, strsz.d_un.d_val)) {
        ctrdl_setLastError(Err_ReadFailed);
        ctrdl_freeELF(out);
        return false;
    }

    // Read reloc info.
    Elf32_Dyn jmpRelArray;
    Elf32_Dyn jmpRelSize;
    Elf32_Dyn jmpRelType;
    const bool hasJmpRelArray = ctrdl_getELFDynEntryWithTag(out, DT_JMPREL, &jmpRelArray);
    const bool hasJmpRelSize = ctrdl_getELFDynEntryWithTag(out, DT_PLTRELSZ, &jmpRelSize);
    const bool hasJmpRelType = ctrdl_getELFDynEntryWithTag(out, DT_PLTREL, &jmpRelType);
    const bool hasJmpRel = hasJmpRelArray && hasJmpRelSize && hasJmpRelType;

    size_t numActuallyJmpRel = 0;
    if (hasJmpRel) {
        switch (jmpRelType.d_un.d_val) {
            case DT_REL:
                numActuallyJmpRel = jmpRelSize.d_un.d_val / sizeof(Elf32_Rel);
                out->relArraySize = numActuallyJmpRel;
                out->relaArraySize = 0;
                break;
            case DT_RELA:
                numActuallyJmpRel = jmpRelSize.d_un.d_val / sizeof(Elf32_Rela);
                out->relArraySize = 0;
                out->relaArraySize = numActuallyJmpRel;
                break;
            default:
                ctrdl_setLastError(Err_InvalidObject);
                ctrdl_freeELF(out);
                return false;
        }
    }

    Elf32_Dyn relArray;
    Elf32_Dyn relSize;
    Elf32_Dyn relEnt;
    const bool hasRelArray = ctrdl_getELFDynEntryWithTag(out, DT_REL, &relArray);
    const bool hasRelSize = ctrdl_getELFDynEntryWithTag(out, DT_RELSZ, &relSize);
    const bool hasRelEnt = ctrdl_getELFDynEntryWithTag(out, DT_RELENT, &relEnt);
    const bool hasRel = hasRelArray && hasRelSize && hasRelEnt;

    size_t numActuallyRel = 0;
    if (hasRel) {
        numActuallyRel = (relSize.d_un.d_val / relEnt.d_un.d_val);
        out->relArraySize += numActuallyRel;
    }

    if (out->relArraySize) {
        out->relArray = malloc(out->relArraySize * sizeof(Elf32_Rel));
        if (!out->relArray) {
            ctrdl_setLastError(Err_NoMemory);
            ctrdl_freeELF(out);
            return false;
        }

        if (!stream->seek(stream, relArray.d_un.d_ptr)) {
            ctrdl_setLastError(Err_ReadFailed);
            ctrdl_freeELF(out);
            return false;
        }

        if (!stream->read(stream, out->relArray, numActuallyRel * sizeof(Elf32_Rel))) {
            ctrdl_setLastError(Err_ReadFailed);
            ctrdl_freeELF(out);
            return false;
        }
    }

    Elf32_Dyn relaArray;
    Elf32_Dyn relaSize;
    Elf32_Dyn relaEnt;
    const bool hasRelaArray = ctrdl_getELFDynEntryWithTag(out, DT_RELA, &relaArray);
    const bool hasRelaSize = ctrdl_getELFDynEntryWithTag(out, DT_RELASZ, &relaSize);
    const bool hasRelaEnt = ctrdl_getELFDynEntryWithTag(out, DT_RELAENT, &relaEnt);
    const bool hasRela = hasRelaArray && hasRelaSize && hasRelaEnt;

    size_t numActuallyRela = 0;
    if (hasRela) {
        numActuallyRela = (relaSize.d_un.d_val / relaEnt.d_un.d_val);
        out->relaArraySize += numActuallyRela;
    }

    if (out->relaArraySize) {
        out->relaArray = malloc(out->relaArraySize * sizeof(Elf32_Rela));
        if (!out->relaArray) {
            ctrdl_setLastError(Err_NoMemory);
            ctrdl_freeELF(out);
            return false;
        }

        if (!stream->seek(stream, relaArray.d_un.d_ptr)) {
            ctrdl_setLastError(Err_ReadFailed);
            ctrdl_freeELF(out);
            return false;
        }

        if (!stream->read(stream, out->relaArray, numActuallyRela * sizeof(Elf32_Rela))) {
            ctrdl_setLastError(Err_ReadFailed);
            ctrdl_freeELF(out);
            return false;
        }
    }

    if (hasJmpRel) {
        void* dst = NULL;
        size_t toRead = 0;

        if (jmpRelType.d_un.d_val == DT_REL) {
            dst = out->relArray + numActuallyRel;
            toRead = numActuallyJmpRel * sizeof(Elf32_Rel);
        } else {
            dst = out->relaArray + numActuallyRela;
            toRead = numActuallyJmpRel * sizeof(Elf32_Rela);
        }

        if (!stream->seek(stream, jmpRelArray.d_un.d_ptr)) {
            ctrdl_setLastError(Err_ReadFailed);
            ctrdl_freeELF(out);
            return false;
        }

        if (!stream->read(stream, dst, toRead)) {
            ctrdl_setLastError(Err_ReadFailed);
            ctrdl_freeELF(out);
            return false;
        }
    }

    return true;
}

void ctrdl_freeELF(CTRDLElf* elf) {
    free(elf->segments);
    free(elf->dynEntries);
    free(elf->symBuckets);
    free(elf->symChains);
    free(elf->symEntries);
    free(elf->stringTable);
    free(elf->relArray);
    free(elf->relaArray);
}

size_t ctrdl_getELFNumSegmentsByType(CTRDLElf* elf, Elf32_Word type) {
    size_t count = 0;

    for (size_t i = 0; i < elf->header.e_phnum; ++i) {
        Elf32_Phdr* ph = &elf->segments[i];
        if (ph->p_type == type)
            ++count;
    }

    return count;
}

size_t ctrdl_getELFSegmentsByType(CTRDLElf* elf, Elf32_Word type, Elf32_Phdr* out, size_t maxSize) {
    size_t count = 0;

    for (size_t i = 0; i < elf->header.e_phnum; ++i) {
        Elf32_Phdr* ph = &elf->segments[i];
        if (ph->p_type == type) {
            memcpy(&out[count], ph, sizeof(Elf32_Phdr));
            ++count;

            if (count >= maxSize)
                break;
        }
    }

    return count;
}

size_t ctrdl_getELFNumDynEntriesWithTag(CTRDLElf* elf, Elf32_Sword tag) {
    size_t count = 0;
    Elf32_Dyn* entry = elf->dynEntries;

    while (entry->d_tag != DT_NULL) {
        if (entry->d_tag == tag) {
            ++count;
        }

        ++entry;
    }

    return count;
}

size_t ctrdl_getELFDynEntriesWithTag(CTRDLElf* elf, Elf32_Sword tag, Elf32_Dyn* out, size_t maxSize) {
    size_t count = 0;
    Elf32_Dyn* entry = elf->dynEntries;

    while (entry->d_tag != DT_NULL) {
        if (entry->d_tag == tag) {
            memcpy(&out[count], entry, sizeof(Elf32_Dyn));
            ++count;
        }

        ++entry;
    }

    return count;
}