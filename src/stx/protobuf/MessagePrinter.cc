/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/protobuf/MessagePrinter.h>
#include <stx/stringutil.h>

namespace stx {
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
} // namespace stx

