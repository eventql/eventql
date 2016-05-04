cmake_minimum_required(VERSION 2.8.7)
project(libcstable)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# INCLUDES
set(MODULE_INCLUDE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(${MODULE_INCLUDE})
set(CSTABLE_INCLUDE ${MODULE_INCLUDE} PARENT_SCOPE)

# DEPENDENCIES
find_package(Threads)

include(deps/libstx/src/stx/cmake/CFlags.cmake)

if(NOT HAVE_STX)
  set(STX_PATH "deps/libstx" CACHE STRING "libstx directory")
  message(STATUS "using libstx at: ${STX_PATH}")
  add_subdirectory(${STX_PATH} libstx)
  include_directories(${STX_INCLUDE})
  set(HAVE_STX true)
endif()

add_subdirectory(src/cstable)
