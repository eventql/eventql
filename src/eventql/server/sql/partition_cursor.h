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
#include <eventql/sql/result_cursor.h>
#include <eventql/db/partition_map.h>

#include "eventql/eventql.h"

namespace eventql {

class PartitionCursor : public csql::ResultCursor {
public:

  PartitionCursor(
      csql::Transaction* txn,
      csql::ExecutionContext* execution_context,
      RefPtr<Table> table,
      RefPtr<PartitionSnapshot> snap,
      RefPtr<csql::SequentialScanNode> stmt);

  bool next(csql::SValue* row, int row_len) override;

  size_t getNumColumns() override;

protected:

  bool openNextTable();

  csql::Transaction* txn_;
  csql::ExecutionContext* execution_context_;
  RefPtr<Table> table_;
  RefPtr<PartitionSnapshot> snap_;
  RefPtr<csql::SequentialScanNode> stmt_;
  Set<SHA1Hash> id_set_;
  size_t cur_table_;
  ScopedPtr<csql::ResultCursor> cur_cursor_;
  ScopedPtr<csql::CSTableScan> cur_scan_;
  ScopedPtr<PartitionArena::SkiplistReader> cur_skiplist_;
};

class StaticPartitionCursor : public csql::ResultCursor {
public:

  StaticPartitionCursor(
      csql::Transaction* txn,
      csql::ExecutionContext* execution_context,
      RefPtr<Table> table,
      RefPtr<PartitionSnapshot> snap,
      RefPtr<csql::SequentialScanNode> stmt);

  bool next(csql::SValue* row, int row_len) override;

  size_t getNumColumns() override;

protected:

  csql::Transaction* txn_;
  csql::ExecutionContext* execution_context_;
  RefPtr<Table> table_;
  RefPtr<PartitionSnapshot> snap_;
  RefPtr<csql::SequentialScanNode> stmt_;
  ScopedPtr<csql::ResultCursor> cur_cursor_;
  ScopedPtr<csql::CSTableScan> cur_scan_;
};

}
