# CTRDL

This library provides an implementation of common `dl*` APIs, which can be used for loading and executing ELF files on the Nintendo 3DS.

## Build

```
mkdir Build
cd Build
$DEVKITPRO/devkitARM/bin/arm-none-eabi-cmake --toolchain "$DEVKITPRO/cmake/3DS.cmake" ..
```

## Limitations

- RTLD_LAZY, RTLD_DEEPBIND, and RTLD_NODELETE are not supported;
- NULL pseudo path for main process is not supported;
- dlsym() is stubbed;
- dladdr() never fills dli_sname nor dli_saddr;
- Only a handful relocations are implemented.