cmake_minimum_required(VERSION 2.8.8)
include(FindPkgConfig)
include(CheckIncludeFile)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/") 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
project(cmapp)

option(ENABLE_TESTS "Build unit tests [default: ON]" ON)

include(3rdparty/libstx/cmake/libfnord.cmake)

add_subdirectory(3rdparty/libcstable)
include_directories(${CSTABLE_INCLUDE})

add_subdirectory(3rdparty/libsstable)
include_directories(${SSTABLE_INCLUDE})

add_subdirectory(3rdparty/libdproc)
include_directories(${DPROC_INCLUDE})

add_subdirectory(3rdparty/brokerd)
include_directories(${BROKERD_INCLUDE})

add_subdirectory(3rdparty/chartsql)
include_directories(${CHARTSQL_INCLUDE})

add_subdirectory(3rdparty/tsdb)
include_directories(${TSDB_INCLUDE})

add_subdirectory(3rdparty/fnord-fts)

add_subdirectory(src/customerdb)
add_subdirectory(src/analytics)
add_subdirectory(src/logjoin)
add_subdirectory(src/inventory)
