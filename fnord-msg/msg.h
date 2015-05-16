/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_MSG_MSG_H
#define _FNORD_MSG_MSG_H
#include <fnord-base/stdtypes.h>

namespace fnord {
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
  target->ParseFromArray(data, size);
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

} // namespace msg
} // namespace fnord
#endif
