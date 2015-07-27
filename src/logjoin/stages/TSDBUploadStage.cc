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
#include "stx/protobuf/MessagePrinter.h"
#include "logjoin/stages/TSDBUploadStage.h"
#include "logjoin/common.h"
#include "common/SessionSchema.h"
#include "tsdb/RecordEnvelope.pb.h"
#include "tsdb/TimeWindowPartitioner.h"

using namespace stx;

namespace cm {

void TSDBUploadStage::process(
    RefPtr<SessionContext> ctx,
    const String& tsdb_addr,
    http::HTTPConnectionPool* http) {
  const auto& logjoin_cfg = ctx->customer_config->config.logjoin_config();

  auto attrs_schema = msg::MessageSchema::decode(
      logjoin_cfg.session_attributes_schema());

  msg::DynamicMessage attrs(attrs_schema);
  for (const auto& attr : ctx->attributes()) {
    attrs.addField(attr.first, attr.second);
  }

  auto attrs_obj = attrs.data();
  attrs_obj.id = 1;

  msg::MessageObject session;
  session.addChild(attrs_obj);

  iputs(
      "session: $0",
      msg::MessagePrinter::print(
          session,
          *SessionSchema::forCustomer(ctx->customer_config->config)));

  //tsdb::RecordEnvelopeList records;

  //auto time = ctx->session.last_seen_time() * kMicrosPerSecond;
  //auto record_id = SHA1::compute(ctx->uuid);
  //auto stream_key = "web.sessions";
  //auto partition_key = tsdb::TimeWindowPartitioner::partitionKeyFor(
  //    stream_key,
  //    time,
  //    4 * kMicrosPerHour);

  //auto record_data = msg::encode(ctx->session);

  //auto r = records.add_records();
  //r->set_tsdb_namespace(ctx->customer_key);
  //r->set_stream_key(stream_key);
  //r->set_partition_key(partition_key.toString());
  //r->set_record_id(record_id.toString());
  //r->set_record_data(record_data->data(), record_data->size());

  //URI uri(StringUtil::format("http://$0/tsdb/insert", tsdb_addr));
  //http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
  //req.addHeader("Host", uri.hostAndPort());
  //req.addHeader("Content-Type", "application/fnord-msg");
  //req.addBody(*msg::encode(records));

  //auto res = http->executeRequest(req);
  //res.wait();

  //if (res.get().statusCode() != 201) {
  //  RAISEF(
  //      kRuntimeError,
  //      "received non-201 response: $0",
  //      res.get().body().toString());
  //}
}

} // namespace cm

