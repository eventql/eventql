/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord/stringutil.h>
#include <fnord/exception.h>
#include <fnord/inspect.h>
#include <fnord/protobuf/MessageSchema.h>
#include <fnord/protobuf/msg.h>
#include <fnord/CodingOptions.pb.h>

namespace fnord {
namespace msg {

RefPtr<MessageSchema> MessageSchema::fromProtobuf(
    const google::protobuf::Descriptor* dsc) {
  Vector<msg::MessageSchemaField> fields;

  auto nfields = dsc->field_count();
  for (size_t i = 0; i < nfields; ++i) {
    auto field = dsc->field(i);

    CodingOptions copts = field->options().GetExtension(coding);

    EncodingHint enc_hint = EncodingHint::NONE;
    if (copts.encoding() == "BITPACK") enc_hint = EncodingHint::BITPACK;
    if (copts.encoding() == "LEB128") enc_hint = EncodingHint::LEB128;

    switch (field->type()) {

      case google::protobuf::FieldDescriptor::TYPE_BOOL:
        fields.emplace_back(
            field->number(),
            field->name(),
            msg::FieldType::BOOLEAN,
            0,
            field->is_repeated(),
            field->is_optional(),
            enc_hint);
        break;

      case google::protobuf::FieldDescriptor::TYPE_STRING:
        fields.emplace_back(
            field->number(),
            field->name(),
            msg::FieldType::STRING,
            copts.has_maxval() ? copts.maxval() : 0xffffffff,
            field->is_repeated(),
            field->is_optional(),
            enc_hint);
        break;

      case google::protobuf::FieldDescriptor::TYPE_UINT64:
        fields.emplace_back(
            field->number(),
            field->name(),
            msg::FieldType::UINT64,
            copts.has_maxval() ?
                copts.maxval() : std::numeric_limits<uint64_t>::max(),
            field->is_repeated(),
            field->is_optional(),
            enc_hint);
        break;

      case google::protobuf::FieldDescriptor::TYPE_UINT32:
        fields.emplace_back(
            field->number(),
            field->name(),
            msg::FieldType::UINT32,
            copts.has_maxval() ?
                copts.maxval() : std::numeric_limits<uint32_t>::max(),
            field->is_repeated(),
            field->is_optional(),
            enc_hint);
        break;

      case google::protobuf::FieldDescriptor::TYPE_ENUM: {
        size_t maxval = 0;
        auto enum_dsc = field->enum_type();
        auto nvals = enum_dsc->value_count();
        for (int j = 0; j < nvals; ++j) {
          auto enum_val = enum_dsc->value(j);
          if (enum_val->number() > maxval) {
            maxval = enum_val->number();
          }
        }

        fields.emplace_back(
            field->number(),
            field->name(),
            msg::FieldType::UINT32,
            maxval,
            field->is_repeated(),
            field->is_optional(),
            maxval < 250 ? EncodingHint::BITPACK : EncodingHint::LEB128);
        break;
      }

      case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
        fields.emplace_back(
            MessageSchemaField::mkObjectField(
                field->number(),
                field->name(),
                field->is_repeated(),
                field->is_optional(),
                MessageSchema::fromProtobuf(field->message_type())));
        break;

      default:
        RAISEF(
            kNotImplementedError,
            "field type not implemented: $0",
            field->type_name());

    }
  }

  return new msg::MessageSchema(dsc->full_name(), fields);
}

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

      for (const auto& f : field.schema->fields()) {
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

    case FieldType::UINT64:
      type_name = "uint64";
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

    case EncodingHint::LEB128:
      attrs += " @encoding=LEB128";
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

MessageSchemaField MessageSchemaField::mkObjectField(
    uint32_t id,
    String name,
    bool repeated,
    bool optional,
    RefPtr<msg::MessageSchema> schema) {
  MessageSchemaField field(
      id,
      name,
      FieldType::OBJECT,
      0,
      repeated,
      optional);

  field.schema = schema;
  return field;
}

MessageSchema::MessageSchema(
    const String& name,
    Vector<MessageSchemaField> fields) :
    name_(name),
    fields_(fields) {
  for (const auto& field : fields_) {
    field_ids_.emplace(field.name, field.id);
    field_types_.emplace(field.id, field.type);
    field_names_.emplace(field.id, field.name);
  }
}

MessageSchema::MessageSchema(const MessageSchema& other) :
    name_(other.name_),
    fields_(other.fields_),
    field_ids_(other.field_ids_),
    field_types_(other.field_types_),
    field_names_(other.field_names_) {}

const String& MessageSchema::name() const {
  return name_;
}

const Vector<MessageSchemaField>& MessageSchema::fields() const {
  return fields_;
}

String MessageSchema::toString() const {
  String str = StringUtil::format("object $0 {\n", name_);

  for (const auto& f : fields_) {
    schemaNodeToString(1, f, &str);
  }

  str += "}";
  return str;
}

uint32_t MessageSchema::fieldId(const String& path) const {
  auto id = field_ids_.find(path);
  if (id == field_ids_.end()) {
    RAISEF(kIndexError, "field not found: $0", path);
  } else {
    return id->second;
  }
}

FieldType MessageSchema::fieldType(uint32_t id) const {
  if (id == 0) {
    return FieldType::OBJECT;
  }

  auto type = field_types_.find(id);
  if (type == field_types_.end()) {
    RAISEF(kIndexError, "field not found: $0", id);
  } else {
    return type->second;
  }
}

const String& MessageSchema::fieldName(uint32_t id) const {
  if (id == 0) {
    return name_;
  }

  auto name = field_names_.find(id);
  if (name == field_names_.end()) {
    RAISEF(kIndexError, "field not found: $0", id);
  } else {
    return name->second;
  }
}

RefPtr<MessageSchema> MessageSchema::fieldSchema(uint32_t id) const {
  for (const auto& field : fields_) {
    if (field.id == id) {
      return field.schema;
    }
  }

  RAISEF(kIndexError, "field not found: $0", id);
}

Set<String> MessageSchema::columns() const {
  Set<String> columns;

  for (const auto& c : field_names_) {
    columns.emplace(c.second);
  }

  return columns;
}

RefPtr<MessageSchema> MessageSchemaRepository::getSchema(
    const String& name) const {
  auto iter = schemas_.find(name);
  if (iter == schemas_.end()) {
    RAISEF(kRuntimeError, "schema not found: '$0'", name);
  }

  return iter->second;
}

void MessageSchemaRepository::registerSchema(RefPtr<MessageSchema> schema) {
  schemas_.emplace(schema->name(), schema);
}

} // namespace msg
} // namespace fnord
