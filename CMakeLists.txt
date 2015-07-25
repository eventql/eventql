<<<<<<< HEAD
cmake_minimum_required(VERSION 2.8.8)
project(libstx)

include(cmake/libstx.cmake)
=======
cmake_minimum_required(VERSION 2.8)
project(cortex-libraries)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_subdirectory(cortex-base)
add_subdirectory(cortex-http)
add_subdirectory(cortex-flow)
add_subdirectory(examples)
#add_subdirectory(flow-tool)
>>>>>>> 6f22a8e6fe0766e834d1643d90792dec4c9fc17c
