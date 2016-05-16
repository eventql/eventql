project(ZenBase)
cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0026 OLD)

include(CheckIncludeFile)
include(CheckCXXSourceCompiles)

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

set(CMAKE_C_FLAGS_RELEASE "-O3 ${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 ${CMAKE_CXX_FLAGS_RELEASE}")

set(PROTOC_ARGS --proto_path ${CMAKE_SOURCE_DIR}/src/eventql/core)

add_library(hunspell OBJECT
    deps/3rdparty/hunspell/affentry.cxx
    deps/3rdparty/hunspell/affixmgr.cxx
    deps/3rdparty/hunspell/csutil.cxx
    deps/3rdparty/hunspell/dictmgr.cxx
    deps/3rdparty/hunspell/filemgr.cxx
    deps/3rdparty/hunspell/hashmgr.cxx
    deps/3rdparty/hunspell/hunspell.cxx
    deps/3rdparty/hunspell/hunzip.cxx
    deps/3rdparty/hunspell/phonet.cxx
    deps/3rdparty/hunspell/replist.cxx
    deps/3rdparty/hunspell/suggestmgr.cxx
    deps/3rdparty/hunspell/utf_info.cxx
    deps/3rdparty/hunspell/hnjalloc.c
    deps/3rdparty/hunspell/hnjalloc.c
    deps/3rdparty/hunspell/hyphen.c)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/src/eventql/util/mysql")
find_package(MySQL)

if(MYSQL_FOUND)
  SET(STX_ENABLE_MYSQL TRUE)
endif()

include_directories(deps/3rdparty/)
include_directories(src/)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/deps/3rdparty/)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/deps/3rdparty/pcre)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/deps/3rdparty/mozjs/dist/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/deps/3rdparty/mozjs/js/src)

add_subdirectory(deps/3rdparty/pcre)
include_directories(deps/3rdparty/pcre)
set(HAVE_PCRE true)
set(PCRE_LIBRARIES "pcre")

include(ExternalProject)
ExternalProject_Add(mozjs_build
    URL ${PROJECT_SOURCE_DIR}/deps/3rdparty/mozjs
    SOURCE_DIR ${PROJECT_BINARY_DIR}/deps/3rdparty/mozjs
    BINARY_DIR ${PROJECT_BINARY_DIR}/deps/3rdparty/mozjs
    BUILD_COMMAND sh -c "CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} AR=${CMAKE_AR} RANLIB=${CMAKE_RANLIB} CFLAGS=\"${CMAKE_C_FLAGS} -DJS_THREADSAFE\" CXXFLAGS=\"${CMAKE_CXX_FLAGS} -DJS_THREADSAFE\" LDFLAGS=\"${CMAKE_EXE_LINKER_FLAGS}\" make JS_THREADSAFE=1"
    CONFIGURE_COMMAND sh -c "CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} AR=${CMAKE_AR} RANLIB=${CMAKE_RANLIB} CFLAGS=\"${CMAKE_C_FLAGS} -DJS_THREADSAFE\" CXXFLAGS=\"${CMAKE_CXX_FLAGS} -DJS_THREADSAFE\" LDFLAGS=\"${CMAKE_EXE_LINKER_FLAGS}\" ${PROJECT_SOURCE_DIR}/deps/3rdparty/mozjs/js/src/configure --disable-shared-js"
    INSTALL_COMMAND "true")

ExternalProject_Add(editline_build
    URL ${PROJECT_SOURCE_DIR}/deps/3rdparty/editline
    SOURCE_DIR ${PROJECT_BINARY_DIR}/deps/3rdparty/editline
    BINARY_DIR ${PROJECT_BINARY_DIR}/deps/3rdparty/editline
    BUILD_COMMAND sh -c "CC=${CMAKE_C_COMPILER} AR=${CMAKE_AR} RANLIB=${CMAKE_RANLIB} CFLAGS=\"${CMAKE_C_FLAGS}\" LDFLAGS=\"${CMAKE_EXE_LINKER_FLAGS}\" make"
    CONFIGURE_COMMAND sh -c "CC=${CMAKE_C_COMPILER} AR=${CMAKE_AR} RANLIB=${CMAKE_RANLIB} CFLAGS=\"${CMAKE_C_FLAGS}\" LDFLAGS=\"${CMAKE_EXE_LINKER_FLAGS}\" ./configure"
    INSTALL_COMMAND "true")


set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${CMAKE_BINARY_DIR}/deps/3rdparty/mozjs/dist/lib)
set(CMAKE_CXX_FLAGS "-std=c++0x -ftemplate-depth=500 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wdelete-non-virtual-dtor ${CMAKE_CXX_FLAGS} -Wno-predefined-identifier-outside-function")
set(CMAKE_C_FLAGS "-std=c11 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -pedantic ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -lpthread")
set(CMAKE_C_FLAGS_DEBUG "-g ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_DEBUG "-g ${CMAKE_CXX_FLAGS_DEBUG}")
#set(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address ${CMAKE_CXX_FLAGS_DEBUG}")
#set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-fsanitize=address ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")

# OSX FLAGS
if(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ ")

# LINUX FLAGS
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl -static-libstdc++ -static-libgcc")
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  set(BUILD_SHARED_LIBRARIES OFF)
  set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)       # remove -Wl,-Bdynamic
  set(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS)
  set(CMAKE_SHARED_LIBRARY_C_FLAGS)         # remove -fPIC
  set(CMAKE_SHARED_LIBRARY_CXX_FLAGS)
  set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)    # remove -rdynamic
  set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
endif()

# check for codecvt
CHECK_CXX_SOURCE_COMPILES("
  #include <codecvt>
  int main() {
    std::codecvt_utf8_utf16<wchar_t> x;
    return 0;
  }
" HAVE_STD_CODECVT)

if (NOT HAVE_STD_CODECVT)
  message(FATAL_ERROR "ERROR: c++ stdlib does not support <codecvt>")
endif()

add_subdirectory(src/eventql/util)
set(HAVE_STX true)

add_subdirectory(src/eventql/io/cstable)
set(HAVE_CSTABLE true)

add_subdirectory(src/eventql/io/sstable)
set(HAVE_SSTABLE true)

add_subdirectory(deps/libcplot)
include_directories(${CPLOT_INCLUDE})
set(HAVE_CPLOT true)

add_subdirectory(src/eventql/sql)
set(HAVE_CSQL true)

add_subdirectory(src/eventql)
add_subdirectory(src/eventql/master)
