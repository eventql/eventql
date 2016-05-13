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
#include "eventql/server/sql/partition_cursor.h"

namespace zbase {

PartitionCursor::PartitionCursor(
    csql::Transaction* txn,
    RefPtr<Table> table,
    RefPtr<PartitionSnapshot> snap,
    RefPtr<csql::SequentialScanNode> stmt) :
    txn_(txn),
    table_(table),
    snap_(snap),
    stmt_(stmt) {};

bool PartitionCursor::next(csql::SValue* row, int row_len) {
  return false;
}

size_t PartitionCursor::getNumColumns() {
  return stmt_->numColumns();
}

}
