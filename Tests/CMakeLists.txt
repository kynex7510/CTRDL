cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(dl-test)

add_subdirectory(Libs)

add_executable(dl-test-math MathApp.c)
target_link_libraries(dl-test-math PRIVATE dl math-resolver)
ctr_create_3dsx(dl-test-math)

#add_executable(dl-test-interop InteropApp.c)
#target_link_libraries(dl-test-interop PRIVATE dl libinterop-resolver)
#ctr_create_3dsx(dl-test-interop)

install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dl-test-math.3dsx
#    ${CMAKE_CURRENT_BINARY_DIR}/dl-test-interop.3dsx
    TYPE BIN
)