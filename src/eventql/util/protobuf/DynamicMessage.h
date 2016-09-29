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
#pragma once
#include <eventql/util/stdtypes.h>
#include "eventql/util/protobuf/MessageObject.h"
#include "eventql/util/protobuf/MessageSchema.h"

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
