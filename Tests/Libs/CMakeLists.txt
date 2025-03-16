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

add_library(math SHARED Math.c)

add_custom_command(
    OUTPUT /home/user/Documents/repos/CTRDL/Build/Tests/Libs/libmath.s # $<TARGET_FILE_DIR:math>/libmath.s
    COMMAND ${RESGEN_PATH}/ResGen -b $<TARGET_FILE:math>
    DEPENDS math
)
add_library(math-resolver OBJECT $<TARGET_FILE_DIR:math>/libmath.s)
target_compile_options(math-resolver PRIVATE -g0)

#add_library(interop SHARED Interop.c)
#add_library(libinterop-resolver INTERFACE libinterop.s)
#target_link_libraries(interop PRIVATE dl-shared)
#add_custom_command(
#    TARGET interop
#    POST_BUILD
#    COMMAND ${RESGEN_PATH}/ResGen -b $<TARGET_FILE:interop>
#    BYPRODUCTS libinterop.s
#)

install(FILES $<TARGET_FILE:math> 
#$<TARGET_FILE:interop>
TYPE BIN)