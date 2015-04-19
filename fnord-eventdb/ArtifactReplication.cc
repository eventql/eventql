/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "unistd.h"
#include <fnord-base/logging.h>
#include <fnord-base/wallclock.h>
#include <fnord-http/HTTPFileDownload.h>
#include <fnord-eventdb/ArtifactReplication.h>

namespace fnord {
namespace eventdb {

ArtifactReplication::ArtifactReplication(
    ArtifactIndex* index,
    http::HTTPConnectionPool* http,
    size_t max_concurrent_reqs) :
    index_(index),
    http_(http),
    interval_(1 * kMicrosPerSecond),
    max_concurrent_reqs_(max_concurrent_reqs),
    running_(true),
    rr_(0) {}

void ArtifactReplication::addSource(const URI& source) {
  sources_.emplace_back(source);
}

void ArtifactReplication::downloadPending() {
  auto artifacts = index_->listArtifacts();

  for (const auto& a : artifacts) {
    if (a.status == ArtifactStatus::DOWNLOAD ||
        a.status == ArtifactStatus::MISSING) {
      enqueueArtifact(a);
    }
  }
}

void ArtifactReplication::downloadArtifact(const ArtifactRef& artifact) {
  fnord::logInfo(
      "fnord.evdb",
      "Downloading artifact: $0",
      artifact.name);

  for (const auto& f : artifact.files) {
    if (f.size == 0) {
      fnord::logWarning(
          "fn.evdb",
          "Ignoring empty artifact file '$0'",
          f.filename);

      continue;
    }

    bool retrieved = false;
    auto s = ++rr_;
    for (int i = 0; i < sources_.size(); ++i) {
      auto uri = sources_[(s + i) % sources_.size()];
      uri.setPath(FileUtil::joinPaths(uri.path(), f.filename));
      http::HTTPRequest req(http::HTTPMessage::M_HEAD, uri.path());
      req.addHeader("Host", uri.hostAndPort());

      auto res = http_->executeRequest(req);
      res.wait();

      const auto& r = res.get();
      if (r.statusCode() != 200) {
        continue;
      }

      size_t content_len = 0;
      try {
        content_len = std::stoul(r.getHeader("Content-Length"));
      } catch (...) {}

      if (content_len != f.size) {
        fnord::logWarning(
            "fn.evdb",
            "Ignoring remote artifact '$0' because it has the wrong size "\
            "($2 vs $3)",
            uri.toString(),
            content_len,
            f.size);

        continue;
      }

      try {
        downloadFile(f, uri);
        retrieved = true;
      } catch (const Exception& e) {
        fnord::logError("fn.evdb", e, "download error");
        continue;
      }

      break;
    }

    if (!retrieved) {
      fnord::logWarning(
          "fn.evdb",
          "No source found for remote artifact file '$0'",
          f.filename);

      try {
        index_->updateStatus(artifact.name, ArtifactStatus::MISSING);
      } catch (const Exception& e) {
        /* see comment below */
      }

      return;
    }
  }

  try {
    index_->updateStatus(artifact.name, ArtifactStatus::PRESENT);
  } catch (const Exception& e) {
    /* N.B. artifact may have been deleted (by a superseeding snapshot/merge)
       before we got it, so we ignore "artifact not found" errors */
  }
}


void ArtifactReplication::downloadFile(
    const ArtifactFileRef& file,
    const URI& uri) {
  auto target_file = FileUtil::joinPaths(index_->basePath(), file.filename);

  if (FileUtil::exists(target_file)) {
    return;
  }

  if (FileUtil::exists(target_file + "~")) {
    fnord::logWarning(
        "fn.evdb",
        "Deleting orphaned temp file '$0'",
        target_file + "~");

    FileUtil::rm(target_file + "~");
  }

  http::HTTPRequest req(http::HTTPMessage::M_GET, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  http::HTTPFileDownload download(req, target_file + "~");
  auto res = download.download(http_);
  res.wait();

  if (res.get().statusCode() != 200) {
    FileUtil::rm(target_file + "~");
    RAISE(kRuntimeError, "received non-200 http reponse");
  }

  auto size = FileUtil::size(target_file + "~");
  if (size != file.size) {
    FileUtil::rm(target_file + "~");

    RAISEF(
        kRuntimeError,
        "size mismatch for file: '$0' - $1 vs $2",
        uri.toString(),
        size,
        file.size);
  }

  auto checksum = FileUtil::checksum(target_file + "~");
  if (checksum != file.checksum) {
    FileUtil::rm(target_file + "~");

    RAISEF(
        kRuntimeError,
        "checksum mismatch for file: '$0' - $1 vs $2",
        uri.toString(),
        checksum,
        file.checksum);
  }

  FileUtil::mv(target_file + "~", target_file);
}

void ArtifactReplication::start() {
  running_ = true;
  thread_ = std::thread(std::bind(&ArtifactReplication::run, this));
}

void ArtifactReplication::stop() {
  if (!running_) {
    return;
  }

  running_ = false;
  thread_.join();
}

void ArtifactReplication::enqueueArtifact(const ArtifactRef& artifact) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (cur_downloads_.count(artifact.name) > 0) {
    return;
  }

  while (cur_downloads_.size() >= max_concurrent_reqs_) {
    cv_.wait(lk);
  }

  cur_downloads_.emplace(artifact.name);

  auto thread = std::thread([this, artifact] () {
    try {
      downloadArtifact(artifact);
    } catch (const Exception& e) {
      fnord::logError("fnord.evdb", e, "download error");
    }

    std::unique_lock<std::mutex> wakeup_lk(mutex_);
    cur_downloads_.erase(artifact.name);
    wakeup_lk.unlock();
    cv_.notify_all();
  });

  thread.detach();
}

void ArtifactReplication::run() {
  while (running_.load()) {
    auto begin = WallClock::unixMicros();

    try {
      downloadPending();
    } catch (const Exception& e) {
      fnord::logError("fnord.evdb", e, "ArtifactReplication error");
    }

    auto elapsed = WallClock::unixMicros() - begin;
    if (elapsed < interval_) {
      usleep(interval_ - elapsed);
    }
  }
}

} // namespace eventdb
} // namespace fnord

