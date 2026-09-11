/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _CTRDL_ERROR_H
#define _CTRDL_ERROR_H

#include <dlfcn.h>

void ctrdl_setLastError(const char* fmt, ...);
const char* ctrdl_getLastError(void);
void ctrdl_clearLastError(void);

#endif /* _CTRDL_ERROR_H */