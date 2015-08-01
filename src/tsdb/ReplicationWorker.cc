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
#include <stx/logging.h>
#include <stx/wallclock.h>
#include <tsdb/ReplicationWorker.h>
#include <tsdb/Partition.h>

using namespace stx;

namespace tsdb {

ReplicationWorker::ReplicationWorker() :
    queue_([] (
        const Pair<uint64_t, RefPtr<Partition>>& a,
        const Pair<uint64_t, RefPtr<Partition>>& b) {
      return a.first < b.first;
    }),
    running_(false) {
  start();
}

ReplicationWorker::~ReplicationWorker() {
  stop();
}

void ReplicationWorker::start() {
  running_ = true;

  for (int i = 0; i < 4; ++i) {
    threads_.emplace_back(std::bind(&ReplicationWorker::work, this));
  }
}

void ReplicationWorker::stop() {
  if (!running_) {
    return;
  }

  running_ = false;
  cv_.notify_all();

  for (auto& t : threads_) {
    t.join();
  }
}

void ReplicationWorker::work() {
  std::unique_lock<std::mutex> lk(mutex_);

  while (running_) {
    if (queue_.size() == 0) {
      cv_.wait(lk);
    }

    if (queue_.size() == 0) {
      continue;
    }

    auto now = WallClock::unixMicros();
    if (now < queue_.begin()->first) {
      cv_.wait_for(
          lk,
          std::chrono::microseconds(queue_.begin()->first - now));

      continue;
    }

    auto partition = queue_.begin()->second;
    queue_.erase(queue_.begin());
    lk.unlock();

    try {
      replicate(partition);
    } catch (const StandardException& e) {
      logError("tsdb", e, "ReplicationWorker error");

      lk.lock();
      auto delay = 30 * kMicrosPerSecond; // FIXPAUL increasing delay..
      queue_.emplace(now + delay, partition);
      continue;
    }

    lk.lock();
    waitset_.erase(partition->uuid());
    maybeEnqueuePartition(partition);
  }
}

} // namespace tsdb
