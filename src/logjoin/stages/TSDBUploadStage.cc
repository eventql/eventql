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
#include "logjoin/stages/TSDBUploadStage.h"
#include "logjoin/common.h"
#include "tsdb/RecordEnvelope.pb.h"
#include "tsdb/TimeWindowPartitioner.h"

using namespace stx;

namespace cm {

void TSDBUploadStage::process(
    RefPtr<TrackedSessionContext> ctx,
    const String& tsdb_addr,
    http::HTTPConnectionPool* http) {
  tsdb::RecordEnvelopeList records;

  auto time = ctx->joined_session.last_seen_time() * kMicrosPerSecond;
  auto record_id = SHA1::compute(ctx->tracked_session.uid);
  auto stream_key = "web.sessions";
  auto partition_key = tsdb::TimeWindowPartitioner::partitionKeyFor(
      stream_key,
      time,
      4 * kMicrosPerHour);

  auto record_data = msg::encode(ctx->joined_session);

  auto r = records.add_records();
  r->set_tsdb_namespace(ctx->tracked_session.customer_key);
  r->set_stream_key(stream_key);
  r->set_partition_key(partition_key.toString());
  r->set_record_id(record_id.toString());
  r->set_record_data(record_data->data(), record_data->size());

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

} // namespace cm

