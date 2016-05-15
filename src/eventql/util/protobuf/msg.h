/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#ifndef _STX_MSG_MSG_H
#define _STX_MSG_MSG_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/buffer.h>
#include "eventql/util/exception.h"
#include <eventql/util/3rdparty/google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <eventql/util/3rdparty/google/protobuf/text_format.h>

namespace util {
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
} // namespace util
#endif
