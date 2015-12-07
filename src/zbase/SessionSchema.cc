/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/SessionSchema.h"

using namespace stx;

namespace zbase {

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
                72,
                "device_id",
                msg::FieldType::STRING,
                0,
                false,
                true),
            msg::MessageSchemaField(
                72,
                "user_id",
                msg::FieldType::STRING,
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

  //for (const auto& evschema : cfg.logjoin_config().session_event_schemas()) {
  //  TableDefinition td;
  //  td.set_customer(cfg.customer());
  //  td.set_table_name("sessions." + evschema.evtype());
  //  auto tblcfg = td.mutable_config();
  //  tblcfg->set_schema(evschema.schema());
  //  tblcfg->set_partitioner(zbase::TBL_PARTITION_TIMEWINDOW);
  //  tblcfg->set_storage(zbase::TBL_STORAGE_COLSM);
  //  tbls.emplace_back(td);
  //}

  if (!cfg.has_logjoin_config()) {
    return tbls;
  }

  {
    TableDefinition td;
    td.set_customer(cfg.customer());
    td.set_table_name("sessions");
    auto tblcfg = td.mutable_config();
    tblcfg->set_schema(SessionSchema::forCustomer(cfg)->encode().toString());
    tblcfg->set_partitioner(zbase::TBL_PARTITION_TIMEWINDOW);
    tblcfg->set_storage(zbase::TBL_STORAGE_COLSM);
    tbls.emplace_back(td);
  }

  return tbls;
}

} // namespace zbase
