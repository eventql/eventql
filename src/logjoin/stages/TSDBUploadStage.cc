/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/SHA1.h"
#include "stx/protobuf/msg.h"
#include "stx/protobuf/MessageEncoder.h"
#include "stx/protobuf/MessagePrinter.h"
#include "logjoin/stages/TSDBUploadStage.h"
#include "logjoin/common.h"
#include "zbase/SessionSchema.h"
#include "zbase/core/TimeWindowPartitioner.h"

using namespace stx;

namespace cm {

void TSDBUploadStage::process(
    RefPtr<SessionContext> ctx,
    const String& tsdb_addr,
    http::HTTPConnectionPool* http) {
  tsdb::RecordEnvelopeList records;
  serializeSession(ctx, &records);

  //for (const auto& ev : ctx->outputEvents()) {
  //  serializeEvent(ctx, ev, &records);
  //}

  URI uri(StringUtil::format("http://$0/tsdb/insert", tsdb_addr));
  http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  req.addHeader("Content-Type", "application/fnord-msg");
  req.addBody(*msg::encode(records));

  auto res = http->executeRequest(req);
  res.wait();

  if (res.get().statusCode() != 201) {
    RAISEF(
        kRuntimeError,
        "received non-201 response: $0",
        res.get().body().toString());
  }
}

void TSDBUploadStage::serializeSession(
    RefPtr<SessionContext> ctx,
    tsdb::RecordEnvelopeList* records) {
  const auto& logjoin_cfg = ctx->customer_config->config.logjoin_config();

  HashMap<String, uint32_t> event_ids;
  for (const auto& evschema : logjoin_cfg.session_event_schemas()) {
    event_ids.emplace(evschema.evtype(), evschema.evid());
  }

  auto session_schema = SessionSchema::forCustomer(
      ctx->customer_config->config);

  msg::DynamicMessage session(session_schema);
  session.addField("session_id", ctx->uuid);
  session.addDateTimeField("time", ctx->time);
  session.addDateTimeField("last_seen_time", ctx->last_seen_time);
  session.addDateTimeField("first_seen_time", ctx->first_seen_time);

  /* add attributes */
  session.addObject("attr", [ctx] (msg::DynamicMessage* obj) {
    for (const auto& attr : ctx->attributes()) {
      obj->addField(attr.first, attr.second);
    }
  });

  /* add events */
  msg::MessageObject events_obj(2);

  for (const auto& ev : ctx->outputEvents()) {
    auto evid = event_ids.find(ev->obj.schema()->name());
    if (evid == event_ids.end()) {
      continue;
    }

    auto ev_obj = ev->obj.data();
    ev_obj.id = evid->second;
    events_obj.addChild(ev_obj);
  }

  auto session_data = session.data();
  session_data.addChild(events_obj);

  /* encode session */
  Buffer record_data;
  msg::MessageEncoder::encode(session_data, *session_schema, &record_data);

  /* add to record list */
  auto record_id = SHA1::compute(ctx->uuid);
  auto table_name = "sessions";
  auto partition_key = tsdb::TimeWindowPartitioner::partitionKeyFor(
      table_name,
      ctx->time,
      4 * kMicrosPerHour);

  auto r = records->add_records();
  r->set_tsdb_namespace(ctx->customer_key);
  r->set_table_name(table_name);
  r->set_partition_sha1(partition_key.toString());
  r->set_record_id(record_id.toString());
  r->set_record_data(record_data.data(), record_data.size());
}

void TSDBUploadStage::serializeEvent(
    RefPtr<SessionContext> ctx,
    RefPtr<OutputEvent> ev,
    tsdb::RecordEnvelopeList* records) {
  Buffer record_data;
  msg::MessageEncoder::encode(
      ev->obj.data(),
      *ev->obj.schema(),
      &record_data);

  /* add to record list */
  auto record_id = ev->evid;
  auto table_name = "sessions." + ev->obj.schema()->name();
  auto partition_key = tsdb::TimeWindowPartitioner::partitionKeyFor(
      table_name,
      ev->time,
      4 * kMicrosPerHour);

  auto r = records->add_records();
  r->set_tsdb_namespace(ctx->customer_key);
  r->set_table_name(table_name);
  r->set_partition_sha1(partition_key.toString());
  r->set_record_id(record_id.toString());
  r->set_record_data(record_data.data(), record_data.size());
}

} // namespace cm

