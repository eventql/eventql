/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include "eventql/util/protobuf/MessageSchema.h"
#include "eventql/CustomerConfig.h"

using namespace stx;

namespace eventql {

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

} // namespace eventql

