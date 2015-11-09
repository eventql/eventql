/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/protobuf/MessageObject.h>
#include <stx/exception.h>

namespace stx {
namespace msg {

String fieldTypeToString(FieldType type) {
  switch (type) {
    case FieldType::OBJECT: return "OBJECT";
    case FieldType::STRING: return "STRING";
    case FieldType::BOOLEAN: return "BOOLEAN";
    case FieldType::UINT32: return "UINT32";
    case FieldType::UINT64: return "UINT64";
    case FieldType::DOUBLE: return "DOUBLE";
    case FieldType::DATETIME: return "DATETIME";
  }
}

FieldType fieldTypeFromString(String str) {
  StringUtil::toUpper(&str);

  if (str == "OBJECT") return FieldType::OBJECT;
  if (str == "STRING") return FieldType::STRING;
  if (str == "BOOLEAN") return FieldType::BOOLEAN;
  if (str == "UINT32") return FieldType::UINT32;
  if (str == "UINT64") return FieldType::UINT64;
  if (str == "DOUBLE") return FieldType::DOUBLE;
  if (str == "DATETIME") return FieldType::DATETIME;

  RAISEF(kTypeError, "can't convert '$0' to FieldType", str);
}

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
    uint64_t value) :
    id(_id),
    type(FieldType::UINT64) {
  new (&data_) uint64_t(value);
}

MessageObject::MessageObject(
    uint32_t _id,
    double value) :
    id(_id),
    type(FieldType::DOUBLE) {
  new (&data_) double(value);
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
    uint32_t _id,
    UnixTime time) :
    id(_id),
    type(FieldType::DATETIME) {
  new (&data_) uint64_t(time.unixMicros());
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

    case FieldType::UINT64:
    case FieldType::DATETIME:
      new (&data_) uint64_t(other.asUInt64());
      break;

    case FieldType::DOUBLE:
      new (&data_) double(other.asDouble());
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
    case FieldType::UINT64:
    case FieldType::DOUBLE:
    case FieldType::BOOLEAN:
    case FieldType::DATETIME:
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

    case FieldType::UINT64:
    case FieldType::DATETIME:
      new (&data_) uint64_t(other.asUInt64());
      break;

    case FieldType::DOUBLE:
      new (&data_) double(other.asDouble());
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
    case FieldType::UINT64:
    case FieldType::DOUBLE:
    case FieldType::BOOLEAN:
    case FieldType::DATETIME:
      break;

  }
}

Vector<MessageObject>& MessageObject::asObject() const {
#ifndef STX_NODEBUG
  if (type != FieldType::OBJECT) {
    RAISE(kTypeError);
  }
#endif

  return *((Vector<MessageObject>*) &data_);
}

const String& MessageObject::asString() const {
#ifndef STX_NODEBUG
  if (type != FieldType::STRING) {
    RAISE(kTypeError);
  }
#endif

  return *((String*) &data_);
}

uint32_t MessageObject::asUInt32() const {
  if (type == FieldType::UINT64) {
    return *((uint64_t*) &data_); // silent truncation :(
  }

#ifndef STX_NODEBUG
  if (type != FieldType::UINT32) {
    RAISE(kTypeError);
  }
#endif

  return *((uint32_t*) &data_);
}

uint64_t MessageObject::asUInt64() const {
  if (type == FieldType::UINT32) {
    return *((uint32_t*) &data_);
  }

#ifndef STX_NODEBUG
  if (type != FieldType::UINT64 &&
      type != FieldType::DATETIME) {
    RAISE(kTypeError, "can't convert MessageObject to UINT64");
  }
#endif

  return *((uint64_t*) &data_);
}

double MessageObject::asDouble() const {
#ifndef STX_NODEBUG
  if (type != FieldType::DOUBLE) {
    RAISE(kTypeError);
  }
#endif

  return *((double*) &data_);
}

bool MessageObject::asBool() const {
  if (type == FieldType::UINT32) {
    uint32_t val = *((uint32_t*) &data_);
    return val > 0;
  }

#ifndef STX_NODEBUG
  if (type != FieldType::BOOLEAN) {
    RAISE(kTypeError);
  }
#endif

  uint8_t val = *((uint8_t*) &data_);
  return val > 0;
}

UnixTime MessageObject::asUnixTime() const {
#ifndef STX_NODEBUG
  if (type != FieldType::DATETIME) {
    RAISE(kTypeError);
  }
#endif

  auto val = *((uint64_t*) &data_);
  return UnixTime(val);
}

MessageObject& MessageObject::getObject(uint32_t id) const {
  for (auto& f : asObject()) {
    if (f.id == id) {
      return f;
    }
  }

  RAISEF(kIndexError, "no such field: $0", id);
}

Vector<MessageObject*> MessageObject::getObjects(uint32_t id) const {
  Vector<MessageObject*> lst;

  for (auto& f : asObject()) {
    if (f.id == id) {
      lst.emplace_back(&f);
    }
  }

  return lst;
}

//bool MessageObject::getBool(uint32_t id) const;

uint32_t MessageObject::getUInt32(uint32_t id) const {
  for (const auto& f : asObject()) {
    if (f.id == id) {
      return f.asUInt32();
    }
  }

  RAISEF(kIndexError, "no such field: $0", id);
}

uint64_t MessageObject::getUInt64(uint32_t id) const {
  for (const auto& f : asObject()) {
    if (f.id == id) {
      return f.asUInt64();
    }
  }

  RAISEF(kIndexError, "no such field: $0", id);
}

const String& MessageObject::getString(uint32_t id) const {
  for (const auto& f : asObject()) {
    if (f.id == id) {
      return f.asString();
    }
  }

  RAISEF(kIndexError, "no such field: $0", id);
}

UnixTime MessageObject::getUnixTime(uint32_t id) const {
  for (const auto& f : asObject()) {
    if (f.id == id) {
      return f.asUnixTime();
    }
  }

  RAISEF(kIndexError, "no such field: $0", id);
}

double MessageObject::getDouble(uint32_t id) const {
  for (const auto& f : asObject()) {
    if (f.id == id) {
      return f.asDouble();
    }
  }

  RAISEF(kIndexError, "no such field: $0", id);
}

bool MessageObject::getBool(uint32_t id) const {
  for (const auto& f : asObject()) {
    if (f.id == id) {
      return f.asBool();
    }
  }

  RAISEF(kIndexError, "no such field: $0", id);
}

} // namespace msg
} // namespace stx
