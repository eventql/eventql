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
#include <fnord/logging.h>
#include <fnord/wallclock.h>
#include <tsdb/CompactionWorker.h>
#include <tsdb/Partition.h>

using namespace fnord;

namespace tsdb {

CompactionWorker::CompactionWorker(
    TSDBNodeRef* node) :
    queue_(&node->compactionq),
    running_(true) {}

void CompactionWorker::start() {
  running_ = true;
  thread_ = std::thread(std::bind(&CompactionWorker::run, this));
}

void CompactionWorker::stop() {
  if (!running_) {
    return;
  }

  running_ = false;
  queue_->wakeup();
  thread_.join();
}

void CompactionWorker::run() {
  while (running_.load()) {
    auto job = queue_->interruptiblePop();
    if (job.isEmpty()) {
      continue;
    }

    try {
      job.get()->compact();
    } catch (const std::exception& e) {
      fnord::logError("fnord.evdb", e, "CompactionWorker error");
    }
  }
}

} // namespace tsdb

