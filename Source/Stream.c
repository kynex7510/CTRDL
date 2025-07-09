/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Stream.h"

#include <string.h>

static bool ctrdl_fileSeekImpl(void* stream, size_t offset) {
    return !fseek((FILE*)((CTRDLStream*)stream)->handle, offset, SEEK_SET);
}

static bool ctrdl_fileReadImpl(void* s, void* out, size_t size) {
    CTRDLStream* stream = (CTRDLStream*)s;
    FILE* f = (FILE*)stream->handle;
    size_t dataRead = 0;

    while (dataRead < size) {
        const size_t toRead = size - dataRead;
        size_t ret = fread((u8*)(out) + dataRead, 1, toRead, (FILE*)stream->handle);
        if (ret != toRead) {
            if (feof(f) || ferror(f))
                return false;
        }

        dataRead += ret;
    }

    return true;
}

static bool ctrdl_memSeekImpl(void* s, size_t offset) {
    CTRDLStream* stream = (CTRDLStream*)s;
    if (offset <= stream->size) {
        stream->offset = offset;
        return true;
    }

    return false;
}

static bool ctrdl_memReadImpl(void* s, void* out, size_t size) {
    CTRDLStream* stream = (CTRDLStream*)s;
    if (size <= (stream->size - stream->offset)) {
        memcpy(out, (void*)((u8*)(stream->handle) + stream->offset), size);
        stream->offset += size;
        return true;
    }

    return false;
}

void ctrdl_makeFileStream(CTRDLStream* stream, FILE* f) {
    stream->handle = (void*)f;
    stream->seek = ctrdl_fileSeekImpl;
    stream->read = ctrdl_fileReadImpl;
}

void ctrdl_makeMemStream(CTRDLStream* stream, const void* buffer, size_t size) {
    stream->handle = (void*)buffer;
    stream->seek = ctrdl_memSeekImpl;
    stream->read = ctrdl_memReadImpl;
    stream->size = 0;
    stream->offset = 0;
}