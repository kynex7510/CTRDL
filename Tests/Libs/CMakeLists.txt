cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(dl-test-libs C ASM)

# Shared libs are disabled by default; enable them.
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)

# Prevent linking default libraries.
set(CMAKE_C_STANDARD_LIBRARIES "")
set(CMAKE_CXX_STANDARD_LIBRARIES "")
set(CMAKE_ASM_STANDARD_LIBRARIES "")

# Add "libmath" library.
add_library(math SHARED Math.c)
target_link_libraries(math PRIVATE dl-shared)

# Generate resolver for "libmath".
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libmath-resolver.s
    COMMAND ${RESGEN_PATH}/ResGen $<TARGET_FILE:math> -o ${CMAKE_CURRENT_BINARY_DIR}/libmath-resolver.s
    DEPENDS math
)

# Add "libmath" resolver library (application will link to this).
add_library(math-resolver OBJECT ${CMAKE_CURRENT_BINARY_DIR}/libmath-resolver.s)
target_compile_options(math-resolver PRIVATE -g0)

# Setup "libinterop" like above.
add_library(interop SHARED Interop.c)
target_link_libraries(interop PRIVATE dl-shared)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libinterop-resolver.s
    COMMAND ${RESGEN_PATH}/ResGen $<TARGET_FILE:interop> -o ${CMAKE_CURRENT_BINARY_DIR}/libinterop-resolver.s
    DEPENDS interop
)
add_library(interop-resolver OBJECT ${CMAKE_CURRENT_BINARY_DIR}/libinterop-resolver.s)
target_compile_options(interop-resolver PRIVATE -g0)

install(FILES $<TARGET_FILE:math>  $<TARGET_FILE:interop> TYPE BIN)