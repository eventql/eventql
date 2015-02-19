cmake_minimum_required(VERSION 2.8)
project(xzero-libraries)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_subdirectory(xzero-base)
add_subdirectory(xzero-http)
add_subdirectory(xzero-flow)

add_subdirectory(examples)
add_subdirectory(flow-tool)
