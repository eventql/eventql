cmake_minimum_required(VERSION 2.8.8)
include(FindPkgConfig)
include(CheckIncludeFile)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/") 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
project(cmapp)

option(ENABLE_TESTS "Build unit tests [default: ON]" ON)

include(lib/libfnord/cmake/libfnord.cmake)

add_subdirectory(lib/libfnord/fnord-fts)

add_subdirectory(lib/libcstable)
include_directories(${CSTABLE_INCLUDE})

add_subdirectory(lib/libsstable)
include_directories(${SSTABLE_INCLUDE})

add_subdirectory(lib/libdproc)
include_directories(${DPROC_INCLUDE})

#add_subdirectory(app/sensord)
#include_directories(${SENSORD_INCLUDE})

add_subdirectory(app/brokerd)
include_directories(${BROKERD_INCLUDE})

add_subdirectory(app/chartsql)
include_directories(${CHARTSQL_INCLUDE})

add_subdirectory(app/tsdb)
include_directories(${TSDB_INCLUDE})

add_subdirectory(app/inventoryd)
include_directories(${INVENTORYD_INCLUDE})

add_subdirectory(app/logjoind)
include_directories(${LOGJOIND_INCLUDE})

add_subdirectory(app/metricd)
include_directories(${METRICD_INCLUDE})

add_subdirectory(app/analyticsd)
include_directories(${ANALYTICSD_INCLUDE})
