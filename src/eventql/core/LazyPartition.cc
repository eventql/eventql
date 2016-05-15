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
#include <eventql/core/LazyPartition.h>
#include <eventql/core/PartitionMap.h>

using namespace util;

namespace eventql {

LazyPartition::LazyPartition() {
  z1stats()->num_partitions.incr(1);
}

LazyPartition::LazyPartition(
    RefPtr<Partition> partition) :
    partition_(partition) {
  z1stats()->num_partitions.incr(1);
}

LazyPartition::~LazyPartition() {
  z1stats()->num_partitions.decr(1);
}

RefPtr<Partition> LazyPartition::getPartition(
    const String& tsdb_namespace,
    RefPtr<Table> table,
    const SHA1Hash& partition_key,
    ServerConfig* cfg,
    PartitionMap* pmap) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (partition_.get() != nullptr) {
    auto partition = partition_;
    return partition;
  }

  partition_ = Partition::reopen(
      tsdb_namespace,
      table,
      partition_key,
      cfg);

  auto partition = partition_;
  lk.unlock();

  auto change = mkRef(new PartitionChangeNotification());
  change->partition = partition;
  pmap->publishPartitionChange(change);

  return partition;
}

RefPtr<Partition> LazyPartition::getPartition() {
  std::unique_lock<std::mutex> lk(mutex_);
  if (partition_.get() == nullptr) {
    RAISE(kRuntimeError, "partition not loaded");
  }

  auto partition = partition_;
  return partition;
}


bool LazyPartition::isLoaded() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return partition_.get() != nullptr;
}



}
