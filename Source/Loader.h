/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _CTRDL_LOADER_H
#define _CTRDL_LOADER_H

#include "Handle.h"
#include "Stream.h"

CTRDLHandle* ctrdl_loadObject(const char* name, int flags, CTRDLStream* stream, CTRDLResolverFn resolver, void* resolverUserData);
bool ctrdl_unloadObject(CTRDLHandle* handle);

#endif /* _CTRDL_LOADER_H */