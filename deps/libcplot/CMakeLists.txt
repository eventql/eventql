cmake_minimum_required(VERSION 2.8.8)
project(libcplot)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# INCLUDES
set(MODULE_INCLUDE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(${MODULE_INCLUDE})
set(CPLOT_INCLUDE ${MODULE_INCLUDE} PARENT_SCOPE)

# DEPENDENCIES
find_package(Threads)

if(NOT HAVE_STX)
  add_subdirectory(deps/libstx)
  include_directories(${STX_INCLUDE})
  set(HAVE_STX true)
endif()

add_subdirectory(src/cplot)
