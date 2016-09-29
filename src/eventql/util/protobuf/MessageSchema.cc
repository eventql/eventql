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
#include <eventql/util/stringutil.h>
#include <eventql/util/exception.h>
#include <eventql/util/inspect.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/protobuf/MessageSchema.h>
#include <eventql/util/protobuf/msg.h>
#include <libsimdcomp/simdcomp.h>

namespace msg {

String MessageSchemaField::typeName() const {
  switch (type) {
    case FieldType::OBJECT:
      return "object";

    case FieldType::BOOLEAN:
      return "bool";

    case FieldType::UINT32:
      return "uint32";

    case FieldType::UINT64:
      return "uint64";

    case FieldType::STRING:
      return "string";

    case FieldType::DOUBLE:
      return "double";

    case FieldType::DATETIME:
      return "datetime";

  }
}

size_t MessageSchemaField::typeSize() const {
  switch (type) {
    case FieldType::OBJECT:
    case FieldType::BOOLEAN:
    case FieldType::DOUBLE:
    case FieldType::DATETIME:
      return 0;

    case FieldType::UINT32:
    case FieldType::UINT64:
      return bits(type_size);

    case FieldType::STRING:
      return type_size;

  }
}

static void schemaNodeToString(
    size_t level,
    const MessageSchemaField& field,
    String* str) {
  String ws(level * 2, ' ');
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

  if (field.type == FieldType::OBJECT) {
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
      field.typeName(),
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

MessageSchema::MessageSchema(std::nullptr_t) {}

MessageSchema::MessageSchema(
    const String& name,
    Vector<MessageSchemaField> fields) :
    name_(name) {
  for (const auto& field : fields) {
    addField(field);
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

void MessageSchema::setName(const String& name) {
  name_ = name;
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

bool MessageSchema::hasField(const String& path) const {
  auto id = field_ids_.find(path);
  return id != field_ids_.end();
}

bool MessageSchema::hasField(uint32_t id) const {
  auto iter = field_types_.find(id);
  return iter != field_types_.end();
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

void MessageSchema::addField(const MessageSchemaField& field) {
  field_ids_.emplace(field.name, field.id);
  field_types_.emplace(field.id, field.type);
  field_names_.emplace(field.id, field.name);
  fields_.emplace_back(field);
}

void MessageSchema::removeField(uint32_t id) {
  for (auto f = fields_.begin(); f != fields_.end(); ++f) {
    if (f->id != id) {
      continue;
    }

    field_ids_.erase(f->name);
    field_types_.erase(f->id);
    field_names_.erase(f->id);
    fields_.erase(f);
    return;
  }

  RAISEF(kNotFoundError, "field not found: $0", id);
}

Buffer MessageSchema::encode() const {
  util::BinaryMessageWriter writer;
  encode(&writer);
  return Buffer(writer.data(), writer.size());
}

void MessageSchema::encode(util::BinaryMessageWriter* buf) const {
  buf->appendUInt8(0x1);
  buf->appendLenencString(name_);
  buf->appendVarUInt(fields_.size());

  for (const auto& field : fields_) {
    buf->appendVarUInt(field.id);
    buf->appendLenencString(field.name);
    buf->appendUInt8((uint8_t) field.type);
    buf->appendVarUInt(field.type_size);
    buf->appendUInt8(field.repeated);
    buf->appendUInt8(field.optional);
    buf->appendUInt8((uint8_t) field.encoding);

    if (field.type == FieldType::OBJECT) {
      field.schema->encode(buf);
    }
  }
}

void MessageSchema::decode(util::BinaryMessageReader* buf) {
  auto version = *buf->readUInt8();
  (void) version; // unused

  name_ = buf->readLenencString();

  auto nfields = buf->readVarUInt();
  for (int i = 0; i < nfields; ++i) {
    auto id = buf->readVarUInt();
    auto name = buf->readLenencString();
    auto type = (FieldType) *buf->readUInt8();
    auto type_size = buf->readVarUInt();
    auto repeated = *buf->readUInt8() > 0;
    auto optional = *buf->readUInt8() > 0;
    auto encoding = (EncodingHint) *buf->readUInt8();

    msg::MessageSchemaField field(
        id,
        name,
        type,
        type_size,
        repeated,
        optional,
        encoding);

    if (field.type == FieldType::OBJECT) {
      field.schema = mkRef(new MessageSchema(nullptr));
      field.schema->decode(buf);
    }

    addField(field);
  }
}

RefPtr<MessageSchema> MessageSchema::decode(const String& data) {
  util::BinaryMessageReader reader(data.data(), data.size());
  auto schema = mkRef(new MessageSchema(nullptr));
  schema->decode(&reader);
  return schema;
}

uint32_t MessageSchema::maxFieldId() {
  uint32_t max_field_id = 0;
  for (size_t i = 0; i < fields_.size(); ++i) {
    const auto& field = fields_[i];

    if (field.id > max_field_id) {
      max_field_id = field.id;
    }

    if (field.type == FieldType::OBJECT) {
      uint32_t nested_max_field_id = field.schema->maxFieldId();
      if (nested_max_field_id > max_field_id) {
        max_field_id = nested_max_field_id;
      }
    }
  }

  return max_field_id;
}

void MessageSchema::toJSON(json::JSONOutputStream* json) const {
  json->beginObject();

  json->addObjectEntry("name");
  json->addString(name_);
  json->addComma();

  json->addObjectEntry("columns");
  json->beginArray();

  for (int i = 0; i < fields_.size(); ++i) {
    const auto& field = fields_[i];

    if (i > 0) {
      json->addComma();
    }

    json->beginObject();

    json->addObjectEntry("id");
    json->addInteger(field.id);
    json->addComma();

    json->addObjectEntry("name");
    json->addString(field.name);
    json->addComma();

    json->addObjectEntry("type");
    json->addString(fieldTypeToString(field.type));
    json->addComma();

    json->addObjectEntry("type_size");
    json->addInteger(field.type_size);
    json->addComma();

    json->addObjectEntry("optional");
    field.optional ? json->addTrue() : json->addFalse();
    json->addComma();

    json->addObjectEntry("repeated");
    field.repeated ? json->addTrue() : json->addFalse();
    json->addComma();

    json->addObjectEntry("encoding_hint");
    switch (field.encoding) {
      case EncodingHint::BITPACK:
        json->addString("BITPACK");
        break;
      case EncodingHint::LEB128:
        json->addString("LEB128");
        break;
      default:
        json->addString("NONE");
    }

    if (field.type == FieldType::OBJECT) {
      json->addComma();
      json->addObjectEntry("schema");
      field.schema->toJSON(json);
    }

    json->endObject();
  }

  json->endArray();
  json->endObject();
}

void MessageSchema::fromJSON(
    json::JSONObject::const_iterator begin,
    json::JSONObject::const_iterator end) {
  auto tname = json::objectGetString(begin, end, "name");
  if (!tname.isEmpty()) {
    name_ = tname.get();
  }

  auto cols = json::objectLookup(begin, end, "columns");
  if (cols == end) {
    RAISE(kRuntimeError, "missing field: columns");
  }

  auto ncols = json::arrayLength(cols, end);
  for (size_t i = 0; i < ncols; ++i) {
    auto col = json::arrayLookup(cols, end, i); // O(N^2) but who cares...

    auto id = json::objectGetUInt64(col, end, "id");
    if (id.isEmpty()) {
      RAISE(kRuntimeError, "missing field: id");
    }

    auto name = json::objectGetString(col, end, "name");
    if (name.isEmpty()) {
      RAISE(kRuntimeError, "missing field: name");
    }

    auto type = json::objectGetString(col, end, "type");
    if (type.isEmpty()) {
      RAISE(kRuntimeError, "missing field: type");
    }

    auto type_size = json::objectGetUInt64(col, end, "type_size");
    auto optional = json::objectGetBool(col, end, "optional");
    auto repeated = json::objectGetBool(col, end, "repeated");

    auto field_type = fieldTypeFromString(type.get());

    if (field_type == msg::FieldType::OBJECT) {
      auto child_schema_json = json::objectLookup(col, end, "schema");
      if (child_schema_json == end) {
        RAISE(kRuntimeError, "missing field: schema");
      }

      auto child_schema = mkRef(new MessageSchema(nullptr));
      child_schema->fromJSON(child_schema_json, end);

      addField(
          MessageSchemaField::mkObjectField(
              id.get(),
              name.get(),
              repeated.isEmpty() ? false : repeated.get(),
              optional.isEmpty() ? false : optional.get(),
              child_schema));
    } else {
      addField(
          MessageSchemaField(
              id.get(),
              name.get(),
              field_type,
              type_size.isEmpty() ? 0 : type_size.get(),
              repeated.isEmpty() ? false : repeated.get(),
              optional.isEmpty() ? false : optional.get()));
    }
  }
}

Vector<Pair<String, MessageSchemaField>> MessageSchema::columns() const {
  Vector<Pair<String, MessageSchemaField>> columns;

  for (const auto& field : fields_) {
    switch (field.type) {

      case FieldType::OBJECT: {
        auto cld_cols = field.schema->columns();
        for (const auto& c : cld_cols) {
          columns.emplace_back(field.name + "." + c.first, c.second);
        }
        break;
      }

      default:
        columns.emplace_back(field.name, field);
        break;
    }
  }

  return columns;
}

} // namespace msg
