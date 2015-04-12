/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-msg/MessageDecoder.h>
#include <fnord-base/inspect.h>

namespace fnord {
namespace msg {

void MessageDecoder::decode(
    const Buffer& buf,
    const MessageSchema& schema,
    MessageObject* msg) {
  util::BinaryMessageReader reader(buf.data(), buf.size());

  auto num_fields = *reader.readUInt32();
  Vector<Pair<uint32_t, uint64_t>> fields;

  for (int i = 0; i < num_fields; ++i) {
    auto id = *reader.readUInt32();
    auto end = *reader.readUInt32();
    fields.emplace_back(id, end);
  }

  decodeObject(0, 0, reader.remaining(), fields, schema, &reader, msg);
}

void MessageDecoder::decodeObject(
    size_t idx,
    uint64_t begin,
    uint64_t end,
    const Vector<Pair<uint32_t, uint64_t>>& fields,
    const MessageSchema& schema,
    util::BinaryMessageReader* reader,
    MessageObject* msg) {
  auto fbegin = begin;
  auto fid = fields[idx].first;
  auto fend = fields[idx].second;

  switch (schema.type(fid)) {

    case FieldType::OBJECT: {
      MessageObject* nxt;
      if (fid == 0) {
        nxt = msg;
      } else {
        nxt = &msg->addChild(fid);
      }

      auto obegin = fbegin;
      for (int i = idx + 1; i < fields.size(); ++i) {
        auto oend = fields[i].second;

        if (oend <= obegin) {
          continue;
        }

        if (oend > fend) {
          break;
        }

        decodeObject(i, obegin, oend, fields, schema, reader, nxt);

        obegin = oend;
      }
      break;
    }

    case FieldType::UINT32: {
      uint32_t val = *reader->readUInt32();
      msg->addChild(fid, val);
      break;
    }

    case FieldType::BOOLEAN: {
      uint8_t val = *reader->readUInt8();
      msg->addChild(fid, val == 1);
      break;
    }

    case FieldType::STRING: {
      auto val = reader->read(fend - fbegin);
      msg->addChild(fid, String((const char*) val, fend - fbegin));
      break;
    }

  }
}


} // namespace msg
} // namespace fnord

