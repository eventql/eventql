/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/stdtypes.h>
#include <stx/human.h>
#include <stx/protobuf/DynamicMessage.h>
#include <stx/protobuf/JSONEncoder.h>

namespace stx {
namespace msg {

DynamicMessage::DynamicMessage(
    RefPtr<msg::MessageSchema> schema) :
    schema_(schema) {}

bool DynamicMessage::addField(const String& name, const String& value) {
  if (!schema_->hasField(name)) {
    return false;
  }

  auto field_id = schema_->fieldId(name);
  return addField(field_id, value);
}

bool DynamicMessage::addField(uint32_t field_id, const String& value) {
  if (!schema_->hasField(field_id)) {
    return false;
  }

  switch (schema_->fieldType(field_id)) {

    case msg::FieldType::BOOLEAN: {
      auto v = Human::parseBoolean(value);
      if (v.isEmpty()) {
        return false;
      } else {
        if (v.get()) {
          data_.addChild(field_id, msg::TRUE);
        } else {
          data_.addChild(field_id, msg::FALSE);
        }
        return true;
      }
    }

    case msg::FieldType::UINT32:
      try {
        auto v = std::stoull(value);
        data_.addChild(field_id, uint32_t(v));
        return true;
      } catch (...) {
        return false;
      }

    case msg::FieldType::UINT64:
      try {
        auto v = std::stoull(value);
        data_.addChild(field_id, uint64_t(v));
        return true;
      } catch (...) {
        return false;
      }

    case msg::FieldType::DOUBLE:
      try {
        auto v = std::stod(value);
        data_.addChild(field_id, double(v));
        return true;
      } catch (...) {
        return false;
      }

    case msg::FieldType::DATETIME: {
      auto v = Human::parseTime(value);
      if (v.isEmpty()) {
        return false;
      } else {
        data_.addChild(field_id, v.get());
        return true;
      }
    }

    case msg::FieldType::STRING:
      data_.addChild(field_id, value);
      return true;

    case msg::FieldType::OBJECT:
      return false;

  }
}

bool DynamicMessage::addUInt32Field(const String& name, uint32_t val) {
  if (!schema_->hasField(name)) {
    return false;
  }

  auto field_id = schema_->fieldId(name);
  if (schema_->fieldType(field_id) != msg::FieldType::UINT32) {
    return false;
  }

  data_.addChild(field_id, uint32_t(val));
  return true;
}

bool DynamicMessage::addUInt64Field(const String& name, uint64_t val) {
  if (!schema_->hasField(name)) {
    return false;
  }

  auto field_id = schema_->fieldId(name);
  if (schema_->fieldType(field_id) != msg::FieldType::UINT64) {
    return false;
  }

  data_.addChild(field_id, uint64_t(val));
  return true;
}

bool DynamicMessage::addDateTimeField(const String& name, const UnixTime& val) {
  if (!schema_->hasField(name)) {
    return false;
  }

  auto field_id = schema_->fieldId(name);
  return addDateTimeField(field_id, val);
}

bool DynamicMessage::addDateTimeField(uint32_t field_id, const UnixTime& val) {
  if (!schema_->hasField(field_id)) {
    return false;
  }

  if (schema_->fieldType(field_id) != msg::FieldType::DATETIME) {
    return false;
  }

  data_.addChild(field_id, val);
  return true;
}

bool DynamicMessage::addBoolField(const String& name, bool val) {
  if (!schema_->hasField(name)) {
    return false;
  }

  auto field_id = schema_->fieldId(name);
  if (schema_->fieldType(field_id) != msg::FieldType::BOOLEAN) {
    return false;
  }

  if (val) {
    data_.addChild(field_id, msg::TRUE);
  } else {
    data_.addChild(field_id, msg::FALSE);
  }

  return true;
}

bool DynamicMessage::addStringField(const String& name, const String& value) {
  if (!schema_->hasField(name)) {
    return false;
  }

  auto field_id = schema_->fieldId(name);
  if (schema_->fieldType(field_id) != msg::FieldType::STRING) {
    return false;
  }

  data_.addChild(field_id, value);
  return true;
}

bool DynamicMessage::addObject(
    const String& name,
    Function<void (DynamicMessage* msg)> fn) {
  if (!schema_->hasField(name)) {
    return false;
  }

  auto field_id = schema_->fieldId(name);
  if (schema_->fieldType(field_id) != msg::FieldType::OBJECT) {
    return false;
  }

  auto subschema = schema_->fieldSchema(field_id);

  DynamicMessage submsg(subschema);
  fn(&submsg);
  submsg.data_.id = field_id;
  data_.addChild(submsg.data_);

  return true;
}

void DynamicMessage::toJSON(json::JSONOutputStream* json) const {
  json->beginObject();
  json->addObjectEntry("type");
  json->addString(schema_->name());
  json->addComma();
  json->addObjectEntry("data");
  msg::JSONEncoder::encode(data_, *schema_, json);
  json->endObject();
}

const msg::MessageObject& DynamicMessage::data() const {
  return data_;
}

RefPtr<msg::MessageSchema> DynamicMessage::schema() const {
  return schema_;
}

} // namespace msg
} // namespace stx

