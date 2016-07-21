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
#include <eventql/sql/expressions/table_expression.h>
#include <eventql/auth/internal_auth.h>
#include <eventql/db/partition_map.h>

#include "eventql/eventql.h"

namespace eventql {

class TableScan : public csql::TableExpression {
public:

  struct PartitionLocation {
    SHA1Hash partition_id;
    Vector<ReplicaRef> servers;
    Option<SHA1Hash> cache_key;
    RefPtr<csql::SequentialScanNode> qtree;
  };

  TableScan(
    csql::Transaction* txn,
    csql::ExecutionContext* execution_context,
    const String& tsdb_namespace,
    const String& table_name,
    const Vector<PartitionLocation>& partitions,
    RefPtr<csql::SequentialScanNode> seqscan,
    Option<SHA1Hash> cache_key,
    PartitionMap* partition_map,
    InternalAuth* auth);

  ScopedPtr<csql::ResultCursor> execute() override;

  size_t getNumColumns() const override;

  Option<SHA1Hash> getCacheKey() const override;

protected:

  bool next(csql::SValue* row, size_t row_len);

  ScopedPtr<csql::ResultCursor> openPartition(
      const PartitionLocation& partition);

  ScopedPtr<csql::ResultCursor> openLocalPartition(
      const SHA1Hash& partition_id,
      RefPtr<csql::SequentialScanNode> qtree);

  ScopedPtr<csql::ResultCursor> openRemotePartition(
      const SHA1Hash& partition_id,
      RefPtr<csql::SequentialScanNode> qtree,
      const Vector<ReplicaRef> servers);

  csql::Transaction* txn_;
  csql::ExecutionContext* execution_context_;
  csql::ExecutionContext child_execution_context_;
  String tsdb_namespace_;
  String table_name_;
  Vector<PartitionLocation> partitions_;
  RefPtr<csql::SequentialScanNode> seqscan_;
  Option<SHA1Hash> cache_key_;
  PartitionMap* partition_map_;
  InternalAuth* auth_;
  ScopedPtr<csql::ResultCursor> cur_cursor_;
  size_t cur_partition_;
};

}
