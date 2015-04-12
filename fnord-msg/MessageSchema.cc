/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/stringutil.h>
#include <fnord-base/exception.h>
#include <fnord-base/inspect.h>
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
      attrs += StringUtil::format(" @maxval=$0", field.type_size);
      break;

    case FieldType::STRING:
      type_name = "string";
      attrs += StringUtil::format(" @maxlen=$0", field.type_size);
      break;

  }

  switch (field.encoding) {

    case EncodingHint::NONE:
      break;

    case EncodingHint::BITPACK:
      attrs += " @encoding=BITPACK";
      break;

  }


  str->append(StringUtil::format(
      "$0$1$2$3 $4 = $5$6;\n",
      ws,
      type_prefix,
      type_name,
      type_suffix,
      field.name,
      field.id,
      attrs));
}

static void addFieldToIDIndex(
    const String& prefix,
    const MessageSchemaField& field,
    HashMap<String, uint32_t>* field_ids,
    HashMap<uint32_t, FieldType>* field_types) {
  auto colname = prefix + field.name;
  field_ids->emplace(colname, field.id);
  field_types->emplace(field.id, field.type);
  for (const auto& f : field.fields) {
    addFieldToIDIndex(colname + ".", f, field_ids, field_types);
  }
}

MessageSchema::MessageSchema(
    const String& _name,
    Vector<MessageSchemaField> _fields) :
    name(_name),
    fields(_fields) {
  for (const auto& f : fields) {
    addFieldToIDIndex("", f, &field_ids, &field_types);
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

uint32_t MessageSchema::id(const String& path) const {
  auto id = field_ids.find(path);
  if (id == field_ids.end()) {
    RAISEF(kIndexError, "unknown field: $0", path);
  } else {
    return id->second;
  }
}

FieldType MessageSchema::type(uint32_t id) const {
  if (id == 0) {
    return FieldType::OBJECT;
  }

  auto type = field_types.find(id);
  if (type == field_types.end()) {
    RAISEF(kIndexError, "unknown field: $0", id);
  } else {
    return type->second;
  }
}

} // namespace msg
} // namespace fnord
