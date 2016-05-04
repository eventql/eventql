cmake_minimum_required(VERSION 2.8.7)
project(libstx)

option(STX_BUILD_UNIT_TESTS "build stx unit tests" ON)

set(CMAKE_CXX_FLAGS "-std=c++0x -ftemplate-depth=500 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wdelete-non-virtual-dtor ${CMAKE_CXX_FLAGS} -Wno-predefined-identifier-outside-function")
set(CMAKE_C_FLAGS "-std=c11 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -pedantic ${CMAKE_C_FLAGS}")

set(STX_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(STX_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(MODULE_INCLUDE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(${MODULE_INCLUDE})
set(STX_INCLUDE ${MODULE_INCLUDE} PARENT_SCOPE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/stx)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${STX_LDFLAGS}")
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} ${STX_LDFLAGS}")
