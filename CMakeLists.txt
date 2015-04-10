cmake_minimum_required(VERSION 2.8)
project(cortex-libraries)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_subdirectory(cortex-base)
add_subdirectory(cortex-http)
add_subdirectory(cortex-flow)
add_subdirectory(examples)
#add_subdirectory(flow-tool)
