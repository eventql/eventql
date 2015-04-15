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
    running_(true) {}

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

void ArtifactReplication::downloadPending() {
  fnord::iputs("ArtifactReplication::downloadPending", 1);
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

