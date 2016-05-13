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
#include <eventql/sql/result_cursor.h>
#include <eventql/core/PartitionMap.h>

using namespace stx;

namespace zbase {

class PartitionCursor : public csql::ResultCursor {
public:

  PartitionCursor(
      csql::Transaction* txn,
      RefPtr<Table> table,
      RefPtr<PartitionSnapshot> snap,
      RefPtr<csql::SequentialScanNode> stmt);

  bool next(csql::SValue* row, int row_len) override;

  size_t getNumColumns() override;

protected:

  csql::Transaction* txn_;
  RefPtr<Table> table_;
  RefPtr<PartitionSnapshot> snap_;
  RefPtr<csql::SequentialScanNode> stmt_;
  Set<SHA1Hash> id_set_;
  size_t cur_table_;
  ScopedPtr<csql::ResultCursor> cur_cursor_;
  ScopedPtr<csql::CSTableScan> cur_scan_;
};

}
