/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/protobuf/MessageEncoder.h>

namespace stx {
namespace msg {

void MessageEncoder::encode(
    const MessageObject& msg,
    const MessageSchema& schema,
    Buffer* buf) {
  util::BinaryMessageWriter body;
  for (const auto& o : msg.asObject()) {
    encodeObject(o, schema, &body);
  }
  buf->append(body.data(), body.size());
}

void MessageEncoder::encodeObject(
    const MessageObject& msg,
    const MessageSchema& schema,
    util::BinaryMessageWriter* data) {
  try {
    switch (schema.fieldType(msg.id)) {

      case FieldType::OBJECT: {
        util::BinaryMessageWriter cld;
        Vector<Pair<uint32_t, uint64_t>> obj_fields;
        auto obj_schema = schema.fieldSchema(msg.id);
        for (const auto& o : msg.asObject()) {
          encodeObject(o, *obj_schema, &cld);
        }

        data->appendVarUInt((msg.id << 3) | 0x2);
        data->appendVarUInt(cld.size());
        data->append(cld.data(), cld.size());
        return;
      }

      case FieldType::STRING: {
        const auto& str = msg.asString();
        data->appendVarUInt((msg.id << 3) | 0x2);
        data->appendVarUInt(str.size());
        data->append(str.data(), str.size());
        break;
      }

      case FieldType::UINT32:
        data->appendVarUInt((msg.id << 3) | 0x0);
        data->appendVarUInt(msg.asUInt32());
        break;

      case FieldType::UINT64:
      case FieldType::DATETIME:
        data->appendVarUInt((msg.id << 3) | 0x0);
        data->appendVarUInt(msg.asUInt64());
        break;

      case FieldType::DOUBLE:
        data->appendVarUInt((msg.id << 3) | 0x1);
        data->appendDouble(msg.asDouble());
        break;

      case FieldType::BOOLEAN:
        data->appendVarUInt((msg.id << 3) | 0x0);
        data->appendVarUInt(msg.asBool() ? 1 : 0);
        break;

    }
  } catch (const std::exception& e) {
    RAISEF(
        kRuntimeError,
        "error while encoding field $0 ('$1'): $2",
        msg.id,
        schema.fieldName(msg.id),
        e.what());
  }
}


} // namespace msg
} // namespace stx

