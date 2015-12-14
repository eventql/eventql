/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <zbase/core/LazyPartition.h>
#include <zbase/core/PartitionMap.h>

using namespace stx;

namespace zbase {

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
