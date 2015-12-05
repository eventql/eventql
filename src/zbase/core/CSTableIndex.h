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
#include <stx/stdtypes.h>
#include <zbase/core/TSDBService.h>
#include <stx/random.h>

using namespace stx;

namespace zbase {

class CSTableIndex {
public:

  CSTableIndex(PartitionMap* pmap, size_t nthreads);
  ~CSTableIndex();

  void enqueuePartition(RefPtr<Partition> partition);

protected:

  void enqueuePartitionWithLock(RefPtr<Partition> partition);

  void start();
  void stop();
  void work();

  size_t nthreads_;
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

}
