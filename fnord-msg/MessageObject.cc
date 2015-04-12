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

namespace fnord {
namespace msg {

MessageObject::MessageObject(uint32_t _id /* = 0 */) : id(_id) {
  new (&data_) Vector<MessageObject>();
}

MessageObject::MessageObject(uint32_t _id, uint32_t value) : id(_id) {
  new (&data_) uint32_t(value);
}

MessageObject::MessageObject(uint32_t _id, const String& value) : id(_id) {
  new (&data_) String(value);
}

MessageObject::MessageObject(uint32_t _id, bool value) : id(_id) {
  new (&data_) uint8_t(value ? 1 : 0);
}

Vector<MessageObject>& MessageObject::asObject() const {
  return *((Vector<MessageObject>*) &data_);
}

const String& MessageObject::asString() const {
  return *((String*) &data_);
}

uint32_t MessageObject::asUInt32() const {
  return *((uint32_t*) &data_);
}

bool MessageObject::asBool() const {
  uint8_t val = *((uint8_t*) &data_);
  return val > 0;
}


//MessageObject::MessageObject(uint32_t id, uint32_t value);
//MessageObject::MessageObject(uint32_t id, String value);
//MessageObject::MessageObject(uint32_t id, bool value);

} // namespace msg
} // namespace fnord
