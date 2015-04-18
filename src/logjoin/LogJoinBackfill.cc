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
    const String& statefile,
    bool dry_run,
    const Vector<URI>& uris,
    http::HTTPConnectionPool* http) :
    table_(table),
    backfill_fn_(backfill_fn),
    statefile_(statefile),
    dry_run_(dry_run),
    target_uris_(uris),
    tail_(nullptr),
    http_(http),
    shutdown_(false),
    inputq_(64000),
    uploadq_(64000),
    num_records_(0) {
  if (FileUtil::exists(statefile_)) {
    auto data = FileUtil::read(statefile_);
    util::BinaryMessageReader reader(data.data(), data.size());
    eventdb::LogTableTailCursor cursor;
    cursor.decode(&reader);
    tail_ = RefPtr<eventdb::LogTableTail>(
        new eventdb::LogTableTail(table_, cursor));
  } else {
    tail_ = RefPtr<eventdb::LogTableTail>(new eventdb::LogTableTail(table_));
  }
}

void LogJoinBackfill::start() {
  fnord::logInfo("cm.logjoin", "Starting LogJoin backfill");

  for (int i = 8; i > 0; --i) {
    threads_.emplace_back([this] {
      while (inputq_.length() > 0 || !shutdown_.load()) {
        if (runWorker() == 0) {
          usleep(10000);
        }
      }
    });
  }

  for (int i = 4; i > 0; --i) {
    threads_.emplace_back([this] {
      while (uploadq_.length() > 0 || !shutdown_.load()) {
        if (runUpload() == 0) {
          usleep(10000);
        }
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

  auto running = tail_->fetchNext(
      [this] (const msg::MessageObject& record) -> bool {
        std::shared_ptr<msg::MessageObject> r(new msg::MessageObject(record));
        inputq_.insert(r, true);
        ++num_records_;
        return true;
      },
      batch_size);

  auto file = File::openFile(
      statefile_ + "~",
      File::O_CREATEOROPEN | File::O_TRUNCATE | File::O_WRITE);

  util::BinaryMessageWriter buf;
  auto cursor = tail_->getCursor();
  cursor.encode(&buf);
  file.write(buf.data(), buf.size());
  FileUtil::mv(statefile_ + "~", statefile_);

  return running;
}

size_t LogJoinBackfill::runWorker() {
  size_t n = 0;

  for (int i = 0; i < 8192; ++i) {
    auto rec = inputq_.poll();
    if (rec.isEmpty()) {
      break;
    }

    ++n;

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

  return n;
}

size_t LogJoinBackfill::runUpload() {
  size_t n = 0;

  Buffer buf;

  for (int i = 0; i < 50; ++i) {
    auto rec = uploadq_.poll();
    if (rec.isEmpty()) {
      break;
    }

    buf.append(rec.get().data(), rec.get().size());
    ++n;
  }

  if (n == 0) {
    return 0;
  }

  for (auto delay = kMicrosPerSecond / 10;
      true;
      usleep((delay = std::min(delay * 2, kMicrosPerSecond * 30)))) {
    try {
      auto uri = target_uris_[rnd_.random64() % target_uris_.size()];
      http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
      req.addHeader("Host", uri.hostAndPort());
      req.addHeader("Content-Type", "application/fnord-msg");
      req.addBody(buf);

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

  return n;
}

} // namespace cm

