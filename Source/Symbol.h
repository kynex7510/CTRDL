#ifndef _CTRDL_SYMBOL_H
#define _CTRDL_SYMBOL_H

#include "Handle.h"

const Elf32_Sym* ctrdl_symNameLookupSingle(CTRDLHandle* handle, const char* name);
const Elf32_Sym* ctrdl_symNameLookupLoadOrder(CTRDLHandle* handle, const char* name, u32* modBase);
const Elf32_Sym* ctrdl_symNameLookupDepOrder(CTRDLHandle* handle, const char* name);
const Elf32_Sym* ctrdl_symValueLookupSingle(CTRDLHandle* handle, Elf32_Word value);

#endif /* _CTRDL_SYMBOL_H */