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
#include <fnord-base/exception.h>
#include <fnord-msg/MessageSchema.h>

namespace fnord {
namespace msg {

static void schemaNodeToString(
    size_t level,
    const MessageSchemaField& field,
    String* str) {
  String ws(level * 2, ' ');
  String type_name;
  String attrs;

  String type_prefix = "";
  String type_suffix = "";

  if (field.optional) {
    type_prefix += "optional[";
    type_suffix += "]";
  }

  if (field.repeated) {
    type_prefix += "list[";
    type_suffix += "]";
  }

  switch (field.type) {

    case FieldType::OBJECT:
      str->append(StringUtil::format(
          "$0$1object$2 $3 = $4 {\n",
          ws,
          type_prefix,
          type_suffix,
          field.name,
          field.id));

      for (const auto& f : field.fields) {
        schemaNodeToString(level + 1, f, str);
      }

      str->append(ws + "}\n");
      return;

    case FieldType::BOOLEAN:
      type_name = "bool";
      break;

    case FieldType::UINT32:
      type_name = "uint32";
      attrs = StringUtil::format("[max=$0]", field.type_size);
      break;

    case FieldType::STRING:
      type_name = "string";
      attrs = StringUtil::format("[maxlen=$0]", field.type_size);
      break;

  }

  str->append(StringUtil::format(
      "$0$1$2$3 $4 = $5$6$7;\n",
      ws,
      type_prefix,
      type_name,
      type_suffix,
      field.name,
      field.id,
      attrs.length() > 0 ? " " : "",
      attrs));
}

static void sortSchemaFields(Vector<MessageSchemaField>* fields) {
  std::sort(fields->begin(), fields->end(), [] (
      const MessageSchemaField& a,
      const MessageSchemaField& b) {
    return a.id < b.id;
  });

  for (auto& f : *fields) {
    if (!f.fields.empty()) {
      sortSchemaFields(&f.fields);
    }
  }
}

MessageSchema::MessageSchema(
    const String& _name,
    Vector<MessageSchemaField> _fields) :
    name(_name),
    fields(_fields) {
  sortSchemaFields(&fields);
}

String MessageSchema::toString() const {
  String str = StringUtil::format("object $0 {\n", name);

  for (const auto& f : fields) {
    schemaNodeToString(1, f, &str);
  }

  str += "}";
  return str;
}

uint32_t MessageSchema::id(const String& path) {
  String p = path;
  String k;
  size_t s = 0;

  const Vector<MessageSchemaField>* fs = &fields;
  while (s != String::npos) {
    s = StringUtil::find(p, '.');
    if (s == String::npos) {
      k = p;
    } else {
      k = p.substr(0, s);
      p = p.substr(s + 1);
    }

    for (const auto& f : *fs) {
      if (f.name == k) {
        if (s == String::npos) {
          return f.id;
        } else {
          fs = &f.fields;
        }

        break;
      }
    }
  }

  RAISEF(kIndexError, "unknown field: $0", path);
}

} // namespace msg
} // namespace fnord
