/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
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
#include "eventql/server/sql/pipelined_expression.h"

namespace eventql {

TableScan::TableScan(
    csql::Transaction* txn,
    csql::ExecutionContext* execution_context,
    const String& tsdb_namespace,
    const String& table_name,
    const Vector<PartitionLocation>& partitions,
    RefPtr<csql::SequentialScanNode> seqscan,
    PartitionMap* partition_map,
    InternalAuth* auth) :
    txn_(txn),
    execution_context_(execution_context),
    tsdb_namespace_(tsdb_namespace),
    table_name_(table_name),
    partitions_(partitions),
    seqscan_(seqscan),
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
    return openLocalPartition(partition.partition_id);
  } else {
    return openRemotePartition(partition.partition_id, partition.servers);
  }
}

ScopedPtr<csql::ResultCursor> TableScan::openLocalPartition(
    const SHA1Hash& partition_key) {
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
              seqscan_));

    case eventql::TBL_STORAGE_STATIC:
      return mkScoped(
          new StaticPartitionCursor(
              txn_,
              &child_execution_context_,
              table.get(),
              partition.get()->getSnapshot(),
              seqscan_));
  }
}

ScopedPtr<csql::ResultCursor> TableScan::openRemotePartition(
    const SHA1Hash& partition_key,
    const Vector<ReplicaRef> servers) {
  auto table_name = StringUtil::format(
      "tsdb://remote/$0/$1",
      URI::urlEncode(table_name_),
      partition_key.toString());

  auto seqscan_copy = seqscan_->template deepCopyAs<csql::SequentialScanNode>();
  seqscan_copy->setTableName(table_name);

  auto remote_expr = mkScoped(
      new PipelinedExpression(
          txn_,
          &child_execution_context_,
          tsdb_namespace_,
          auth_,
          1));

  remote_expr->addRemoteQuery(
      seqscan_copy.get(),
      servers);

  return mkScoped(
      new csql::TableExpressionResultCursor(std::move(remote_expr)));
}

}
