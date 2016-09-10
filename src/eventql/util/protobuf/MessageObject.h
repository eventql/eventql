/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#ifndef _STX_MSG_MESSAGEBOJECT_H
#define _STX_MSG_MESSAGEBOJECT_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/UnixTime.h>

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
static const TrueType MSG_TRUE {};
static const FalseType MSG_FALSE {};

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
  size_t fieldCount(uint32_t id) const;
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

#endif
