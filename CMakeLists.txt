project(ZenBase)
cmake_minimum_required(VERSION 2.8)

include(CheckIncludeFile)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/") 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(ZBASE_BUILD_ID $ENV{ZBASE_BUILD_ID})
if(NOT ZBASE_BUILD_ID)
  set(ZBASE_BUILD_ID "devel")
endif()

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

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/src/zbase/util/mysql")
find_package(MySQL)

if(MYSQL_FOUND)
  SET(STX_ENABLE_MYSQL TRUE)
endif()

include_directories(3rdparty/)
include_directories(src/)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/3rdparty/)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/3rdparty/pcre)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/3rdparty/mozjs/dist/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/3rdparty/mozjs/js/src)

add_subdirectory(3rdparty/pcre)
include_directories(3rdparty/pcre)
set(HAVE_PCRE true)
set(PCRE_LIBRARIES "pcre")

include(ExternalProject)
ExternalProject_Add(mozjs_build
    URL ${PROJECT_SOURCE_DIR}/3rdparty/mozjs
    SOURCE_DIR ${PROJECT_BINARY_DIR}/3rdparty/mozjs
    BINARY_DIR ${PROJECT_BINARY_DIR}/3rdparty/mozjs
    BUILD_COMMAND sh -c "CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} AR=${CMAKE_AR} RANLIB=${CMAKE_RANLIB} CFLAGS=\"${CMAKE_C_FLAGS} -DJS_THREADSAFE\" CXXFLAGS=\"${CMAKE_CXX_FLAGS} -DJS_THREADSAFE\" LDFLAGS=\"${CMAKE_EXE_LINKER_FLAGS}\" make JS_THREADSAFE=1"
    CONFIGURE_COMMAND sh -c "CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} AR=${CMAKE_AR} RANLIB=${CMAKE_RANLIB} CFLAGS=\"${CMAKE_C_FLAGS} -DJS_THREADSAFE\" CXXFLAGS=\"${CMAKE_CXX_FLAGS} -DJS_THREADSAFE\" LDFLAGS=\"${CMAKE_EXE_LINKER_FLAGS}\" ../../../../../3rdparty/mozjs/js/src/configure --disable-shared-js"
    INSTALL_COMMAND "true")

set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${CMAKE_BINARY_DIR}/3rdparty/mozjs/dist/lib)

include(deps/libstx/src/stx/cmake/CFlags.cmake)

add_subdirectory(deps/libstx)
include_directories(${STX_INCLUDE})
set(HAVE_STX true)

add_subdirectory(deps/libcstable)
include_directories(${CSTABLE_INCLUDE})

add_subdirectory(src/sstable)
add_subdirectory(src/cplot)
add_subdirectory(src/csql)

add_subdirectory(3rdparty/brokerd)
include_directories(${BROKERD_INCLUDE})

add_subdirectory(src/zbase)
add_subdirectory(src/zbase/master)
add_subdirectory(src/zbase/docdb)
add_subdirectory(src/zbase/logjoin)
add_subdirectory(src/zbase/dproc)
