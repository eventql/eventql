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
#include <eventql/util/protobuf/MessageDecoder.h>
#include <eventql/util/inspect.h>

namespace msg {

void MessageDecoder::decode(
    const Buffer& buf,
    const MessageSchema& schema,
    MessageObject* msg) {
  decode(buf.data(), buf.size(), schema, msg);
}

void MessageDecoder::decode(
    const void* data,
    size_t size,
    const MessageSchema& schema,
    MessageObject* msg) {
  util::BinaryMessageReader reader(data, size);

  while (reader.remaining() > 0) {
    auto fkey = reader.readVarUInt();
    auto fid = fkey >> 3;

    /* skip unknown fields */
    if (!schema.hasField(fid)) {
      auto wire_type = (WireType) uint8_t(fkey & 0x7);
      switch (wire_type) {

        case WireType::VARINT:
          reader.readVarUInt();
          break;

        case WireType::LENENC:
          reader.read(reader.readVarUInt());
          break;

        case WireType::FIXED32:
          reader.readUInt32();
          break;

        case WireType::FIXED64:
          reader.readUInt64();
          break;

        default:
          RAISE(kRuntimeError, "invalid wire type");

      }

      continue;
    }

    /* parse known fields */
    switch (schema.fieldType(fid)) {
      case FieldType::OBJECT: {
        auto len = reader.readVarUInt();
        auto nxt = &msg->addChild(fid);
        if (len > reader.remaining()) {
          RAISE(kBufferOverflowError);
        }

        auto obj_schema = schema.fieldSchema(fid);
        decode(reader.read(len), len, *obj_schema, nxt);
        break;
      }

      case FieldType::UINT32: {
        auto val = reader.readVarUInt();
        msg->addChild(fid, (uint32_t) val);
        break;
      }

      case FieldType::UINT64: {
        auto val = reader.readVarUInt();
        msg->addChild(fid, (uint64_t) val);
        break;
      }

      case FieldType::DATETIME: {
        auto val = reader.readVarUInt();
        msg->addChild(fid, UnixTime(val));
        break;
      }

      case FieldType::DOUBLE: {
        auto val = reader.readDouble();
        msg->addChild(fid, val);
        break;
      }

      case FieldType::BOOLEAN: {
        auto val = reader.readVarUInt();
        if (val == 1) {
          msg->addChild(fid, msg::MSG_TRUE);
        } else {
          msg->addChild(fid, msg::MSG_FALSE);
        }
        break;
      }

      case FieldType::STRING: {
        auto len = reader.readVarUInt();
        auto val = reader.read(len);
        msg->addChild(fid, String((const char*) val, len));
        break;
      }

    }
  }
}


} // namespace msg

