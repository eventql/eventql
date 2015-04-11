/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_MSG_MESSAGEBOJECT_H
#define _FNORD_MSG_MESSAGEBOJECT_H
#include <fnord-base/stdtypes.h>

namespace fnord {
namespace msg {

struct MessageObject;

union MessageObjectValues {
  Vector<MessageObject> t_obj;
  String t_str;
  uint64_t t_uint;
};

struct MessageObject {
  MessageObject(uint32_t id = 0);
  MessageObject(uint32_t id, uint32_t value);
  MessageObject(uint32_t id, const String& value);
  MessageObject(uint32_t id, bool value);

  Vector<MessageObject>& asObject() const;

  template <typename... ArgTypes>
  MessageObject& addChild(ArgTypes... args) {
    auto& o = asObject();
    o.emplace_back(args...);
    return o.back();
  }

  uint32_t id;
  char data_[sizeof(MessageObjectValues)];
};


} // namespace msg
} // namespace fnord

#endif
