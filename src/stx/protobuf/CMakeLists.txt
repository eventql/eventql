# This file is part of the "FnordMetric" project
#   Copyright (c) 2015 Paul Asmuth
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/")
find_package(Protobuf)

set(PROTOC_ARGS --proto_path ${CMAKE_CURRENT_SOURCE_DIR}/ PARENT_SCOPE)
option(ENABLE_TESTS "Build unit tests [default: ON]" ON)

add_library(stx-protobuf OBJECT
    MessageDecoder.cc
    MessageEncoder.cc
    MessagePrinter.cc
    MessageObject.cc
    MessageSchema.cc
    JSONEncoder.cc)

add_dependencies(stx-protobuf stx-base)

set(FNORD_PROTOBUF_LIBS ${PROTOBUF_LIBRARIES} PARENT_SCOPE)

include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
