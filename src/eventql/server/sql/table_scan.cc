/**
 * Copyright (c) 2015 - zScale Technology GmbH <legal@zscale.io>
 *   All Rights Reserved.
 *
 * Authors:
 *   Paul Asmuth <paul@zscale.io>
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "eventql/server/sql/table_scan.h"
#include "eventql/server/sql/partition_cursor.h"
#include "eventql/server/sql/remote_expression.h"

namespace eventql {

TableScan::TableScan(
    csql::Transaction* txn,
    const String& tsdb_namespace,
    const String& table_name,
    const Vector<SHA1Hash>& partitions,
    RefPtr<csql::SequentialScanNode> seqscan,
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    AnalyticsAuth* auth) :
    txn_(txn),
    tsdb_namespace_(tsdb_namespace),
    table_name_(table_name),
    partitions_(partitions),
    seqscan_(seqscan),
    partition_map_(partition_map),
    replication_scheme_(replication_scheme),
    auth_(auth),
    cur_partition_(0) {}


ScopedPtr<csql::ResultCursor> TableScan::execute() {
  return mkScoped(
      new csql::DefaultResultCursor(
          seqscan_->numColumns(),
          std::bind(
              &TableScan::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
};

bool TableScan::next(csql::SValue* row, size_t row_len) {
  while (cur_partition_ < partitions_.size()) {
    if (cur_cursor_.get() == nullptr) {
      cur_cursor_ = openPartition(partitions_[cur_partition_]);
    }

    if (cur_cursor_.get() && cur_cursor_->next(row, row_len)) {
      return true;
    } else {
      cur_cursor_.reset(nullptr);
      ++cur_partition_;
    }
  }

  return false;
}

ScopedPtr<csql::ResultCursor> TableScan::openPartition(
    const SHA1Hash& partition_key) {
  if (replication_scheme_->hasLocalReplica(partition_key)) {
    return openLocalPartition(partition_key);
  } else {
    return openRemotePartition(partition_key);
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

  return mkScoped(
      new PartitionCursor(
          txn_,
          table.get(),
          partition.get()->getSnapshot(),
          seqscan_));
}

ScopedPtr<csql::ResultCursor> TableScan::openRemotePartition(
    const SHA1Hash& partition_key) {
  auto table_name = StringUtil::format(
      "tsdb://remote/$0/$1",
      URI::urlEncode(table_name_),
      partition_key.toString());

  auto seqscan_copy = seqscan_->template deepCopyAs<csql::SequentialScanNode>();
  seqscan_copy->setTableName(table_name);

  return mkScoped(
      new csql::TableExpressionResultCursor(
          mkScoped(
              new RemoteExpression(
                  txn_,
                  tsdb_namespace_,
                  seqscan_copy.get(),
                  replication_scheme_->replicasFor(partition_key),
                  auth_))));
}

}
