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

RefPtr<msg::MessageSchema> SessionSchema::forCustomer(
    const CustomerConfig& cfg) {
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
            msg::MessageSchemaField(
                70,
                "session_id",
                msg::FieldType::STRING,
                0,
                false,
                true),
            msg::MessageSchemaField(
                71,
                "time",
                msg::FieldType::DATETIME,
                0,
                false,
                true),
            msg::MessageSchemaField(
                53,
                "first_seen_time",
                msg::FieldType::DATETIME,
                0,
                false,
                true),
            msg::MessageSchemaField(
                54,
                "last_seen_time",
                msg::FieldType::DATETIME,
                0,
                false,
                true),
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

Vector<TableDefinition> SessionSchema::tableDefinitionsForCustomer(
    const CustomerConfig& cfg) {
  Vector<TableDefinition> tbls;

  for (const auto& evschema : cfg.logjoin_config().session_event_schemas()) {
    TableDefinition td;
    td.set_customer(cfg.customer());
    td.set_table_name("sessions." + evschema.evtype());
    td.set_type(TBL_LOGTABLE);
    td.set_schema_inline(evschema.schema());
    tbls.emplace_back(td);
  }

  {
    TableDefinition td;
    td.set_customer(cfg.customer());
    td.set_table_name("sessions");
    td.set_type(TBL_LOGTABLE);
    td.set_schema_inline(SessionSchema::forCustomer(cfg)->encode().toString());
    tbls.emplace_back(td);
  }

  return tbls;
}

} // namespace cm
