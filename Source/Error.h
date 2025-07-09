/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _CTRDL_ERROR_H
#define _CTRDL_ERROR_H

#include <dlfcn.h>

typedef enum {
    Err_OK,
    Err_InvalidParam,
    Err_ReadFailed,
    Err_NoMemory,
    Err_HandleLimit,
    Err_NotFound,
    Err_InvalidObject,
    Err_InvalidBit,
    Err_NotSO,
    Err_InvalidArch,
    Err_MapFailed,
    Err_RelocFailed,
    Err_DepsLimit,
    Err_DepFailed,
    Err_FreeFailed,
} CTRDLError;

CTRDLError ctrdl_getLastError(void);
void ctrdl_setLastError(CTRDLError error);
void ctrdl_clearLastError(void);
const char* ctrdl_getErrorAsString(CTRDLError error);

#endif /* _CTRDL_ERROR_H */