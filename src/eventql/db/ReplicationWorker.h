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
#pragma once
#include <thread>
#include <condition_variable>
#include <eventql/util/stdtypes.h>
#include <eventql/db/partition_map.h>
#include <eventql/db/PartitionReplication.h>
#include <eventql/db/ReplicationScheme.h>

#include "eventql/eventql.h"

namespace eventql {

class ReplicationInfo {
public:

  ReplicationInfo();
  void reset();
  void setPartition(String name);
  void setTargetHost(String host_name);
  void setTargetHostStatus(size_t bytes_sent, size_t records_sent);

  String toString() const;

protected:
  mutable std::mutex mutex_;
  bool is_idle_;
  String cur_partition_;
  String cur_target_host_;
  UnixTime cur_partition_since_;
  UnixTime cur_target_host_since_;
  uint64_t cur_target_host_bytes_sent_;
  uint64_t cur_target_host_records_sent_;
};

class ReplicationWorker {
public:

  enum class ReplicationOptions : uint64_t {
    CORK = 1,
    URGENT = 2
  };

  static const uint64_t kReplicationCorkWindowMicros = 10 * kMicrosPerSecond;

  ReplicationWorker(
      RefPtr<ReplicationScheme> repl_scheme,
      PartitionMap* pmap,
      http::HTTPConnectionPool* http);

  ~ReplicationWorker();

  void enqueuePartition(
      RefPtr<Partition> partition,
      uint64_t flags = (uint64_t) ReplicationOptions::CORK);

  size_t getNumThreads() const;

  const ReplicationInfo* getReplicationInfo(size_t thread_id) const;

protected:

  void enqueuePartitionWithLock(
      RefPtr<Partition> partition,
      uint64_t flags);

  void start();
  void stop();
  void work(size_t thread_id);

  RefPtr<ReplicationScheme> repl_scheme_;
  PartitionMap* pmap_;
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
  size_t num_replication_threads_;
  Vector<ReplicationInfo> replication_infos_;
};

} // namespace eventql

