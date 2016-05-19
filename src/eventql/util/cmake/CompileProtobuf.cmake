#=============================================================================
# Copyright 2009 Kitware, Inc.
# Copyright 2009 Philip Lowman <philip@yhbt.com>
# Copyright 2008 Esben Mose Hansen, Ange Optimization ApS
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

FIND_PATH(PROTOBUF_INCLUDE_DIR stubs/common.h /usr/include/google/protobuf)
FIND_LIBRARY(PROTOBUF_LIBRARY NAMES protobuf PATHS ${GNUWIN32_DIR}/lib)
FIND_PROGRAM(PROTOBUF_PROTOC_EXECUTABLE protoc)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(protobuf DEFAULT_MSG PROTOBUF_INCLUDE_DIR PROTOBUF_LIBRARY PROTOBUF_PROTOC_EXECUTABLE)

include_directories(${PROTOBUF_INCLUDE_DIR})

# ensure that they are cached
SET(PROTOBUF_INCLUDE_DIR ${PROTOBUF_INCLUDE_DIR} CACHE INTERNAL "The protocol buffers include path")
SET(PROTOBUF_LIBRARY ${PROTOBUF_LIBRARY} CACHE INTERNAL "The libraries needed to use protocol buffers library")
SET(PROTOBUF_PROTOC_EXECUTABLE ${PROTOBUF_PROTOC_EXECUTABLE} CACHE INTERNAL "The protocol buffers compiler")

function(STX_PROTOBUF_GENERATE_CPP SRCS HDRS)
  if(NOT ARGN)
    message(SEND_ERROR "Error: STX_PROTOBUF_GENERATE_CPP() called without any proto files")
    return()
  endif(NOT ARGN)

  set(${SRCS})
  set(${HDRS})
  foreach(FIL ${ARGN})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)

    get_filename_component(ROOT_DIR ${CMAKE_SOURCE_DIR}/src ABSOLUTE)
    string(LENGTH ${ROOT_DIR} ROOT_DIR_LEN)

    get_filename_component(FIL_DIR ${ABS_FIL} PATH)
    string(LENGTH ${FIL_DIR} FIL_DIR_LEN)
    math(EXPR FIL_DIR_REM "${FIL_DIR_LEN} - ${ROOT_DIR_LEN}")

    string(SUBSTRING ${FIL_DIR} ${ROOT_DIR_LEN} ${FIL_DIR_REM} FIL_DIR_PREFIX)
    set(FIL_WEPREFIX "${FIL_DIR_PREFIX}/${FIL_WE}")

    list(APPEND ${SRCS} "${CMAKE_BINARY_DIR}/${FIL_WEPREFIX}.pb.cc")
    list(APPEND ${HDRS} "${CMAKE_BINARY_DIR}/${FIL_WEPREFIX}.pb.h")

    add_custom_command(
      OUTPUT "${CMAKE_BINARY_DIR}/${FIL_WEPREFIX}.pb.cc"
             "${CMAKE_BINARY_DIR}/${FIL_WEPREFIX}.pb.h"
      COMMAND  ${PROTOBUF_PROTOC_EXECUTABLE}
      ARGS --cpp_out ${CMAKE_BINARY_DIR} --proto_path ${CMAKE_SOURCE_DIR}/src/eventql/util/3rdparty --proto_path ${CMAKE_SOURCE_DIR}/src ${ABS_FIL}
      DEPENDS ${ABS_FIL}
      VERBATIM )
  endforeach()

  set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
  set(${SRCS} ${${SRCS}} PARENT_SCOPE)
  set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction()


