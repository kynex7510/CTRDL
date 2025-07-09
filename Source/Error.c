/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Error.h"

static __thread size_t g_LastError = (size_t)Err_OK;

CTRDLError ctrdl_getLastError(void) {
	const CTRDLError error = (CTRDLError)g_LastError;
	ctrdl_clearLastError();
	return error;
}

void ctrdl_setLastError(CTRDLError error) { g_LastError = (size_t)error; }
void ctrdl_clearLastError(void) { ctrdl_setLastError(Err_OK); }

const char* ctrdl_getErrorAsString(CTRDLError error) {
	switch (error) {
		case Err_InvalidParam:
			return "invalid parameter";
		case Err_ReadFailed:
			return "could not read input file";
		case Err_NoMemory:
			return "no memory";
		case Err_HandleLimit:
			return "hit the handle limit";
		case Err_NotFound:
			return "not found";
		case Err_InvalidObject:
			return "invalid object";
		case Err_InvalidBit:
			return "the object is not 32-bit";
		case Err_NotSO:
			return "the object is not shared";
		case Err_InvalidArch:
			return "invalid architecture";
		case Err_MapFailed:
			return "could not map object";
		case Err_RelocFailed:
			return "relocation failed";
		case Err_DepsLimit:
			return "too many dependencies";
		case Err_DepFailed:
			return "could not load dependency";
		case Err_FreeFailed:
			return "could not unload object";
	};

	return NULL;
}