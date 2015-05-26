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
#include "fnord-base/wallclock.h"
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
      uploadQueryFeed(session);
      uploadRecoQueryFeed(session);
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
  json.addObjectEntry("time");
  json.addInteger(WallClock::unixMicros() / kMicrosPerSecond);
  json.addComma();
  json.addObjectEntry("session_id");
  json.addString(session.customer_session_id());
  json.addComma();
  json.addObjectEntry("ab_test_group");
  json.addString(StringUtil::toString(session.ab_test_group()));
  json.addComma();
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
  for (const auto& q : session.search_queries()) {
    Set<String> product_list;
    Set<String> clicked_products;
    for (const auto& item : q.result_items()) {
      product_list.emplace(item.item_id());
      if (item.clicked()) {
        clicked_products.emplace(item.item_id());
      }
    }

    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("time");
    json.addInteger(WallClock::unixMicros() / kMicrosPerSecond);
    json.addComma();
    json.addObjectEntry("session_id");
    json.addString(session.customer_session_id());
    json.addComma();
    json.addObjectEntry("ab_test_group");
    json.addString(StringUtil::toString(q.ab_test_group()));
    json.addComma();
    json.addObjectEntry("query_string");
    json.addString(q.query_string_normalized());
    json.addComma();
    json.addObjectEntry("page_number");
    json.addInteger(q.page());
    json.addComma();
    json.addObjectEntry("product_list");
    json::toJSON(product_list, &json);
    json.addComma();
    json.addObjectEntry("clicked_products");
    json::toJSON(clicked_products, &json);
    json.endObject();

    auto topic = StringUtil::format(
        "logjoin.ecommerce_search_queries.$0",
        session.customer());

    broker_client_.insert(broker_addr_, topic, buf);
  }
}

void LogJoinUpload::uploadRecoQueryFeed(const JoinedSession& session) {
  for (const auto& q : session.search_queries()) {
    switch (q.page_type()) {
      case PAGETYPE_SEARCH_PAGE:
      case PAGETYPE_CATALOG_PAGE:
        break;

      default:
        continue;
    }

    Set<String> product_list;
    Set<String> clicked_products;

    for (const auto& item : q.result_items()) {
      if (!item.is_recommendation()) {
        continue;
      }

      product_list.emplace(item.item_id());
      if (item.clicked()) {
        clicked_products.emplace(item.item_id());
      }
    }

    if (product_list.empty()){
      continue;
    }

    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("time");
    json.addInteger(q.time());
    json.addComma();
    json.addObjectEntry("session_id");
    json.addString(session.customer_session_id());
    json.addComma();
    json.addObjectEntry("ab_test_group");
    json.addString(StringUtil::toString(q.ab_test_group()));
    json.addComma();
    json.addObjectEntry("product_list");
    json::toJSON(product_list, &json);
    json.addComma();
    json.addObjectEntry("clicked_products");
    json::toJSON(clicked_products, &json);
    json.endObject();

    auto topic = StringUtil::format(
        "logjoin.ecommerce_recommendations.$0",
        session.customer());

    broker_client_.insert(broker_addr_, topic, buf);
  }
}



} // namespace cm

