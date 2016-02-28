/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "frontend/IndexFeedUpload.h"
#include "stx/uri.h"
#include "stx/logging.h"
#include "stx/util/binarymessagewriter.h"
#include "stx/http/httprequest.h"
#include "stx/protobuf/MessageSchema.h"
#include "stx/protobuf/MessagePrinter.h"
#include "stx/protobuf/MessageEncoder.h"
#include "stx/protobuf/msg.h"
#include "schemas.h"
#include "unistd.h"

using namespace stx;

namespace zbase {

IndexFeedUpload::IndexFeedUpload(
    const String& target_url,
    thread::Queue<IndexChangeRequest>* queue,
    http::HTTPConnectionPool* http,
    RefPtr<msg::MessageSchema> schema) :
    target_url_(target_url),
    queue_(queue),
    http_(http),
    schema_(schema),
    batch_size_(kDefaultBatchSize),
    running_(true) {}

void IndexFeedUpload::start() {
  running_ = true;
  thread_ = std::thread(std::bind(&IndexFeedUpload::run, this));
}

void IndexFeedUpload::stop() {
  if (!running_) {
    return;
  }

  running_ = false;
  thread_.join();
}

void IndexFeedUpload::uploadNext() {
  Vector<IndexChangeRequest> reqs;
  Set<String> customers;

  reqs.emplace_back(queue_->pop());
  customers.emplace(reqs.back().customer());

  for (int i = 0; i < 100; ++i) {
    auto nxt = queue_->poll();
    if (nxt.isEmpty()) {
      break;
    } else {
      reqs.emplace_back(nxt.get());
      customers.emplace(nxt.get().customer());
    }
  }

  for (const auto& customer : customers) {
    Vector<IndexChangeRequest> batch;

    for (const auto& r : reqs) {
      if (r.customer() == customer) {
        batch.emplace_back(r);
      }
    }

    uploadBatch(customer, batch);
  }
}

void IndexFeedUpload::uploadBatch(
    const String& customer,
    const Vector<IndexChangeRequest>& batch) {
  util::BinaryMessageWriter body;

  for (const auto& job : batch) {
    auto change_req = job;
    change_req.set_customer(customer);
    auto buf = msg::encode(change_req);
    body.appendVarUInt(buf->size());
    body.append(buf->data(), buf->size());
  }

  URI uri(target_url_ + "/insert_batch?table=index_feed-" + customer);

  http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  req.addHeader("Content-Type", "application/fnord-msg");
  req.addBody(body.data(), body.size());

  uploadWithRetries(req);
}

// FIXPAUL retry...
void IndexFeedUpload::uploadWithRetries(const http::HTTPRequest& req) {
  size_t sleeptime = 1000; // 1ms
  auto max_retries = 14;

  for (int i = 0; i < max_retries; ++i) {
    try {
      auto res = http_->executeRequest(req);
      res.wait();

      const auto& r = res.get();
      if (r.statusCode() != 201) {
        RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
      }

      return;
    } catch (const Exception& e) {
      stx::logError("cm.frontend", e, "error in IndexFeedUploader");
    }

    usleep(sleeptime);
    sleeptime *= 2;
  }

  RAISE(kRuntimeError, "too many retries... giving up");
}

void IndexFeedUpload::run() {
  while (running_) {
    try {
      uploadNext();
    } catch (const std::exception& e) {
      stx::logError("cm.frontend", e, "error in IndexFeedUploader");
    }
  }
}


} // namespace zbase

