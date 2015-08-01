/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <tsdb/LazyPartition.h>
#include <tsdb/PartitionMap.h>

using namespace stx;

namespace tsdb {

LazyPartition::LazyPartition() {}

LazyPartition::LazyPartition(
    RefPtr<Partition> partition) :
    partition_(partition) {}

RefPtr<Partition> LazyPartition::getPartition(
    const String& tsdb_namespace,
    RefPtr<Table> table,
    const SHA1Hash& partition_key,
    const String& db_path,
    PartitionMap* pmap) {
  if (partition_.get() != nullptr) {
    return partition_;
  }

  std::unique_lock<std::mutex> lk(mutex_);
  if (partition_.get() != nullptr) {
    return partition_;
  }

  partition_ = Partition::reopen(
      tsdb_namespace,
      table,
      partition_key,
      db_path);

  auto change = mkRef(new PartitionChangeNotification());
  change->partition = partition_;
  pmap->publishPartitionChange(change);

  return partition_;
}

RefPtr<Partition> LazyPartition::getPartition() {
  if (partition_.get() == nullptr) {
    RAISE(kRuntimeError, "partition not loaded");
  }

  return partition_;
}


bool LazyPartition::isLoaded() const {
  return partition_.get() != nullptr;
}



}
