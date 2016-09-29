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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/exception.h>
#include <eventql/util/autoref.h>
#include <eventql/util/util/binarymessagereader.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/util/protobuf/MessageObject.h>
#include <eventql/util/json/json.h>

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
  uint32_t maxFieldId();

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

  void addField(const MessageSchemaField& field);
  void removeField(uint32_t id);

protected:

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

} // namespace msg

