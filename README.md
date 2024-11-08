# CTRDL

Implementation of common `dl*` APIs, along with custom additions, which can be used for loading and executing ELF files on the Nintendo 3DS.

## Build

Download a [prebuilt](https://github.com/kynex7510/CTRDL/releases) version, use as a CMake dependency, or build manually:

```
cmake -B Build -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/3DS.cmake" -DCMAKE_BUILD_TYPE=Release
cmake --build Build --config Release
cmake --install Build --prefix Build/Release
```

## Symbol resolution

Since all homebrew is statically linked by default, there's no way for a program to expose symbols to shared objects. This behaviour can be simulated by redeclaring `ctrdlProgramResolver`, which is called internally whenever a symbol has to be looked up in the program, or its dependencies. By default `ctrdlProgramResolver` returns `NULL`.

Additionally, a custom resolver can be passed to the extensions `ctrdlOpen`, `ctrdlFOpen`, `ctrdlMap`, which will be used at the relocation step, and which always precedes other lookup mechanisms (`dlsym` is not affected).

## Limitations

- `RTLD_LAZY`, `RTLD_DEEPBIND`, and `RTLD_NODELETE` are not supported.