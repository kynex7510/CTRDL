cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
include(CMake/CPM.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(dl)

CPMAddPackage("gh:kynex7510/CTRL#c8a24a0")

file(GLOB DL_SOURCES Source/*.c)
add_library(dl STATIC ${DL_SOURCES})
target_include_directories(dl PUBLIC Include)
target_compile_options(dl PRIVATE -Wall -Wno-switch)
target_link_libraries(dl CTRL)
install(TARGETS dl)
install(FILES Include/dlfcn.h DESTINATION include)

# To be used by shared objects for the include.
add_library(dl-shared INTERFACE)
target_include_directories(dl-shared INTERFACE Include)

if(RESGEN_PATH)
    message("-- ResGen path set, tests enabled")
    add_subdirectory(Tests)
endif()