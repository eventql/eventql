/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "frontend/IndexFeedUpload.h"
#include "fnord-base/uri.h"
#include "fnord-http/httprequest.h"
#include "fnord-msg/MessageSchema.h"
#include "fnord-msg/MessagePrinter.h"
#include "fnord-msg/MessageEncoder.h"
#include "schemas.h"
#include "unistd.h"

using namespace fnord;

namespace cm {

IndexFeedUpload::IndexFeedUpload(
    const String& target_url,
    thread::Queue<IndexChangeRequest>* queue,
    http::HTTPConnectionPool* http) :
    target_url_(target_url),
    queue_(queue),
    http_(http),
    batch_size_(kDefaultBatchSize),
    running_(true),
    schema_(indexChangeRequestSchema()) {}

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
  customers.emplace(reqs.back().customer);

  for (int i = 0; i < 100; ++i) {
    auto nxt = queue_->poll();
    if (nxt.isEmpty()) {
      break;
    } else {
      reqs.emplace_back(nxt.get());
      customers.emplace(nxt.get().customer);
    }
  }

  for (const auto& customer : customers) {
    Vector<IndexChangeRequest> batch;

    for (const auto& r : reqs) {
      if (r.customer == customer) {
        batch.emplace_back(r);
      }
    }

    uploadBatch(customer, batch);
  }
}

void IndexFeedUpload::uploadBatch(
    const String& customer,
    const Vector<IndexChangeRequest>& batch) {
  Buffer body;

  for (const auto& job : batch) {
    msg::MessageObject obj;
    obj.addChild(schema_.id("customer"), customer);
    obj.addChild(schema_.id("docid"), job.item.docID().docid);

    for (const auto& attr : job.attrs) {
      auto& attr_obj = obj.addChild(schema_.id("attributes"));
      attr_obj.addChild(schema_.id("attributes.key"), attr.first);
      attr_obj.addChild(schema_.id("attributes.value"), attr.second);
    }

#ifndef FNORD_NOTRACE
    fnord::logTrace(
        "cm.frontend",
        "uploading change index request:\n$0",
        msg::MessagePrinter::print(obj, schema_));
#endif

    msg::MessageEncoder::encode(obj, schema_, &body);
  }

  URI uri(target_url_ + "?table=index_feed-" + customer);

  http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  req.addHeader("Content-Type", "application/fnord-msg");
  req.addBody(body);

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
      fnord::logError("cm.frontend", e, "error in IndexFeedUploader");
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
      fnord::logError("cm.frontend", e, "error in IndexFeedUploader");
    }
  }
}


} // namespace cm

