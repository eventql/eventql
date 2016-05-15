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
#include <eventql/sql/expressions/table_expression.h>
#include <eventql/AnalyticsAuth.h>
#include <eventql/core/PartitionMap.h>

using namespace util;

namespace eventql {

class TableScan : public csql::TableExpression {
public:

  TableScan(
    csql::Transaction* txn,
    const String& tsdb_namespace,
    const String& table_name,
    const Vector<SHA1Hash>& partitions,
    RefPtr<csql::SequentialScanNode> seqscan,
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    AnalyticsAuth* auth);

  ScopedPtr<csql::ResultCursor> execute() override;

protected:

  bool next(csql::SValue* row, size_t row_len);

  ScopedPtr<csql::ResultCursor> openPartition(const SHA1Hash& partition_id);

  ScopedPtr<csql::ResultCursor> openLocalPartition(const SHA1Hash& partition_id);

  ScopedPtr<csql::ResultCursor> openRemotePartition(const SHA1Hash& partition_id);

  csql::Transaction* txn_;
  String tsdb_namespace_;
  String table_name_;
  Vector<SHA1Hash> partitions_;
  RefPtr<csql::SequentialScanNode> seqscan_;
  PartitionMap* partition_map_;
  ReplicationScheme* replication_scheme_;
  AnalyticsAuth* auth_;
  ScopedPtr<csql::ResultCursor> cur_cursor_;
  size_t cur_partition_;
};

}
