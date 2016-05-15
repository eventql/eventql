/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include <eventql/util/protobuf/JSONEncoder.h>

namespace stx {
namespace msg {

void JSONEncoder::encode(
    const MessageObject& msg,
    const MessageSchema& schema,
    json::JSONOutputStream* json) {
  json->beginObject();

  const auto& fields =  msg.asObject();
  size_t n = 0;
  for (const auto& field : schema.fields()) {
    Set<size_t> indexes;
    for (int i = 0; i < fields.size(); ++i) {
      if (field.id == fields[i].id) {
        indexes.emplace(i);
      }
    }

    if (indexes.empty()) {
      continue;
    }

    if (++n > 1) {
      json->addComma();
    }

    json->addObjectEntry(field.name);

    if (field.repeated) {
      json->beginArray();
    }

    size_t i = 0;
    for (const auto& idx : indexes) {
      if (++i > 1) {
        json->addComma();
      }

      encodeField(fields[idx], field, json);
    }

    if (field.repeated) {
      json->endArray();
    }
  }

  json->endObject();
}

void JSONEncoder::encodeField(
    const MessageObject& msg,
    const MessageSchemaField& field,
    json::JSONOutputStream* json) {
  switch (field.type) {

    case FieldType::OBJECT: {
      encode(msg, *field.schema, json);
      break;
    }

    case FieldType::BOOLEAN:
      if (msg.asBool()) {
        json->addTrue();
      } else {
        json->addFalse();
      }
      break;

    case FieldType::STRING:
      json->addString(msg.asString());
      break;

    case FieldType::UINT32:
      json->addInteger(msg.asUInt32());
      break;

    case FieldType::UINT64:
      json->addInteger(msg.asUInt64());
      break;

    case FieldType::DATETIME:
      json->addInteger(msg.asUnixTime().unixMicros());
      break;

    case FieldType::DOUBLE:
      json->addFloat(msg.asDouble());
      break;

  }
}

} // namespace msg
} // namespace stx

