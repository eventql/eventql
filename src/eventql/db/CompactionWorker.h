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
#include <eventql/util/stdtypes.h>
#include <eventql/db/TSDBService.h>
#include <eventql/util/random.h>

#include "eventql/eventql.h"

namespace eventql {

class CompactionWorker {
public:

  CompactionWorker(PartitionMap* pmap, size_t nthreads);
  ~CompactionWorker();

  void enqueuePartition(RefPtr<Partition> partition);

protected:

  void enqueuePartitionWithLock(RefPtr<Partition> partition);

  void start();
  void stop();
  void work();

  PartitionMap* pmap_;
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
