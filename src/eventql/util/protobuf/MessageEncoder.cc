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
#include <eventql/util/protobuf/MessageEncoder.h>

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

