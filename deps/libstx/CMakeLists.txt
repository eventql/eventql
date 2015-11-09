cmake_minimum_required(VERSION 2.8.8)
project(libstx)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include_directories(src/)
include_directories(${CMAKE_BINARY_DIR}/src)
include(src/stx/cmake/libstx.cmake)
