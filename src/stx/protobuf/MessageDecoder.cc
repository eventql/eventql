/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/protobuf/MessageDecoder.h>
#include <stx/inspect.h>

namespace stx {
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
          msg->addChild(fid, msg::TRUE);
        } else {
          msg->addChild(fid, msg::FALSE);
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
} // namespace stx

