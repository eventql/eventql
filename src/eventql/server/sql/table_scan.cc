/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include "eventql/server/sql/table_scan.h"
#include "eventql/server/sql/partition_cursor.h"

namespace eventql {

TableScan::TableScan(
    csql::Transaction* txn,
    csql::ExecutionContext* execution_context,
    const String& tsdb_namespace,
    const String& table_name,
    const Vector<PartitionLocation>& partitions,
    RefPtr<csql::SequentialScanNode> seqscan,
    Option<SHA1Hash> cache_key,
    PartitionMap* partition_map,
    InternalAuth* auth) :
    txn_(txn),
    execution_context_(execution_context),
    tsdb_namespace_(tsdb_namespace),
    table_name_(table_name),
    partitions_(partitions),
    seqscan_(seqscan),
    cache_key_(cache_key),
    partition_map_(partition_map),
    auth_(auth),
    cur_partition_(0) {
  execution_context_->incrementNumTasks(partitions_.size());
}

ScopedPtr<csql::ResultCursor> TableScan::execute() {
  return mkScoped(
      new csql::DefaultResultCursor(
          seqscan_->getNumComputedColumns(),
          std::bind(
              &TableScan::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
};

size_t TableScan::getNumColumns() const {
  return seqscan_->getNumComputedColumns();
}

bool TableScan::next(csql::SValue* row, size_t row_len) {
  while (cur_partition_ < partitions_.size()) {
    if (cur_cursor_.get() == nullptr) {
      cur_cursor_ = openPartition(partitions_[cur_partition_]);
      execution_context_->incrementNumTasksRunning();
    }

    if (cur_cursor_.get() && cur_cursor_->next(row, row_len)) {
      return true;
    } else {
      cur_cursor_.reset(nullptr);
      ++cur_partition_;
      execution_context_->incrementNumTasksCompleted();
    }
  }

  return false;
}

ScopedPtr<csql::ResultCursor> TableScan::openPartition(
    const PartitionLocation& partition) {
  if (partition.servers.empty()) {
    return openLocalPartition(partition.partition_id, partition.qtree);
  } else {
    return openRemotePartition(
        partition.partition_id,
        partition.qtree,
        partition.servers);
  }
}

ScopedPtr<csql::ResultCursor> TableScan::openLocalPartition(
    const SHA1Hash& partition_key,
    RefPtr<csql::SequentialScanNode> qtree) {
  auto partition =  partition_map_->findPartition(
      tsdb_namespace_,
      table_name_,
      partition_key);

  auto table = partition_map_->findTable(tsdb_namespace_, table_name_);
  if (table.isEmpty()) {
    RAISEF(kNotFoundError, "table not found: $0/$1", tsdb_namespace_, table_name_);
  }

  if (partition.isEmpty()) {
    return ScopedPtr<csql::ResultCursor>();
  }

  switch (partition.get()->getTable()->storage()) {
    case eventql::TBL_STORAGE_COLSM:
      return mkScoped(
          new PartitionCursor(
              txn_,
              &child_execution_context_,
              table.get(),
              partition.get()->getSnapshot(),
              qtree));

    case eventql::TBL_STORAGE_STATIC:
      return mkScoped(
          new StaticPartitionCursor(
              txn_,
              &child_execution_context_,
              table.get(),
              partition.get()->getSnapshot(),
              qtree));
  }
}

ScopedPtr<csql::ResultCursor> TableScan::openRemotePartition(
    const SHA1Hash& partition_key,
    RefPtr<csql::SequentialScanNode> qtree,
    const Vector<ReplicaRef> servers) {
  auto table_name = StringUtil::format(
      "tsdb://remote/$0/$1",
      URI::urlEncode(table_name_),
      partition_key.toString());

  auto seqscan_copy = qtree->template deepCopyAs<csql::SequentialScanNode>();
  seqscan_copy->setTableName(table_name);

  std::vector<std::string> server_names;
  for (const auto& s : servers) {
    server_names.emplace_back(s.name);
  }

  return mkScoped(
      new RemotePartitionCursor(
          static_cast<Session*>(txn_->getUserData()),
          txn_,
          &child_execution_context_,
          tsdb_namespace_,
          seqscan_copy.get(),
          server_names));
}

Option<SHA1Hash> TableScan::getCacheKey() const {
  return cache_key_;
}

ReplicaRef::ReplicaRef(
    SHA1Hash _unique_id,
    String _addr) :
    unique_id(_unique_id),
    addr(_addr),
    is_local(false) {}

}
