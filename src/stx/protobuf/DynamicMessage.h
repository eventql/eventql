/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include "stx/protobuf/MessageObject.h"
#include "stx/protobuf/MessageSchema.h"

namespace stx {
namespace msg {

class DynamicMessage {
public:

  DynamicMessage(RefPtr<msg::MessageSchema> schema);

  bool addField(const String& name, const String& val);
  bool addField(uint32_t id, const String& val);
  bool addUInt32Field(const String& name, uint32_t val);
  bool addUInt64Field(const String& name, uint64_t val);
  bool addDateTimeField(const String& name, const UnixTime& val);
  bool addDateTimeField(uint32_t id, const UnixTime& val);
  bool addStringField(const String& name, const String& val);
  bool addBoolField(const String& name, bool val);
  bool addObject(const String& name, Function<void (DynamicMessage* msg)> fn);

  Option<String> getField(const String& name) const;
  Option<String> getField(uint32_t id) const;
  Option<uint32_t> getUInt32Field(const String& name) const;
  Option<uint64_t> getUInt64Field(const String& name) const;
  Option<UnixTime> getDateTimeField(const String& name) const;
  Option<String> getStringField(const String& name) const;
  Option<bool> getBoolField(const String& name) const;

  void toJSON(json::JSONOutputStream* json) const;
  void fromJSON(
      json::JSONObject::const_iterator begin,
      json::JSONObject::const_iterator end);

  const msg::MessageObject& data() const;
  void setData(msg::MessageObject data);

  RefPtr<msg::MessageSchema> schema() const;

  String debugPrint() const;

protected:
  RefPtr<msg::MessageSchema> schema_;
  msg::MessageObject data_;
};

} // namespace msg
} // namespace stx

