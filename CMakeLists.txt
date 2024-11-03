cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
include(CMake/CPM.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(dl)

CPMAddPackage("gh:kynex7510/CTRL#90a1878")

file(GLOB DL_SOURCES Source/*.c)
add_library(dl STATIC ${DL_SOURCES})
target_include_directories(dl PUBLIC Include)
target_compile_options(dl PRIVATE -O2 -Wall -Wno-switch)
target_link_libraries(dl CTRL)

install(TARGETS dl)
install(FILES Include/dlfcn.h DESTINATION include)

add_subdirectory(Tests)