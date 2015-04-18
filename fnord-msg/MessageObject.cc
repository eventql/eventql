/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-msg/MessageObject.h>
#include <fnord-base/exception.h>

namespace fnord {
namespace msg {

MessageObject::MessageObject(
    uint32_t _id /* = 0 */) :
    id(_id),
    type(FieldType::OBJECT) {
  new (&data_) Vector<MessageObject>();
}

MessageObject::MessageObject(
    uint32_t _id,
    uint32_t value) :
    id(_id),
    type(FieldType::UINT32) {
  new (&data_) uint32_t(value);
}

MessageObject::MessageObject(
    uint32_t _id,
    const String& value) : id(_id),
    type(FieldType::STRING) {
  new (&data_) String(value);
}

MessageObject::MessageObject(
    uint32_t _id,
    TrueType t) :
    id(_id),
    type(FieldType::BOOLEAN) {
  new (&data_) uint8_t(1);
}

MessageObject::MessageObject(
    uint32_t _id,
    FalseType f) :
    id(_id),
    type(FieldType::BOOLEAN) {
  new (&data_) uint8_t(0);
}

MessageObject::MessageObject(
    const MessageObject& other) :
    id(other.id),
    type(other.type) {
  switch (type) {

    case FieldType::OBJECT:
      new (&data_) Vector<MessageObject>(other.asObject());
      break;

    case FieldType::STRING:
      new (&data_) String(other.asString());
      break;

    case FieldType::UINT32:
      new (&data_) uint32_t(other.asUInt32());
      break;

    case FieldType::BOOLEAN:
      new (&data_) uint8_t(other.asBool() ? 1 : 0);
      break;

  }
}

MessageObject& MessageObject::operator=(const MessageObject& other) {
  switch (type) {

    case FieldType::OBJECT:
      ((Vector<MessageObject>*) &data_)->~vector();
      break;

    case FieldType::STRING:
      ((String*) &data_)->~String();
      break;

    case FieldType::UINT32:
    case FieldType::BOOLEAN:
      break;

  }

  id = other.id;
  type = other.type;

  switch (type) {

    case FieldType::OBJECT:
      new (&data_) Vector<MessageObject>(other.asObject());
      break;

    case FieldType::STRING:
      new (&data_) String(other.asString());
      break;

    case FieldType::UINT32:
      new (&data_) uint32_t(other.asUInt32());
      break;

    case FieldType::BOOLEAN:
      new (&data_) uint8_t(other.asBool() ? 1 : 0);
      break;

  }

  return *this;
}

MessageObject::~MessageObject() {
  switch (type) {

    case FieldType::OBJECT:
      ((Vector<MessageObject>*) &data_)->~vector();
      break;

    case FieldType::STRING:
      ((String*) &data_)->~String();
      break;

    case FieldType::UINT32:
    case FieldType::BOOLEAN:
      break;

  }
}

Vector<MessageObject>& MessageObject::asObject() const {
#ifndef FNORD_NODEBUG
  if (type != FieldType::OBJECT) {
    RAISE(kTypeError);
  }
#endif

  return *((Vector<MessageObject>*) &data_);
}

const String& MessageObject::asString() const {
#ifndef FNORD_NODEBUG
  if (type != FieldType::STRING) {
    RAISE(kTypeError);
  }
#endif

  return *((String*) &data_);
}

uint32_t MessageObject::asUInt32() const {
#ifndef FNORD_NODEBUG
  if (type != FieldType::UINT32) {
    RAISE(kTypeError);
  }
#endif

  return *((uint32_t*) &data_);
}

bool MessageObject::asBool() const {
  if (type == FieldType::UINT32) {
    uint32_t val = *((uint32_t*) &data_);
    return val > 0;
  }

#ifndef FNORD_NODEBUG
  if (type != FieldType::BOOLEAN) {
    RAISE(kTypeError);
  }
#endif

  uint8_t val = *((uint8_t*) &data_);
  return val > 0;
}


//MessageObject::MessageObject(uint32_t id, uint32_t value);
//MessageObject::MessageObject(uint32_t id, String value);
//MessageObject::MessageObject(uint32_t id, bool value);

} // namespace msg
} // namespace fnord
