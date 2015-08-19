project(ZenBase)

include(CheckIncludeFile)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/") 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

option(ENABLE_TESTS "Build unit tests [default: ON]" ON)

#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
#set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -static")

set(PROTOC_ARGS --proto_path ${CMAKE_SOURCE_DIR}/src/zbase/core)

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

include_directories(3rdparty/)
include_directories(src/)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

add_subdirectory(3rdparty/pcre)
set(HAVE_PCRE true)

include(src/stx/cmake/libstx.cmake)
add_subdirectory(src/sstable)
add_subdirectory(src/cstable)
add_subdirectory(src/cplot)
add_subdirectory(src/csql)

add_subdirectory(3rdparty/brokerd)
include_directories(${BROKERD_INCLUDE})

add_subdirectory(src/zbase)
add_subdirectory(src/zbase/util)
add_subdirectory(src/zbase/util/mysql)
add_subdirectory(src/zbase/util/mdb)
add_subdirectory(src/zbase/master)
add_subdirectory(src/zbase/docdb)
add_subdirectory(src/zbase/logjoin)
add_subdirectory(src/zbase/dproc)
