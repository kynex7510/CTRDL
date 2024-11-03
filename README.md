# CTRDL

This library provides an implementation of common `dl*` APIs along with custom additions, which can be used for loading and executing ELF files on the Nintendo 3DS.

## How-to

Download a [prebuilt](https://github.com/kynex7510/CTRDL/releases) version, use as a CMake dependency, or build manually:

```
cmake -B Build -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/3DS.cmake" -DCMAKE_BUILD_TYPE=Release
cmake --build Build --config Release
cmake --install Build --prefix Build/Release
```

## Limitations

- `RTLD_LAZY`, `RTLD_DEEPBIND`, and `RTLD_NODELETE` are not supported.
- `NULL` pseudo path for main process is not supported.