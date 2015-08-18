
include(CheckIncludeFile)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/") 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
project(cmapp)

option(ENABLE_TESTS "Build unit tests [default: ON]" ON)

#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
#set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -static")

set(PROTOC_ARGS --proto_path ${CMAKE_SOURCE_DIR}/src/zbase/core)
include(3rdparty/libstx/cmake/libstx.cmake)

add_subdirectory(3rdparty/libcstable)
include_directories(${CSTABLE_INCLUDE})

add_subdirectory(3rdparty/libsstable)
include_directories(${SSTABLE_INCLUDE})

add_subdirectory(3rdparty/libdproc)
include_directories(${DPROC_INCLUDE})

add_subdirectory(3rdparty/brokerd)
include_directories(${BROKERD_INCLUDE})

add_subdirectory(3rdparty/libcplot)
include_directories(${CPLOT_INCLUDE})

add_subdirectory(3rdparty/chartsql)
include_directories(${CHARTSQL_INCLUDE})

add_subdirectory(3rdparty/fnord-fts)
include_directories(3rdparty/fnord-fts)
include_directories(3rdparty/)

add_library(hunspell OBJECT
    3rdparty/hunspell/affentry.cxx
    3rdparty/hunspell/affixmgr.cxx
    3rdparty/hunspell/csutil.cxx
    3rdparty/hunspell/dictmgr.cxx
    3rdparty/hunspell/filemgr.cxx
    3rdparty/hunspell/hashmgr.cxx
    3rdparty/hunspell/hunspell.cxx
    3rdparty/hunspell/hunzip.cxx
    3rdparty/hunspell/phonet.cxx
    3rdparty/hunspell/replist.cxx
    3rdparty/hunspell/suggestmgr.cxx
    3rdparty/hunspell/utf_info.cxx
    3rdparty/hunspell/hnjalloc.c
    3rdparty/hunspell/hnjalloc.c
    3rdparty/hunspell/hyphen.c)

include_directories(src/)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

add_subdirectory(src/zbase)
add_subdirectory(src/zbase/master)
add_subdirectory(src/zbase/docdb)
add_subdirectory(src/logjoin)
