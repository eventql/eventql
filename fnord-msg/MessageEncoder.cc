/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-msg/MessageEncoder.h>

namespace fnord {
namespace msg {

void MessageEncoder::encode(
    const MessageObject& msg,
    const MessageSchema& schema,
    Buffer* buf) {
  Vector<Pair<uint32_t, uint64_t>> fields;
  util::BinaryMessageWriter header;
  util::BinaryMessageWriter body;

  encodeObject(msg, schema, &fields, &body);

  header.appendVarUInt(fields.size());
  for (const auto& f : fields) {
    header.appendVarUInt(f.first);
    header.appendVarUInt(f.second);
  }

  buf->append(header.data(), header.size());
  buf->append(body.data(), body.size());
}

void MessageEncoder::encodeObject(
    const MessageObject& msg,
    const MessageSchema& schema,
    Vector<Pair<uint32_t, uint64_t>>* fields,
    util::BinaryMessageWriter* data) {
  switch (schema.type(msg.id)) {

    case FieldType::OBJECT: {
      Vector<Pair<uint32_t, uint64_t>> obj_fields;
      for (const auto& o : msg.asObject()) {
        encodeObject(o, schema, &obj_fields, data);
      }

      fields->emplace_back(msg.id, data->size());
      for (const auto& f : obj_fields) {
        fields->emplace_back(f);
      }
      return;
    }

    case FieldType::STRING: {
      const auto& str = msg.asString();
      data->append(str.data(), str.size());
      break;
    }

    case FieldType::UINT32:
      data->appendVarUInt(msg.asUInt32());
      break;

    case FieldType::BOOLEAN:
      data->appendUInt8(msg.asBool() ? 1 : 0);
      break;

  }

  fields->emplace_back(msg.id, data->size());
}


} // namespace msg
} // namespace fnord

