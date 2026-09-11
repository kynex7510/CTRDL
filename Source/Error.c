/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Error.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static __thread char* g_ErrorBuffer = NULL;
static __thread int g_HasError = 0;

static void lazyInitBuffer(void) {
    if (!g_ErrorBuffer) {
        g_ErrorBuffer = calloc(1, 512);

        if (!g_ErrorBuffer)
            svcBreak(USERBREAK_PANIC);
    }
}

void ctrdl_setLastError(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    lazyInitBuffer();
    g_HasError = 1;

    vsnprintf(g_ErrorBuffer, 512, fmt, args);
}

const char* ctrdl_getLastError(void) {
    if (g_HasError) {
        g_HasError = 0;
        return g_ErrorBuffer;
    }

    return NULL;
}

void ctrdl_clearLastError(void) { g_HasError = 0; }