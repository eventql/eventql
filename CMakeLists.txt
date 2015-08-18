cmake_minimum_required(VERSION 2.8.8)
project(libstx)

include_directories(src/)
include_directories(${CMAKE_BINARY_DIR}/src)
include(src/stx/cmake/libstx.cmake)
