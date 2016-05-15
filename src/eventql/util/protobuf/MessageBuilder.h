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
#ifdef TRASH
#include <eventql/util/protobuf/MessageSchema.h>
#include <eventql/util/stdtypes.h>
#include <eventql/util/buffer.h>
#include <eventql/util/util/binarymessagewriter.h>

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
