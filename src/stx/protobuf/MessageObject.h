/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_MSG_MESSAGEBOJECT_H
#define _STX_MSG_MESSAGEBOJECT_H
#include <stx/stdtypes.h>
#include <stx/UnixTime.h>

namespace stx {
namespace msg {

struct MessageObject;

enum class FieldType : uint8_t {
  OBJECT = 0,
  BOOLEAN = 1,
  UINT32 = 2,
  STRING = 3,
  UINT64 = 4,
  DOUBLE = 5,
  DATETIME = 6,
};

enum class WireType : uint8_t {
  VARINT = 0,
  FIXED64 = 1,
  LENENC = 2,
  FIXED32 = 5,
};

String fieldTypeToString(FieldType type);
FieldType fieldTypeFromString(String str);

union MessageObjectValues {
  Vector<MessageObject> t_obj;
  String t_str;
  uint64_t t_uint;
  double t_dbl;
};

struct TrueType {};
struct FalseType {};
static const TrueType TRUE {};
static const FalseType FALSE {};

struct MessageObject {
  explicit MessageObject(uint32_t id = 0);
  explicit MessageObject(uint32_t id, const String& value);
  explicit MessageObject(uint32_t id, uint32_t value);
  explicit MessageObject(uint32_t id, uint64_t value);
  explicit MessageObject(uint32_t id, double value);
  explicit MessageObject(uint32_t id, TrueType t);
  explicit MessageObject(uint32_t id, FalseType f);
  explicit MessageObject(uint32_t id, UnixTime time);

  MessageObject(const MessageObject& other);
  MessageObject(MessageObject&& other) = delete;
  MessageObject& operator=(const MessageObject& other);
  ~MessageObject();

  Vector<MessageObject>& asObject() const; // fixpaul rename to children..
  const String& asString() const;
  uint32_t asUInt32() const;
  uint64_t asUInt64() const;
  bool asBool() const;
  double asDouble() const;
  UnixTime asUnixTime() const;

  MessageObject& getObject(uint32_t id) const;
  Vector<MessageObject*> getObjects(uint32_t id) const;
  size_t fieldCount(uint32_t id);
  const String& getString(uint32_t id) const;
  uint32_t getUInt32(uint32_t id) const;
  uint64_t getUInt64(uint32_t id) const;
  bool getBool(uint32_t id) const;
  double getDouble(uint32_t id) const;
  UnixTime getUnixTime(uint32_t id) const;

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
} // namespace stx

#endif
