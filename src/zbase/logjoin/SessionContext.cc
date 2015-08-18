/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/protobuf/JSONEncoder.h"
#include "zbase/logjoin/SessionContext.h"

using namespace stx;

namespace zbase {

OutputEvent::OutputEvent(
    UnixTime _time,
    SHA1Hash _evid,
    RefPtr<msg::MessageSchema> schema) :
    time(_time),
    evid(_evid),
    obj(schema) {}

SessionContext::SessionContext(
    TrackedSession session,
    RefPtr<CustomerConfigRef> cconf) :
    uuid(session.uuid),
    customer_key(session.customer_key),
    customer_config(cconf),
    events(session.events) {}

RefPtr<OutputEvent> SessionContext::addOutputEvent(
    UnixTime time,
    SHA1Hash evid,
    const String& evtype) {
  const auto& logjoin_cfg = customer_config->config.logjoin_config();

  for (const auto& evschema : logjoin_cfg.session_event_schemas()) {
    if (evschema.evtype() == evtype) {
      auto schema = msg::MessageSchema::decode(evschema.schema());

      auto event = mkRef(new OutputEvent(time, evid, schema));
      output_events_.emplace_back(event);
      return event;
    }
  }

  RAISEF(kNotFoundError, "event schema not found: $0", evtype);
  return nullptr;
}

void SessionContext::setAttribute(const String& key, const String& value) {
  attributes_[key] = value;
}

const HashMap<String, String>& SessionContext::attributes() const {
  return attributes_;
}

const Vector<RefPtr<OutputEvent>>& SessionContext::outputEvents() const {
  return output_events_;
}

} // namespace zbase
