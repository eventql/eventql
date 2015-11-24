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
#include <zbase/core/ReplicationWorker.h>
#include <zbase/core/Partition.h>
#include <zbase/z1stats.h>

using namespace stx;

namespace zbase {

ReplicationWorker::ReplicationWorker(
    RefPtr<ReplicationScheme> repl_scheme,
    PartitionMap* pmap,
    http::HTTPConnectionPool* http) :
    repl_scheme_(repl_scheme),
    pmap_(pmap),
    http_(http),
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

ReplicationWorker::~ReplicationWorker() {
  stop();
}

void ReplicationWorker::enqueuePartition(RefPtr<Partition> partition) {
  std::unique_lock<std::mutex> lk(mutex_);
  enqueuePartitionWithLock(partition);
}

void ReplicationWorker::enqueuePartitionWithLock(RefPtr<Partition> partition) {
  auto uuid = partition->uuid();
  if (waitset_.count(uuid) > 0) {
    return;
  }

  queue_.emplace(
      WallClock::unixMicros() + kReplicationCorkWindowMicros,
      partition);

  z1stats()->replication_queue_length.incr(1);

  waitset_.emplace(uuid);
  cv_.notify_all();
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
    auto repl_scheme = repl_scheme_;

    RefPtr<PartitionReplication> repl;
    bool success = true;
    {
      lk.unlock();

      try {
        repl = partition->getReplicationStrategy(repl_scheme, http_);
        success = repl->replicate();
      } catch (const StandardException& e) {
        logError("tsdb", e, "ReplicationWorker error");
        success = false;
      }

      lk.lock();
    }

    if (success) {
      z1stats()->replication_queue_length.decr(1);
      waitset_.erase(partition->uuid());

      repl = partition->getReplicationStrategy(repl_scheme, http_);
      if (repl->needsReplication()) {
        enqueuePartitionWithLock(partition);
      } else {
        auto snap = partition->getSnapshot();
        auto full_copies = repl->numFullRemoteCopies();
        if (!repl_scheme->hasLocalReplica(snap->key) &&
            full_copies >= repl_scheme->minNumCopies()) {
          auto dropped =
              pmap_->dropLocalPartition(
                  snap->state.tsdb_namespace(),
                  snap->state.table_key(),
                  snap->key);

          if (!dropped) {
            enqueuePartitionWithLock(partition);
          }
        }
      }
    } else {
      auto delay = 30 * kMicrosPerSecond; // FIXPAUL increasing delay..
      queue_.emplace(now + delay, partition);
    }
  }
}

} // namespace zbase
