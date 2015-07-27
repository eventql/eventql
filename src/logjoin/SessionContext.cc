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

SessionContext::SessionContext(
    TrackedSession session,
    RefPtr<CustomerConfigRef> cconf) :
    uuid(session.uuid),
    customer_key(session.customer_key),
    customer_config(cconf),
    events(session.events) {}

msg::DynamicMessage* SessionContext::addOutputEvent(const String& evtype) {
  const auto& logjoin_cfg = customer_config->config.logjoin_config();

  for (const auto& evschema : logjoin_cfg.session_events()) {
    if (evschema.evtype() == evtype) {
      auto schema = msg::MessageSchema::decode(evschema.schema());

      auto event = new msg::DynamicMessage(schema);
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

const Vector<ScopedPtr<msg::DynamicMessage>>& SessionContext::outputEvents() const {
  return output_events_;
}

} // namespace cm
