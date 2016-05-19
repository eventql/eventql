project(EventQL)
cmake_minimum_required(VERSION 2.8.0)

include(CheckIncludeFile)
include(CheckCXXSourceCompiles)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/") 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(EVQL_BUILD_ID $ENV{EVQL_BUILD_ID})
if(NOT EVQL_BUILD_ID)
  set(EVQL_BUILD_ID "devel")
endif()

option(ENABLE_TESTS "Build unit tests [default: ON]" ON)

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
ExternalProject_Add(spidermonkey_build
    URL ${PROJECT_SOURCE_DIR}/deps/3rdparty/mozjs
    SOURCE_DIR ${PROJECT_BINARY_DIR}/deps/3rdparty/mozjs
    BINARY_DIR ${PROJECT_BINARY_DIR}/deps/3rdparty/mozjs
    CONFIGURE_COMMAND "true"
    BUILD_COMMAND ./build.sh "${CMAKE_C_COMPILER}" "${CMAKE_CXX_COMPILER}" "${CMAKE_RANLIB}" "${CMAKE_AR}" "${PROJECT_SOURCE_DIR}/deps/3rdparty/mozjs"
    INSTALL_COMMAND "true"
    LOG_BUILD 1)

add_library(spidermonkey STATIC IMPORTED)
set_target_properties(spidermonkey PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/deps/3rdparty/mozjs/dist/lib/libjs_static.a)
add_dependencies(spidermonkey spidermonkey_build)

ExternalProject_Add(protobuf_build
    URL ${PROJECT_SOURCE_DIR}/deps/3rdparty/protobuf
    SOURCE_DIR ${PROJECT_BINARY_DIR}/deps/3rdparty/protobuf
    BINARY_DIR ${PROJECT_BINARY_DIR}/deps/3rdparty/protobuf
    BUILD_COMMAND sh -c "make CC=${CMAKE_C_COMPILER} AR=${CMAKE_AR} RANLIB=${CMAKE_RANLIB} CFLAGS='${CMAKE_C_FLAGS}' LDFLAGS='${CMAKE_EXE_LINKER_FLAGS}'"
    CONFIGURE_COMMAND sh -c "CC=${CMAKE_C_COMPILER} AR=${CMAKE_AR} RANLIB=${CMAKE_RANLIB} CFLAGS='${CMAKE_C_FLAGS}' LDFLAGS='${CMAKE_EXE_LINKER_FLAGS}' ./configure"
    INSTALL_COMMAND "true"
    LOG_BUILD 1
    LOG_CONFIGURE 1)

SET(PROTOBUF_PROTOC_EXECUTABLE ${PROJECT_BINARY_DIR}/deps/3rdparty/protobuf/src/protoc CACHE INTERNAL "The protocol buffers compiler")
set(PROTOC_ARGS --proto_path ${CMAKE_SOURCE_DIR}/src/eventql/core)
include_directories(${PROJECT_BINARY_DIR}/deps/3rdparty/protobuf/src)

add_library(protobuf STATIC IMPORTED)
add_dependencies(protobuf protobuf_build)
set_target_properties(protobuf PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/deps/3rdparty/protobuf/src/.libs/libprotobuf.a)

add_library(linenoise STATIC deps/3rdparty/linenoise/linenoise.c)

set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${CMAKE_BINARY_DIR}/deps/3rdparty/mozjs/dist/lib)
set(CMAKE_CXX_FLAGS "-std=c++0x -ftemplate-depth=500 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wdelete-non-virtual-dtor -Wno-predefined-identifier-outside-function -O3 -g ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS "-std=c11 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -pedantic -O3 -g ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -lpthread")

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
add_subdirectory(src/eventql/io/cstable)
add_subdirectory(src/eventql/io/sstable)
add_subdirectory(src/eventql/util/charts)
add_subdirectory(src/eventql/sql)
add_subdirectory(src/eventql)
add_subdirectory(src/eventql/master)
