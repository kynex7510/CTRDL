cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
include(../CMake/CPM.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(ResGen)

CPMAddPackage("gh:fmtlib/fmt#11.1.4")
CPMAddPackage("gh:jarro2783/cxxopts#v3.2.0")

CPMAddPackage(
    URI "gh:boostorg/regex#boost-1.88.0"
    OPTIONS "BOOST_REGEX_STANDALONE ON"
)

CPMAddPackage("gh:nlohmann/json#v3.11.3")

file(GLOB RESGEN_SOURCES Source/*.cpp)
add_executable(ResGen ${RESGEN_SOURCES})
target_link_libraries(ResGen PRIVATE fmt cxxopts Boost::regex nlohmann_json)
install(TARGETS ResGen)