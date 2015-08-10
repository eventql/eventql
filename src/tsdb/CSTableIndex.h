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
#include <tsdb/TSDBService.h>
#include <stx/random.h>

using namespace stx;

namespace tsdb {

class CSTableIndex {
public:

  CSTableIndex(PartitionMap* pmap);
  ~CSTableIndex();

  Option<RefPtr<VFSFile>> fetchCSTable(
      const String& tsdb_namespace,
      const String& table,
      const SHA1Hash& partition) const;

  Option<String> fetchCSTableFilename(
      const String& tsdb_namespace,
      const String& table,
      const SHA1Hash& partition) const;

  void enqueuePartition(RefPtr<Partition> partition);

protected:

  void buildCSTable(RefPtr<Partition> partition);

  bool needsUpdate(RefPtr<PartitionSnapshot> snap) const;

  void start();
  void stop();
  void work();

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
