cmake_minimum_required(VERSION 2.8.8)
include(FindPkgConfig)
include(CheckIncludeFile)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/") 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
project(cmapp)

option(ENABLE_TESTS "Build unit tests [default: ON]" ON)

set(CMAKE_CXX_FLAGS "-ftemplate-depth=500 ${CMAKE_CXX_FLAGS}")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -g")

if(APPLE)
  set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS} -stdlib=libc++ ")
else()
  set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "-ldl -static-libstdc++ -static-libgcc")
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  set(BUILD_SHARED_LIBRARIES OFF)
  set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)       # remove -Wl,-Bdynamic
  set(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS)
  set(CMAKE_SHARED_LIBRARY_C_FLAGS)         # remove -fPIC
  set(CMAKE_SHARED_LIBRARY_CXX_FLAGS)
  set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)    # remove -rdynamic
  set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
endif()

# strip binaries
#  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")

add_subdirectory(lib/libfnord)
include_directories(${FNORD_INCLUDE})
add_subdirectory(lib/libfnord/fnord-fts)

add_subdirectory(lib/libcstable)
include_directories(${CSTABLE_INCLUDE})

add_subdirectory(lib/libsstable)
include_directories(${SSTABLE_INCLUDE})

add_subdirectory(lib/libdproc)
include_directories(${DPROC_INCLUDE})

add_subdirectory(app/sensord)
include_directories(${SENSORD_INCLUDE})

add_subdirectory(app/brokerd)
include_directories(${BROKERD_INCLUDE})

add_subdirectory(app/tsdb)
include_directories(${TSDB_INCLUDE})

add_subdirectory(app/inventoryd)
include_directories(${INVENTORYD_INCLUDE})

add_subdirectory(app/logjoind)
include_directories(${LOGJOIND_INCLUDE})

add_subdirectory(app/metricd)
include_directories(${METRICD_INCLUDE})

add_subdirectory(app/chartsql)
include_directories(${CHARTSQL_INCLUDE})

add_subdirectory(app/analyticsd)
include_directories(${ANALYTICSD_INCLUDE})
