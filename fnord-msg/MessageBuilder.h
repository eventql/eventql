/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_MSG_MESSAGEBUILDER_H
#define _FNORD_MSG_MESSAGEBUILDER_H
#include <fnord-msg/MessageSchema.h>
#include <fnord-base/stdtypes.h>
#include <fnord-base/buffer.h>
#include <fnord-base/util/BinaryMessageWriter.h>

namespace fnord {
namespace msg {

class MessageBuilder {
public:

  void setUInt32(const String path, uint32_t value);

  void encode(const MessageSchema& schema, Buffer* buf);

protected:
  struct FieldValue {
    FieldType type;
    Buffer value;
  };

  void encodeField(
      const String& prefix,
      const MessageSchemaField& field,
      util::BinaryMessageWriter* buf);

  void encodeSingleField(
      const String& path,
      const MessageSchemaField& field,
      util::BinaryMessageWriter* buf);

  size_t countRepetitions(const String& prefix);

  MessageSchema* schema_;
  HashMap<String, FieldValue> fields_;
};

} // namespace msg
} // namespace fnord

#endif
