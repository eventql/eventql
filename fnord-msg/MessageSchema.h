/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_MSG_MESSAGESCHEMA_H
#define _FNORD_MSG_MESSAGESCHEMA_H
#include <fnord-base/stdtypes.h>

namespace fnord {
namespace msg {

enum class FieldType : uint8_t {
  OBJECT = 0,
  BOOLEAN = 1,
  UINT32 = 2,
  STRING = 3
};

struct MessageSchemaField {

  MessageSchemaField(
    uint32_t _id,
    String _name,
    FieldType _type,
    uint64_t _type_size,
    bool _repeated,
    bool _optional) :
    id(_id),
    name(_name),
    type(_type),
    type_size(_type_size),
    repeated(_repeated),
    optional(_optional) {}

  uint32_t id;
  String name;
  FieldType type;
  uint64_t type_size;
  bool repeated;
  bool optional;
  Vector<MessageSchemaField> fields;
};

struct MessageSchema {
  String name;
  Vector<MessageSchemaField> fields;

  String toString() const;
};

} // namespace msg
} // namespace fnord

#endif
