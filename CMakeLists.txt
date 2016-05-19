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
include_directories(deps/3rdparty/protobuf)
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

# PROTOBUF
add_library(protobuf STATIC
    deps/3rdparty/protobuf/google/protobuf/descriptor.cc
    deps/3rdparty/protobuf/google/protobuf/descriptor.pb.cc
    deps/3rdparty/protobuf/google/protobuf/descriptor_database.cc
    deps/3rdparty/protobuf/google/protobuf/dynamic_message.cc
    deps/3rdparty/protobuf/google/protobuf/extension_set.cc
    deps/3rdparty/protobuf/google/protobuf/extension_set_heavy.cc
    deps/3rdparty/protobuf/google/protobuf/generated_message_reflection.cc
    deps/3rdparty/protobuf/google/protobuf/generated_message_util.cc
    deps/3rdparty/protobuf/google/protobuf/io/coded_stream.cc
    deps/3rdparty/protobuf/google/protobuf/io/gzip_stream.cc
    deps/3rdparty/protobuf/google/protobuf/io/printer.cc
    deps/3rdparty/protobuf/google/protobuf/io/tokenizer.cc
    deps/3rdparty/protobuf/google/protobuf/io/zero_copy_stream.cc
    deps/3rdparty/protobuf/google/protobuf/io/zero_copy_stream_impl.cc
    deps/3rdparty/protobuf/google/protobuf/io/zero_copy_stream_impl_lite.cc
    deps/3rdparty/protobuf/google/protobuf/message.cc
    deps/3rdparty/protobuf/google/protobuf/message_lite.cc
    deps/3rdparty/protobuf/google/protobuf/reflection_ops.cc
    deps/3rdparty/protobuf/google/protobuf/repeated_field.cc
    deps/3rdparty/protobuf/google/protobuf/service.cc
    deps/3rdparty/protobuf/google/protobuf/stubs/atomicops_internals_x86_gcc.cc
    deps/3rdparty/protobuf/google/protobuf/stubs/atomicops_internals_x86_msvc.cc
    deps/3rdparty/protobuf/google/protobuf/stubs/common.cc
    deps/3rdparty/protobuf/google/protobuf/stubs/once.cc
    deps/3rdparty/protobuf/google/protobuf/stubs/stringprintf.cc
    deps/3rdparty/protobuf/google/protobuf/stubs/structurally_valid.cc
    deps/3rdparty/protobuf/google/protobuf/stubs/strutil.cc
    deps/3rdparty/protobuf/google/protobuf/stubs/strutil.cc
    deps/3rdparty/protobuf/google/protobuf/stubs/substitute.cc
    deps/3rdparty/protobuf/google/protobuf/text_format.cc
    deps/3rdparty/protobuf/google/protobuf/text_format.cc
    deps/3rdparty/protobuf/google/protobuf/unknown_field_set.cc
    deps/3rdparty/protobuf/google/protobuf/wire_format.cc
    deps/3rdparty/protobuf/google/protobuf/wire_format_lite.cc)


add_executable(protoc
    deps/3rdparty/protobuf/google/protobuf/compiler/command_line_interface.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/main.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/parser.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/subprocess.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/importer.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/code_generator.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/plugin.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/plugin.pb.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/cpp/cpp_enum.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/cpp/cpp_enum_field.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/cpp/cpp_extension.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/cpp/cpp_field.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/cpp/cpp_file.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/cpp/cpp_generator.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/cpp/cpp_helpers.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/cpp/cpp_message.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/cpp/cpp_message_field.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/cpp/cpp_primitive_field.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/cpp/cpp_service.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/cpp/cpp_string_field.cc
    deps/3rdparty/protobuf/google/protobuf/compiler/zip_writer.cc)

target_link_libraries(protoc protobuf z)
get_filename_component(PROTOBUF_PROTOC_EXECUTABLE ${PROJECT_BINARY_DIR}/protoc ABSOLUTE)
set(PROTOC_ARGS --proto_path ${CMAKE_SOURCE_DIR}/src/eventql/core)

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
