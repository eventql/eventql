/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "logjoin/LogJoinUpload.h"
#include "fnord-base/uri.h"
#include "fnord-base/util/binarymessagereader.h"
#include "fnord-base/util/binarymessagewriter.h"
#include "fnord-http/httprequest.h"
#include "fnord-json/json.h"

using namespace fnord;

namespace cm {

LogJoinUpload::LogJoinUpload(
    RefPtr<mdb::MDB> db,
    const String& tsdb_addr,
    const String& broker_addr,
    http::HTTPConnectionPool* http) :
    db_(db),
    tsdb_addr_(tsdb_addr),
    broker_addr_(InetAddr::resolve(broker_addr)),
    http_(http),
    batch_size_(kDefaultBatchSize),
    broker_client_(http) {}

void LogJoinUpload::upload() {
  while (scanQueue("__uploadq-sessions") > 0);
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

    try {
      util::BinaryMessageReader reader(value.data(), value.size());
      reader.readUInt64();
      reader.readUInt64();
      auto ssize = reader.readVarUInt();
      auto sdata = reader.read(ssize);

      JoinedSession session;
      msg::decode<JoinedSession>(sdata, ssize, &session);

      uploadPreferenceSetFeed(session);
    } catch (...) {
      txn->abort();
      throw;
    }

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
  URI uri(tsdb_addr_ + "/tsdb/insert_batch?stream=joined_sessions.dawanda");

  http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  req.addHeader("Content-Type", "application/fnord-msg");

  util::BinaryMessageWriter body;
  for (const auto& b : batch) {
    body.append(b.data(), b.size());
  }

  req.addBody(body.data(), body.size());

  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 201) {
    RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
  }
}

void LogJoinUpload::uploadPreferenceSetFeed(const JoinedSession& session) {
  Set<String> visited_products;
  for (const auto& item_visit : session.item_visits()) {
    visited_products.emplace(item_visit.item_id());
  }

  Set<String> bought_products;
  for (const auto& cart_item : session.cart_items()) {
    if (cart_item.checkout_step() != 1) {
      continue;
    }

    bought_products.emplace(cart_item.item_id());
  }

  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
  json.beginObject();
  json.addObjectEntry("visited_products");
  json::toJSON(visited_products, &json);
  json.addComma();
  json.addObjectEntry("bought_products");
  json::toJSON(bought_products, &json);
  json.endObject();

  auto topic = StringUtil::format(
      "logjoin.ecommerce_preference_sets.$0",
      session.customer());

  broker_client_.insert(broker_addr_, topic, buf);
}

void LogJoinUpload::uploadQueryFeed(const JoinedSession& session) {
}

} // namespace cm

