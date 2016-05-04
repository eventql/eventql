/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "eventql/util/protobuf/MessageSchema.h"
#include "eventql/CustomerConfig.h"

using namespace stx;

namespace zbase {

CustomerConfig createCustomerConfig(const String& customer) {
  CustomerConfig conf;
  conf.set_customer(customer);
  return conf;
}

void eventDefinitonRemoveField(EventDefinition* def, const String& field) {
  auto schema = msg::MessageSchema::decode(def->schema());

  auto cur_schema = schema;
  auto cur_field = field;
  while (StringUtil::includes(cur_field, ".")) {
    auto prefix_len = cur_field.find('.');
    auto prefix = field.substr(0, prefix_len);

    cur_field = cur_field.substr(prefix_len + 1);
    cur_schema = cur_schema->fieldSchema(cur_schema->fieldId(prefix));
  }

  auto edit_id = cur_schema->fieldId(cur_field);
  cur_schema->removeField(edit_id);

  def->set_schema(schema->encode().toString());
}

void eventDefinitonAddField(
    EventDefinition* def,
    const String& field,
    uint32_t id,
    msg::FieldType type,
    bool repeated,
    bool optional) {
  auto schema = msg::MessageSchema::decode(def->schema());

  auto cur_schema = schema;
  auto cur_field = field;
  while (StringUtil::includes(cur_field, ".")) {
    auto prefix_len = cur_field.find('.');
    auto prefix = field.substr(0, prefix_len);

    cur_field = cur_field.substr(prefix_len + 1);

    auto next_field_id = cur_schema->fieldId(prefix);
    if (cur_schema->fieldType(next_field_id) != msg::FieldType::OBJECT) {
      RAISEF(
          kRuntimeError,
          "can't add subfield to '$0' because it is not an object",
          prefix);
    }

    cur_schema = cur_schema->fieldSchema(next_field_id);
  }

  if (type == msg::FieldType::OBJECT) {
    cur_schema->addField(
        msg::MessageSchemaField::mkObjectField(
            id,
            cur_field,
            repeated,
            optional,
            new msg::MessageSchema(nullptr)));
  } else {
    cur_schema->addField(
        msg::MessageSchemaField(id, cur_field, type, 0, repeated, optional));
  }

  def->set_schema(schema->encode().toString());
}

} // namespace zbase

