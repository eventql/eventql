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
#include "fnord-http/httprequest.h"

using namespace fnord;

namespace cm {

LogJoinUpload::LogJoinUpload(
    RefPtr<mdb::MDB> db,
    const String& feedserver_url,
    http::HTTPConnectionPool* http) :
    db_(db),
    feedserver_url_(feedserver_url),
    http_(http),
    batch_size_(kDefaultBatchSize) {}

void LogJoinUpload::upload() {
  while (scanQueue("__uploadq-sessions") > 0);
}

size_t LogJoinUpload::scanQueue(const String& queue_name) {
  auto txn = db_->startTransaction();
  auto cursor = txn->getCursor();

  Buffer key;
  Buffer value;
  bool eof = false;
  Vector<Buffer> batch;
  for (int i = 0; i < batch_size_ && !eof; ++i) {
    if (i == 0) {
      key.append(queue_name);
      eof = !cursor->getFirstOrGreater(&key, &value);
    } else {
      eof = !cursor->getNext(&key, &value);
    }

    if (!StringUtil::beginsWith(key.toString(), queue_name)) {
      eof = true;
    }

    if (!eof) {
      batch.emplace_back(value);
      txn->del(key);
    }
  }

  cursor->close();

  try {
    if (batch.size() > 0) {
      uploadBatch(queue_name, batch);
    }
  } catch (...) {
    txn->abort();
    throw;
  }

  txn->commit();

  return batch.size();
}

void LogJoinUpload::uploadBatch(
    const String& queue_name,
    const Vector<Buffer>& batch) {
  URI uri(feedserver_url_ + "/eventdb/insert?table=dawanda_joined_sessions");

  http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  req.addHeader("Content-Type", "application/fnord-msg");

  Buffer body;
  for (const auto& b : batch) {
    body.append(b.data(), b.size());
  }

  req.addBody(body);

  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 201) {
    RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
  }
}

} // namespace cm

