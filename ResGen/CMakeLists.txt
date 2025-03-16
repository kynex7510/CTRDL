cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
include(../CMake/CPM.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(ResGen)

CPMAddPackage("gh:fmtlib/fmt#11.1.4")
CPMAddPackage("gh:jarro2783/cxxopts#v3.2.0")
CPMAddPackage("gh:google/re2#2024-07-02")
CPMAddPackage("gh:nlohmann/json#v3.11.3")

file(GLOB RESGEN_SOURCES Source/*.cpp)
add_executable(ResGen ${RESGEN_SOURCES})
target_link_libraries(ResGen PRIVATE fmt cxxopts re2 nlohmann_json)
install(TARGETS ResGen)