/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_MSG_MESSAGESCHEMA_H
#define _STX_MSG_MESSAGESCHEMA_H
#include <stx/stdtypes.h>
#include <stx/exception.h>
#include <stx/autoref.h>
#include <stx/util/binarymessagereader.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/protobuf/MessageObject.h>
#include <stx/json/json.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

/**
 * // http://tools.ietf.org/html/rfc5234
 *
 *   <message> := <object>
 *
 *   <message> :=
 *       <varint>               // num fields
 *       { <field_ptr> }        // one field pointer for each field
 *       { <uint8_t> }          // field data
 *
 *   <field_ptr> :=
 *       <varint>               // field id
 *       <varint>               // field data end offset
 *
 */
namespace stx {
namespace msg {

enum class EncodingHint : uint8_t {
  NONE = 0,
  BITPACK = 1,
  LEB128 = 2
};

class MessageSchema;

struct MessageSchemaField {

  static MessageSchemaField mkObjectField(
      uint32_t id,
      String name,
      bool repeated,
      bool optional,
      RefPtr<msg::MessageSchema> schema);

  MessageSchemaField(
    uint32_t _id,
    String _name,
    FieldType _type,
    uint64_t _type_size,
    bool _repeated,
    bool _optional,
    EncodingHint _encoding = EncodingHint::NONE) :
    id(_id),
    name(_name),
    type(_type),
    type_size(_type_size),
    repeated(_repeated),
    optional(_optional),
    encoding(_encoding),
    schema(nullptr) {}

  String typeName() const;
  size_t typeSize() const;

  uint32_t id;
  String name;
  FieldType type;
  uint64_t type_size;
  bool repeated;
  bool optional;
  EncodingHint encoding;
  RefPtr<MessageSchema> schema;
};

class MessageSchema : public RefCounted {
public:

  static RefPtr<MessageSchema> fromProtobuf(
      const google::protobuf::Descriptor* dsc);

  MessageSchema(
      const String& name,
      Vector<MessageSchemaField> fields);

  MessageSchema(std::nullptr_t);

  MessageSchema(const MessageSchema& other);

  const String& name() const;
  void setName(const String& name);

  const Vector<MessageSchemaField>& fields() const;
  bool hasField(const String& name) const;
  bool hasField(uint32_t id) const;
  uint32_t fieldId(const String& name) const;
  FieldType fieldType(uint32_t id) const;
  const String& fieldName(uint32_t id) const;
  RefPtr<MessageSchema> fieldSchema(uint32_t id) const;

  Vector<Pair<String, MessageSchemaField>> columns() const;
  String toString() const;

  Buffer encode() const;
  void encode(util::BinaryMessageWriter* buf) const;
  void decode(util::BinaryMessageReader* buf);
  static RefPtr<msg::MessageSchema> decode(const String& buf);

  void toJSON(json::JSONOutputStream* json) const;
  void fromJSON(
      json::JSONObject::const_iterator begin,
      json::JSONObject::const_iterator end);

protected:

  void addField(const MessageSchemaField& field);

  void findColumns(
      const MessageSchemaField& field,
      const String& prefix,
      Set<String>* columns);

  String name_;
  Vector<MessageSchemaField> fields_;
  HashMap<String, uint32_t> field_ids_;
  HashMap<uint32_t, FieldType> field_types_;
  HashMap<uint32_t, String> field_names_;
};

class MessageSchemaRepository {
public:

  MessageSchemaRepository();

  RefPtr<MessageSchema> getSchema(const String& name) const;

  void registerSchema(RefPtr<MessageSchema> schema);

  void loadProtobufFile(const String& base_path, const String& file_path);

protected:
  HashMap<String, RefPtr<MessageSchema>> schemas_;
  google::protobuf::DescriptorPool pool_;
};

} // namespace msg
} // namespace stx

#endif
