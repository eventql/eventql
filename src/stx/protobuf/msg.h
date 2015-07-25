/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_MSG_MSG_H
#define _STX_MSG_MSG_H
#include <stx/stdtypes.h>
#include <stx/buffer.h>
#include "stx/exception.h"
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>

namespace stx {
namespace msg {

template <typename ProtoType>
ProtoType decode(const Buffer& buffer);

template <typename ProtoType>
void decode(const Buffer& buffer, ProtoType* target);

template <typename ProtoType>
ProtoType decode(const BufferRef& buffer);

template <typename ProtoType>
void decode(const BufferRef& buffer, ProtoType* target);

template <typename ProtoType>
ProtoType decode(const String& buffer);

template <typename ProtoType>
ProtoType decode(const void* data, size_t size);

template <typename ProtoType>
void decode(const void* data, size_t size, ProtoType* target);

template <typename ProtoType>
BufferRef encode(const ProtoType& proto);

template <typename ProtoType>
void encode(const ProtoType& proto, Buffer* target);

template <typename ProtoType>
void encode(const ProtoType& proto, BufferRef target);

template <typename ProtoType>
ProtoType parseText(const Buffer& buffer);

template <typename ProtoType>
void parseText(const Buffer& buffer, ProtoType* target);

template <typename ProtoType>
ProtoType parseText(const BufferRef& buffer);

template <typename ProtoType>
void parseText(const BufferRef& buffer, ProtoType* target);

template <typename ProtoType>
ProtoType parseText(const String& buffer);

template <typename ProtoType>
ProtoType parseText(const void* data, size_t size);

template <typename ProtoType>
void parseText(const void* data, size_t size, ProtoType* target);

// impl...

template <typename ProtoType>
ProtoType decode(const Buffer& buffer) {
  ProtoType proto;
  decode<ProtoType>(buffer.data(), buffer.size(), &proto);
  return proto;
}

template <typename ProtoType>
void decode(const Buffer& buffer, ProtoType* target) {
  decode<ProtoType>(buffer.data(), buffer.size(), target);
}

template <typename ProtoType>
ProtoType decode(const BufferRef& buffer) {
  ProtoType proto;
  decode<ProtoType>(buffer->data(), buffer->size(), &proto);
  return proto;
}

template <typename ProtoType>
void decode(const BufferRef& buffer, ProtoType* target) {
  decode<ProtoType>(buffer->data(), buffer->size(), target);
}

template <typename ProtoType>
ProtoType decode(const String& buffer) {
  ProtoType proto;
  decode<ProtoType>(buffer.data(), buffer.size(), &proto);
  return proto;
}

template <typename ProtoType>
ProtoType decode(const void* data, size_t size) {
  ProtoType proto;
  decode<ProtoType>(data, size, &proto);
  return proto;
}

template <typename ProtoType>
void decode(const void* data, size_t size, ProtoType* target) {
  if (!target->ParseFromArray(data, size)) {
    RAISE(kRuntimeError, "invalid protobuf message");
  }
}

template <typename ProtoType>
BufferRef encode(const ProtoType& proto) {
  BufferRef buf(new Buffer());
  encode<ProtoType>(proto, buf);
  return buf;
}

template <typename ProtoType>
void encode(const ProtoType& proto, BufferRef target) {
  encode<ProtoType>(proto, target.get());
}

template <typename ProtoType>
void encode(const ProtoType& proto, Buffer* target) {
  auto tmp = proto.SerializeAsString();
  target->append(tmp.data(), tmp.size());
}

template <typename ProtoType>
ProtoType parseText(const Buffer& buffer) {
  ProtoType proto;
  parseText<ProtoType>(buffer.data(), buffer.size(), &proto);
  return proto;
}

template <typename ProtoType>
void parseText(const Buffer& buffer, ProtoType* target) {
  parseText<ProtoType>(buffer.data(), buffer.size(), target);
}

template <typename ProtoType>
ProtoType parseText(const BufferRef& buffer) {
  ProtoType proto;
  parseText<ProtoType>(buffer->data(), buffer->size(), &proto);
  return proto;
}

template <typename ProtoType>
void parseText(const BufferRef& buffer, ProtoType* target) {
  parseText<ProtoType>(buffer->data(), buffer->size(), target);
}

template <typename ProtoType>
ProtoType parseText(const String& buffer) {
  ProtoType proto;
  parseText<ProtoType>(buffer.data(), buffer.size(), &proto);
  return proto;
}

template <typename ProtoType>
ProtoType parseText(const void* data, size_t size) {
  ProtoType proto;
  parseText<ProtoType>(data, size, &proto);
  return proto;
}

template <typename ProtoType>
void parseText(const void* data, size_t size, ProtoType* target) {
  google::protobuf::io::ArrayInputStream is(data, size);

  if (!google::protobuf::TextFormat::Parse(&is, target)) {
    RAISE(kRuntimeError, "invalid protobuf message");
  }
}

} // namespace msg
} // namespace stx
#endif
