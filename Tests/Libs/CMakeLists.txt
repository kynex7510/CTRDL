cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(dl-test-libs)

# Shared libs are disabled by default; enable them.
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)

# Prevent linking default libraries.
set(CMAKE_C_STANDARD_LIBRARIES "")
set(CMAKE_CXX_STANDARD_LIBRARIES "")
set(CMAKE_ASM_STANDARD_LIBRARIES "")

add_library(math SHARED Math.c)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libmath.so TYPE BIN)

add_library(interop SHARED Interop.c)
target_link_libraries(interop PRIVATE dl-shared)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libinterop.so TYPE BIN)