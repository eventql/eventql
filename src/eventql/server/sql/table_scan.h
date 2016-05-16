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
#include <eventql/AnalyticsAuth.h>
#include <eventql/db/partition_map.h>

#include "eventql/eventql.h"

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
