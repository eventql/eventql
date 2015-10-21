/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/stringutil.h>
#include <stx/exception.h>
#include <stx/inspect.h>
#include <stx/io/fileutil.h>
#include <stx/protobuf/MessageSchema.h>
#include <stx/protobuf/msg.h>
#include <stx/CodingOptions.pb.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/compiler/parser.h>
#include <3rdparty/simdcomp/simdcomp.h>

namespace stx {
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

      case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
        fields.emplace_back(
            field->number(),
            field->name(),
            msg::FieldType::DOUBLE,
            0,
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
    json->addString("NONE");

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

    addField(
        MessageSchemaField(
            id.get(),
            name.get(),
            fieldTypeFromString(type.get()),
            type_size.isEmpty() ? 0 : type_size.get(),
            repeated.isEmpty() ? false : repeated.get(),
            optional.isEmpty() ? false : optional.get()));
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

MessageSchemaRepository::MessageSchemaRepository() :
    pool_(google::protobuf::DescriptorPool::generated_pool()) {}

RefPtr<MessageSchema> MessageSchemaRepository::getSchema(
    const String& name) const {
  auto iter = schemas_.find(name);
  if (iter != schemas_.end()) {
    return iter->second;
  }

  auto desc = pool_.FindMessageTypeByName(name);
  if (desc != NULL) {
    return MessageSchema::fromProtobuf(desc);
  }

  RAISEF(kRuntimeError, "schema not found: '$0'", name);
}

void MessageSchemaRepository::registerSchema(RefPtr<MessageSchema> schema) {
  schemas_.emplace(schema->name(), schema);
}

void MessageSchemaRepository::loadProtobufFile(
    const String& base_path,
    const String& file_path) {
  auto data = FileUtil::read(FileUtil::joinPaths(base_path, file_path));
  google::protobuf::io::ArrayInputStream is(data.data(), data.size());

  google::protobuf::FileDescriptorProto fd_proto;
  google::protobuf::io::Tokenizer tokenizer(&is, NULL);
  google::protobuf::compiler::Parser parser;
  if (!parser.Parse(&tokenizer, &fd_proto)) {
    RAISEF(kParseError, "error while parsing .proto file '$0'", file_path);
  }

  fd_proto.set_name(file_path);
  pool_.BuildFile(fd_proto);
}

} // namespace msg
} // namespace stx
