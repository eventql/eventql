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

enum class FieldType : uint8_t {
  OBJECT = 0,
  BOOLEAN = 1,
  UINT32 = 2,
  STRING = 3
};

union MessageObjectValues {
  Vector<MessageObject> t_obj;
  String t_str;
  uint64_t t_uint;
};

struct TrueType {};
struct FalseType {};
static const TrueType TRUE {};
static const FalseType FALSE {};


struct MessageObject {
  explicit MessageObject(uint32_t id = 0);
  explicit MessageObject(uint32_t id, const String& value);
  explicit MessageObject(uint32_t id, uint32_t value);
  explicit MessageObject(uint32_t id, TrueType t);
  explicit MessageObject(uint32_t id, FalseType f);

  MessageObject(const MessageObject& other);
  MessageObject(MessageObject&& other) = delete;
  MessageObject& operator=(const MessageObject& other);
  ~MessageObject();

  Vector<MessageObject>& asObject() const;
  const String& asString() const;
  uint32_t asUInt32() const;
  bool asBool() const;

  template <typename... ArgTypes>
  MessageObject& addChild(ArgTypes... args) {
    auto& o = asObject();
    o.emplace_back(args...);
    return o.back();
  }

  uint32_t id;
  FieldType type;
  char data_[sizeof(MessageObjectValues)];
};


} // namespace msg
} // namespace fnord

#endif
