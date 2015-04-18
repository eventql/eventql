/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "unistd.h"
#include "logjoin/LogJoinBackfill.h"

using namespace fnord;

namespace cm {

LogJoinBackfill::LogJoinBackfill(
    RefPtr<eventdb::TableReader> table,
    BackfillFnType backfill_fn,
    const URI& uri,
    http::HTTPConnectionPool* http) :
    table_(table),
    tail_(table),
    backfill_fn_(backfill_fn),
    target_uri_(uri),
    http_(http),
    shutdown_(false),
    inputq_(64000),
    uploadq_(64000),
    num_records_(0) {}

void LogJoinBackfill::start() {
  fnord::logInfo("cm.logjoin", "Starting LogJoin backfill");

  for (int i = 8; i > 0; --i) {
    threads_.emplace_back([this] {
      while (inputq_.length() > 0 || !shutdown_.load()) {
        runWorker();
        usleep(10000);
      }
    });
  }

  for (int i = 2; i > 0; --i) {
    threads_.emplace_back([this] {
      while (uploadq_.length() > 0 || !shutdown_.load()) {
        runUpload();
        usleep(10000);
      }
    });
  }
}

void LogJoinBackfill::shutdown() {
  while (inputq_.length() > 0 || uploadq_.length() > 0) {
    fnord::logInfo(
        "cm.logjoin",
        "Waiting for $0 jobs to finish...",
        inputq_.length() + uploadq_.length());

    usleep(1000000);
  }

  shutdown_ = true;

  fnord::logInfo("cm.logjoin", "Waiting for jobs to finish...");
  for (auto& t : threads_) {
    t.join();
  }

  fnord::logInfo("cm.logjoin", "LogJoin backfill finished succesfully :)");
}

bool LogJoinBackfill::process(size_t batch_size) {
  fnord::logInfo(
      "cm.logjoin",
      "LogJoin backfill comitting; num_records=$0 inputq=$1 uploadq=$2",
      num_records_,
      inputq_.length(),
      uploadq_.length());

  auto running = tail_.fetchNext(
      [this] (const msg::MessageObject& record) -> bool {
        std::shared_ptr<msg::MessageObject> r(new msg::MessageObject(record));
        inputq_.insert(r, true);
        ++num_records_;
        return true;
      },
      batch_size);

  return running;
}

void LogJoinBackfill::runWorker() {
  for (int i = 0; i < 8192; ++i) {
    auto rec = inputq_.poll();
    if (rec.isEmpty()) {
      return;
    }

    try {
      backfill_fn_(rec.get().get());
    } catch (const Exception& e) {
      fnord::logError("cm.logjoin", e, "backfill error");
    }

    if (!dry_run_) {
      Buffer msg_buf;
      msg::MessageEncoder::encode(*rec.get(), table_->schema(), &msg_buf);

      uploadq_.insert(msg_buf, true);
    }
  }
}

void LogJoinBackfill::runUpload() {
  for (int i = 0; i < 8192; ++i) {
    auto rec = uploadq_.poll();
    if (rec.isEmpty()) {
      return;
    }

    for (auto delay = kMicrosPerSecond / 10;
        true;
        usleep((delay = std::min(delay * 2, kMicrosPerSecond * 30)))) {
      try {
        auto uri = target_uri_;
        http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
        req.addHeader("Host", uri.hostAndPort());
        req.addHeader("Content-Type", "application/fnord-msg");
        req.addBody(rec.get());

        auto res = http_->executeRequest(req);
        res.wait();

        const auto& r = res.get();
        if (r.statusCode() != 201) {
          RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
        }

        break;
      } catch (const Exception& e) {
        fnord::logError("cm.logjoin", e, "upload error");
      }
    }
  }
}

} // namespace cm

