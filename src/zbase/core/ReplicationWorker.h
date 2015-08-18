/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <thread>
#include <condition_variable>
#include <stx/stdtypes.h>
#include <zbase/core/PartitionMap.h>
#include <zbase/core/PartitionReplication.h>
#include <zbase/core/ReplicationScheme.h>

using namespace stx;

namespace tsdb {

class ReplicationWorker {
public:
  static const uint64_t kReplicationCorkWindowMicros = 10 * kMicrosPerSecond;

  ReplicationWorker(
      RefPtr<ReplicationScheme> repl_scheme,
      PartitionMap* pmap,
      http::HTTPConnectionPool* http);

  ~ReplicationWorker();

  void enqueuePartition(RefPtr<Partition> partition);

  void updateReplicationScheme(RefPtr<ReplicationScheme> new_repl_scheme);

protected:

  void start();
  void stop();
  void work();

  RefPtr<ReplicationScheme> repl_scheme_;
  http::HTTPConnectionPool* http_;

  Set<SHA1Hash> waitset_;
  std::multiset<
      Pair<uint64_t, RefPtr<Partition>>,
      Function<bool (
          const Pair<uint64_t, RefPtr<Partition>>&,
          const Pair<uint64_t, RefPtr<Partition>>&)>> queue_;

  std::atomic<bool> running_;
  Vector<std::thread> threads_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;
};

} // namespace tsdb

