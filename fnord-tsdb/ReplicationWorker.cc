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
#include <fnord-tsdb/ReplicationWorker.h>
#include <fnord-tsdb/StreamChunk.h>

namespace fnord {
namespace tsdb {

ReplicationWorker::ReplicationWorker(
    TSDBNodeRef* node) :
    queue_(&node->replicationq),
    running_(true) {}

void ReplicationWorker::start() {
  running_ = true;
  thread_ = std::thread(std::bind(&ReplicationWorker::run, this));
}

void ReplicationWorker::stop() {
  if (!running_) {
    return;
  }

  running_ = false;
  queue_->wakeup();
  thread_.join();
}

void ReplicationWorker::run() {
  while (running_.load()) {
    auto job = queue_->interruptiblePop();
    if (job.isEmpty()) {
      continue;
    }

    try {
      job.get()->replicate();
    } catch (const std::exception& e) {
      fnord::logError("fnord.evdb", e, "ReplicationWorker error");
    }
  }
}

} // namespace tsdb
} // namespace fnord
