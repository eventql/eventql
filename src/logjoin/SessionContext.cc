/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/protobuf/JSONEncoder.h"
#include "logjoin/SessionContext.h"

using namespace stx;

namespace cm {

JoinedEvent::JoinedEvent(
    RefPtr<msg::MessageSchema> _schema) :
    schema(_schema) {}

void JoinedEvent::addUInt32Field(const String& name, uint32_t val) {
  if (!schema->hasField(name)) {
    return;
  }

  auto field_id = schema->fieldId(name);
  if (schema->fieldType(field_id) != msg::FieldType::UINT32) {
    return;
  }

  data.addChild(field_id, uint32_t(val));
}

void JoinedEvent::addBoolField(const String& name, bool val) {
  if (!schema->hasField(name)) {
    return;
  }

  auto field_id = schema->fieldId(name);
  if (schema->fieldType(field_id) != msg::FieldType::BOOLEAN) {
    return;
  }

  if (val) {
    data.addChild(field_id, msg::TRUE);
  } else {
    data.addChild(field_id, msg::FALSE);
  }
}

void JoinedEvent::addStringField(const String& name, const String& value) {
  if (!schema->hasField(name)) {
    return;
  }

  auto field_id = schema->fieldId(name);
  if (schema->fieldType(field_id) != msg::FieldType::STRING) {
    return;
  }

  data.addChild(field_id, value);
}

void JoinedEvent::addObject(
    const String& name,
    Function<void (JoinedEvent* ev)> fn) {
  if (!schema->hasField(name)) {
    return;
  }

  auto field_id = schema->fieldId(name);
  if (schema->fieldType(field_id) != msg::FieldType::OBJECT) {
    return;
  }

  auto subschema = schema->fieldSchema(field_id);

  JoinedEvent subev(subschema);
  fn(&subev);
  subev.data.id = field_id;
  data.addChild(subev.data);
}

void JoinedEvent::toJSON(json::JSONOutputStream* json) const {
  json->beginObject();
  json->addObjectEntry("type");
  json->addString(schema->name());
  json->addComma();
  json->addObjectEntry("data");
  msg::JSONEncoder::encode(data, *schema, json);
  json->endObject();
}

SessionContext::SessionContext(
    TrackedSession session,
    RefPtr<CustomerConfigRef> cconf) :
    uuid(session.uuid),
    customer_key(session.customer_key),
    customer_config(cconf),
    events(session.events) {}

JoinedEvent* SessionContext::addOutputEvent(const String& evtype) {
  const auto& logjoin_cfg = customer_config->config.logjoin_config();

  for (const auto& evschema : logjoin_cfg.session_events()) {
    if (evschema.evtype() == evtype) {
      auto schema = msg::MessageSchema::decode(evschema.schema());

      auto event = new JoinedEvent(schema);
      output_events_.emplace_back(event);
      return event;
    }
  }

  RAISEF(kNotFoundError, "event schema not found: $0 -- $1", evtype, logjoin_cfg.DebugString());
  return nullptr;
}

const Vector<ScopedPtr<JoinedEvent>>& SessionContext::outputEvents() const {
  return output_events_;
}

} // namespace cm
