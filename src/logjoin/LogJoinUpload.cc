/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "logjoin/LogJoinUpload.h"
#include "stx/uri.h"
#include "stx/util/binarymessagereader.h"
#include "stx/util/binarymessagewriter.h"
#include "stx/wallclock.h"
#include "stx/http/httprequest.h"
#include "stx/json/json.h"
#include "tsdb/RecordEnvelope.pb.h"
#include "tsdb/TimeWindowPartitioner.h"

using namespace stx;

namespace cm {

LogJoinUpload::LogJoinUpload(
    CustomerDirectory* customer_dir,
    RefPtr<mdb::MDB> db,
    const String& tsdb_addr,
    const String& broker_addr,
    http::HTTPConnectionPool* http) :
    customer_dir_(customer_dir),
    db_(db),
    tsdb_addr_(tsdb_addr),
    broker_addr_(InetAddr::resolve(broker_addr)),
    http_(http),
    broker_client_(http),
    batch_size_(kDefaultBatchSize) {}


void LogJoinUpload::upload() {
  while (scanQueue("__uploadq-sessions") > 0);
}

void LogJoinUpload::onSession(
    const JoinedSession& session) {
  auto conf = customer_dir_->logjoinConfigFor(session.customer());

  //try {
  //} catch (const Exception& e) {
  //  stx::logError("logjoind", e, "error while delivering webhooks");
  //}
}

void LogJoinUpload::deliverWebhooks(
    const LogJoinConfig& conf,
    const JoinedSession& session) {
  for (const auto hook : conf.webhooks()) {
    stx::logDebug("logjoind", "Delivering webhook: $0", hook.target_url());

    Buffer json_buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&json_buf));

    json.beginObject();
    json.addObjectEntry("session");
    //msg::JSONEncoder::encode(session, &json);
    json.endObject();

    stx::iputs("session json: $0", json_buf.toString());

    // FIXPAUL: security risk ahead, make sure url is actually external
    http::HTTPMessage::HeaderList headers;
    headers.emplace_back("X-DeepAnalytics-SessionID", session.session_id());

    auto request = http::HTTPRequest::mkPost(
        hook.target_url(),
        json_buf,
        headers);

  }
}

size_t LogJoinUpload::scanQueue(const String& queue_name) {
  auto txn = db_->startTransaction();
  auto cursor = txn->getCursor();

  Buffer key;
  Buffer value;
  Vector<Buffer> batch;
  for (int i = 0; i < batch_size_; ++i) {
    if (i == 0) {
      key.append(queue_name);

      if (!cursor->getFirstOrGreater(&key, &value)) {
        break;
      }
    } else {
      if (!cursor->getNext(&key, &value)) {
        break;
      }
    }

    if (!StringUtil::beginsWith(key.toString(), queue_name)) {
      break;
    }

    util::BinaryMessageReader reader(value.data(), value.size());
    reader.readUInt64();
    reader.readUInt64();
    auto record_size = reader.readVarUInt();
    auto record_data = reader.read(record_size);
    auto record = msg::decode<cm::JoinedSession>(record_data, record_size);
    onSession(record);

    batch.emplace_back(value);
    cursor->del();
  }

  cursor->close();

  try {
    if (batch.size() > 0) {
      uploadTSDBBatch(batch);
    }
  } catch (...) {
    txn->abort();
    throw;
  }

  txn->commit();

  return batch.size();
}

void LogJoinUpload::uploadTSDBBatch(const Vector<Buffer>& batch) {
  tsdb::RecordEnvelopeList records;

  for (const auto& b : batch) {
    util::BinaryMessageReader reader(b.data(), b.size());
    auto time = *reader.readUInt64();
    auto record_id = SHA1::compute(StringUtil::toString(*reader.readUInt64()));
    auto record_size = reader.readVarUInt();
    auto record_data = reader.read(record_size);

    auto stream_key = "web.sessions";
    auto partition_key = tsdb::TimeWindowPartitioner::partitionKeyFor(
        stream_key,
        time,
        4 * kMicrosPerHour);

    auto r = records.add_records();
    r->set_tsdb_namespace("dawanda");
    r->set_stream_key(stream_key);
    r->set_partition_key(partition_key.toString());
    r->set_record_id(record_id.toString());
    r->set_record_data(record_data, record_size);
  }

  URI uri(StringUtil::format("http://$0/tsdb/insert", tsdb_addr_));
  http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  req.addHeader("Content-Type", "application/fnord-msg");
  req.addBody(*msg::encode(records));

  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 201) {
    RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
  }
}

} // namespace cm

