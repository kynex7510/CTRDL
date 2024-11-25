cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
include(../CMake/CPM.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(ResGen)

CPMAddPackage("gh:fmtlib/fmt#11.0.2")
CPMAddPackage("gh:jarro2783/cxxopts#v3.2.0")

file(GLOB RESGEN_SOURCES Source/*.cpp)
add_executable(${PROJECT_NAME} ${RESGEN_SOURCES})
target_link_libraries(${PROJECT_NAME} fmt cxxopts)