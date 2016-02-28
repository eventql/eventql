/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/stdtypes.h>
#include <stx/logging.h>
#include <stx/io/fileutil.h>
#include <stx/io/mmappedfile.h>
#include <stx/protobuf/msg.h>
#include <stx/wallclock.h>
#include <stx/application.h>
#include <zbase/core/CompactionWorker.h>
#include <zbase/core/LogPartitionCompactionState.pb.h>
#include <zbase/core/RecordSet.h>
#include <zbase/core/LogPartitionReader.h>
#include <stx/protobuf/MessageDecoder.h>
#include <cstable/RecordShredder.h>
#include <cstable/CSTableWriter.h>

using namespace stx;

namespace zbase {

CompactionWorker::CompactionWorker(
    PartitionMap* pmap,
    size_t nthreads) :
    pmap_(pmap),
    nthreads_(nthreads),
    queue_([] (
        const Pair<uint64_t, RefPtr<Partition>>& a,
        const Pair<uint64_t, RefPtr<Partition>>& b) {
      return a.first < b.first;
    }),
    running_(false) {
  pmap->subscribeToPartitionChanges([this] (
      RefPtr<zbase::PartitionChangeNotification> change) {
    enqueuePartition(change->partition);
  });

  start();
}

CompactionWorker::~CompactionWorker() {
  stop();
}

void CompactionWorker::enqueuePartition(RefPtr<Partition> partition) {
  std::unique_lock<std::mutex> lk(mutex_);
  enqueuePartitionWithLock(partition);
}

void CompactionWorker::enqueuePartitionWithLock(RefPtr<Partition> partition) {
  auto interval = partition->getTable()->commitInterval();

  auto uuid = partition->uuid();
  if (waitset_.count(uuid) > 0) {
    return;
  }

  queue_.emplace(
      WallClock::unixMicros() + interval.microseconds(),
      partition);

  waitset_.emplace(uuid);
  cv_.notify_all();
  z1stats()->compaction_queue_length.set(queue_.size());
}

void CompactionWorker::start() {
  running_ = true;

  for (int i = 0; i < nthreads_; ++i) {
    threads_.emplace_back(std::bind(&CompactionWorker::work, this));
  }
}

void CompactionWorker::stop() {
  if (!running_) {
    return;
  }

  running_ = false;
  cv_.notify_all();

  for (auto& t : threads_) {
    t.join();
  }
}

void CompactionWorker::work() {
  Application::setCurrentThreadName("z1d-compaction");

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

    bool success = true;
    {
      lk.unlock();

      try {
        auto writer = partition->getWriter();
        if (writer->compact()) {
          auto change = mkRef(new PartitionChangeNotification());
          change->partition = partition;
          pmap_->publishPartitionChange(change);
        }
      } catch (const StandardException& e) {
        logError("tsdb", e, "CompactionWorker error");
        success = false;
      }

      lk.lock();
    }

    if (success) {
      waitset_.erase(partition->uuid());

      if (partition->getWriter()->needsCompaction()) {
        enqueuePartitionWithLock(partition);
      }
    } else {
      auto delay = 30 * kMicrosPerSecond; // FIXPAUL increasing delay..
      queue_.emplace(now + delay, partition);
    }

    z1stats()->compaction_queue_length.set(queue_.size());
  }
}

} // namespace zbase
