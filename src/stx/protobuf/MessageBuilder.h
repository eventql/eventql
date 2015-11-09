/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifdef TRASH
#include <stx/protobuf/MessageSchema.h>
#include <stx/stdtypes.h>
#include <stx/buffer.h>
#include <stx/util/binarymessagewriter.h>

namespace stx {
namespace msg {

class MessageBuilder {
public:

  void setUInt32(const String path, uint32_t value);
  void setString(const String path, const String& value);
  void setBool(const String path, bool value);

  void encode(const MessageSchema& schema, Buffer* buf);

  size_t countRepetitions(const String& path) const;
  bool isSet(const String& path) const;

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


  MessageSchema* schema_;
  HashMap<String, FieldValue> fields_;
};

} // namespace msg
} // namespace stx

#endif
