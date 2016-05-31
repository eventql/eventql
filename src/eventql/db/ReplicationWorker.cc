/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include "unistd.h"
#include <eventql/util/logging.h>
#include <eventql/util/wallclock.h>
#include <eventql/util/application.h>
#include <eventql/db/ReplicationWorker.h>
#include <eventql/db/Partition.h>
#include <eventql/server/server_stats.h>

#include "eventql/eventql.h"

namespace eventql {

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
      RefPtr<eventql::PartitionChangeNotification> change) {
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

void ReplicationWorker::enqueuePartition(
    RefPtr<Partition> partition,
    uint64_t delay_usecs) {
  std::unique_lock<std::mutex> lk(mutex_);
  enqueuePartitionWithLock(partition);
}

void ReplicationWorker::enqueuePartitionWithLock(
    RefPtr<Partition> partition,
    uint64_t delay_usecs /* = 0 */) {
  auto uuid = partition->uuid();
  if (waitset_.count(uuid) > 0) {
    return;
  }

  queue_.emplace(
      WallClock::unixMicros() + kReplicationCorkWindowMicros + delay_usecs,
      partition);

  z1stats()->replication_queue_length.set(queue_.size());

  waitset_.emplace(uuid);
  cv_.notify_all();
}

void ReplicationWorker::start() {
  running_ = true;

  for (int i = 0; i < 8; ++i) {
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
  Application::setCurrentThreadName("z1d-replication");

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

    z1stats()->replication_queue_length.set(queue_.size());
  }
}

} // namespace eventql
