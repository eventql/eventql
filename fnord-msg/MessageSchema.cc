/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/StringUtil.h>
#include <fnord-msg/MessageSchema.h>

namespace fnord {
namespace msg {

static void schemaNodeToString(
    size_t level,
    const MessageSchemaField& field,
    String* str) {
  String ws(level * 2, ' ');
  switch (field.type) {

    case FieldType::OBJECT:
      str->append(StringUtil::format("$0object $1 {\n", ws, field.name));
      for (const auto& f : field.fields) {
        schemaNodeToString(level + 1, f, str);
      }
      str->append(ws + "}\n");
      break;

    case FieldType::BOOLEAN:
      str->append(StringUtil::format(
          "$0bool $1 = $2;\n",
          ws,
          field.name,
          field.id));
      break;

    case FieldType::UINT32:
      str->append(StringUtil::format(
          "$0uint32 $1 = $2 [max=$3];\n",
          ws,
          field.name,
          field.id,
          field.type_size));
      break;

    case FieldType::STRING:
      str->append(StringUtil::format(
          "$0string $1 = $2 [maxlen=$3];\n",
          ws,
          field.name,
          field.id,
          field.type_size));
      break;

  }
}

String MessageSchema::toString() const {
  String str = StringUtil::format("object $0 {\n", name);

  for (const auto& f : fields) {
    schemaNodeToString(1, f, &str);
  }

  str += "}";
  return str;
}

} // namespace msg
} // namespace fnord
