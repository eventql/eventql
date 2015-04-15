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
#include <fnord-eventdb/ArtifactReplication.h>

namespace fnord {
namespace eventdb {

ArtifactReplication::ArtifactReplication(
    ArtifactIndex* index,
    http::HTTPConnectionPool* http) :
    index_(index),
    http_(http),
    interval_(1 * kMicrosPerSecond),
    cur_requests_(0),
    max_concurrent_reqs_(4),
    running_(true) {}

void ArtifactReplication::downloadPending() {
  auto artifacts = index_->listArtifacts();

  for (const auto& a : artifacts) {
    if (a.status == ArtifactStatus::DOWNLOAD) {
      enqueueArtifact(a);
    }
  }
}

void ArtifactReplication::downloadArtifact(const ArtifactRef& artifact) {
  fnord::logInfo(
      "fnord.evdb",
      "Downloading artifact: $0",
      artifact.name);

  usleep(1000000);
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
  while (cur_requests_ >= max_concurrent_reqs_) {
    cv_.wait(lk);
  }

  ++cur_requests_;
  auto thread = std::thread([this, artifact] () {
    downloadArtifact(artifact);
    std::unique_lock<std::mutex> wakeup_lk(mutex_);
    --cur_requests_;
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

