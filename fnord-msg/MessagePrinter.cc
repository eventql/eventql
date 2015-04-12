/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-msg/MessagePrinter.h>
#include <fnord-base/stringutil.h>

namespace fnord {
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

  switch (schema.type(msg.id)) {

    case FieldType::OBJECT: {
      String str;
      str.append(StringUtil::format("$0$1 {\n", ws, schema.name(msg.id)));

      for (const auto& o : msg.asObject()) {
        str.append(printObject(level + 1, o, schema));
      }

      str.append(ws + "}\n");
      return str;
    }

    case FieldType::BOOLEAN:
      return StringUtil::format(
          "$0$1 = $2\n",
          ws,
          schema.name(msg.id),
          msg.asBool());

    case FieldType::STRING:
      return StringUtil::format(
          "$0$1 = $2\n",
          ws,
          schema.name(msg.id),
          msg.asString());

    case FieldType::UINT32:
      return StringUtil::format(
          "$0$1 = $2\n",
          ws,
          schema.name(msg.id),
          msg.asUInt32());

  }
}

} // namespace msg
} // namespace fnord

