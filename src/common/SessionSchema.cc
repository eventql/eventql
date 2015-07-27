/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "common/SessionSchema.h"

using namespace stx;

namespace cm {

RefPtr<msg::MessageSchema> SessionSchema::forCustomer(CustomerConfig& cfg) {
  Vector<msg::MessageSchemaField> events;

  for (const auto& evschema : cfg.logjoin_config().session_event_schemas()) {
    events.emplace_back(
        msg::MessageSchemaField::mkObjectField(
            evschema.evid(),
            evschema.evtype(),
            true,
            false,
            msg::MessageSchema::decode(evschema.schema())));
  }

  return new msg::MessageSchema(
      "session",
      Vector<msg::MessageSchemaField> {
            msg::MessageSchemaField::mkObjectField(
                1,
                "attr",
                false,
                true,
                msg::MessageSchema::decode(
                    cfg.logjoin_config().session_attributes_schema())),
            msg::MessageSchemaField::mkObjectField(
                2,
                "event",
                false,
                true,
                new msg::MessageSchema("event", events)),
      });
}

} // namespace cm
