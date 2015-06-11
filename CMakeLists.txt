cmake_minimum_required(VERSION 2.8.8)
include(FindPkgConfig)
include(CheckIncludeFile)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/") 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
project(cmapp)

option(ENABLE_TESTS "Build unit tests [default: ON]" ON)

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_BINARY_DIR}/3rdparty/libfnord/)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
include_directories(./)
include_directories(./src)
include_directories(3rdparty/libfnord/)
include_directories(3rdparty/libfnord/3rdparty/)

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

find_package(Threads)

set(NO_BUNDLED_LIBFNORD true)


add_subdirectory(3rdparty/libfnord/fnord-base)
add_subdirectory(3rdparty/libfnord/fnord-msg)
add_subdirectory(3rdparty/libfnord/fnord-fts)
include_directories(${FNORD_FTS_INCLUDE})
add_subdirectory(3rdparty/libfnord/fnord-mdb)
add_subdirectory(3rdparty/libfnord/fnord-rpc)
add_subdirectory(3rdparty/libfnord/fnord-http)
add_subdirectory(3rdparty/libfnord/fnord-json)
add_subdirectory(3rdparty/libfnord/fnord-cstable)
add_subdirectory(3rdparty/libfnord/fnord-sstable)
add_subdirectory(3rdparty/libfnord/fnord-chart)

add_subdirectory(3rdparty/sensord)
include_directories(${SENSORD_INCLUDE})

add_subdirectory(3rdparty/tsdb)
include_directories(${TSDB_INCLUDE})

add_subdirectory(3rdparty/brokerd)
include_directories(${BROKERD_INCLUDE})

include_directories(3rdparty/chartsql/src/)

add_subdirectory(3rdparty/libfnord/fnord-metricdb)
add_subdirectory(3rdparty/libfnord/fnord-dht)
add_subdirectory(3rdparty/libfnord/fnord-dproc)
add_subdirectory(3rdparty/libfnord/fnord-afx)
add_subdirectory(src/)

