# This file is part of the "libstx" project
#   Copyright (c) 2015 Paul Asmuth
#
# libstx is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
set(PROTOC_ARGS --proto_path ${CMAKE_CURRENT_SOURCE_DIR}/ PARENT_SCOPE)

add_library(stx-protobuf OBJECT
    ../../../3rdparty/google/protobuf/descriptor.cc
    ../../../3rdparty/google/protobuf/descriptor.pb.cc
    ../../../3rdparty/google/protobuf/descriptor_database.cc
    ../../../3rdparty/google/protobuf/dynamic_message.cc
    ../../../3rdparty/google/protobuf/extension_set.cc
    ../../../3rdparty/google/protobuf/extension_set_heavy.cc
    ../../../3rdparty/google/protobuf/generated_message_reflection.cc
    ../../../3rdparty/google/protobuf/generated_message_util.cc
    ../../../3rdparty/google/protobuf/message.cc
    ../../../3rdparty/google/protobuf/message_lite.cc
    ../../../3rdparty/google/protobuf/reflection_ops.cc
    ../../../3rdparty/google/protobuf/repeated_field.cc
    ../../../3rdparty/google/protobuf/service.cc
    ../../../3rdparty/google/protobuf/text_format.cc
    ../../../3rdparty/google/protobuf/unknown_field_set.cc
    ../../../3rdparty/google/protobuf/wire_format.cc
    ../../../3rdparty/google/protobuf/wire_format_lite.cc
    MessageDecoder.cc
    MessageEncoder.cc
    MessagePrinter.cc
    MessageObject.cc
    MessageSchema.cc
    DynamicMessage.cc
    JSONEncoder.cc)

add_dependencies(stx-protobuf stx-base)

#set(STX_PROTOBUF_LIBS ${PROTOBUF_LIBRARIES} PARENT_SCOPE)

#include_directories(${PROTOBUF_INCLUDE_DIRS})
#include_directories(${CMAKE_CURRENT_BINARY_DIR})
