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
#include <eventql/util/protobuf/MessagePrinter.h>
#include <eventql/util/stringutil.h>

namespace msg {

String MessagePrinter::print(
    const MessageObject& msg,
    const MessageSchema& schema) {
  return printObject(0, msg, schema);
}

String MessagePrinter::printObject(
    size_t level,
    const MessageObject& msg,
    const MessageSchema& schema) {
  String ws(level * 2, ' ');

  String str;
  str.append(StringUtil::format("$0$1 {\n", ws, schema.name()));

  for (const auto& o : msg.asObject()) {
    str.append(printField(level + 1, o, schema));
  }

  str.append(ws + "}\n");
  return str;
}

String MessagePrinter::printField(
    size_t level,
    const MessageObject& msg,
    const MessageSchema& schema) {
  String ws(level * 2, ' ');

  switch (schema.fieldType(msg.id)) {

    case FieldType::OBJECT:
      return printObject(level, msg, *schema.fieldSchema(msg.id));

    case FieldType::BOOLEAN:
      return StringUtil::format(
          "$0$1 = $2\n",
          ws,
          schema.fieldName(msg.id),
          msg.asBool());

    case FieldType::STRING:
      return StringUtil::format(
          "$0$1 = \"$2\"\n",
          ws,
          schema.fieldName(msg.id),
          msg.asString());

    case FieldType::UINT32:
      return StringUtil::format(
          "$0$1 = $2\n",
          ws,
          schema.fieldName(msg.id),
          msg.asUInt32());

    case FieldType::UINT64:
      return StringUtil::format(
          "$0$1 = $2\n",
          ws,
          schema.fieldName(msg.id),
          msg.asUInt64());

    case FieldType::DATETIME:
      return StringUtil::format(
          "$0$1 = $2\n",
          ws,
          schema.fieldName(msg.id),
          msg.asUnixTime());

    case FieldType::DOUBLE:
      return StringUtil::format(
          "$0$1 = $2\n",
          ws,
          schema.fieldName(msg.id),
          msg.asDouble());

  }
}

} // namespace msg

