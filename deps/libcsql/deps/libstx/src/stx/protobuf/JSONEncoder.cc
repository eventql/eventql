/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/protobuf/JSONEncoder.h>

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

