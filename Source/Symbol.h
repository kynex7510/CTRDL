/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _CTRDL_SYMBOL_H
#define _CTRDL_SYMBOL_H

#include "Handle.h"

const Elf32_Sym* ctrdl_symNameLookupSingle(CTRDLHandle* handle, const char* name);
const Elf32_Sym* ctrdl_symNameLookupLoadOrder(CTRDLHandle* handle, const char* name, u32* modBase);
const Elf32_Sym* ctrdl_symNameLookupDepOrder(CTRDLHandle* handle, const char* name);
const Elf32_Sym* ctrdl_symValueLookupSingle(CTRDLHandle* handle, Elf32_Word value);

#endif /* _CTRDL_SYMBOL_H */