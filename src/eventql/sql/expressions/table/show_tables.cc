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
#include <eventql/sql/expressions/table/show_tables.h>
#include <eventql/sql/transaction.h>

namespace csql {

ShowTablesExpression::ShowTablesExpression(
    Transaction* txn) :
    txn_(txn),
    counter_(0) {}

ReturnCode ShowTablesExpression::execute() {
  txn_->getTableProvider()->listTables([this] (const TableInfo& table) {
    Vector<SValue> row;
    row.emplace_back(table.table_name);

    if (table.description.isEmpty()) {
      row.emplace_back();
    } else {
      row.emplace_back(table.description.get());
    }

    buf_.emplace_back(row);
  });

  return ReturnCode::success();
}

ReturnCode ShowTablesExpression::nextBatch(
    size_t limit,
    SVector* columns,
    size_t* nrecords) {
  return ReturnCode::error("ERUNTIME", "ShowTablesExpression::nextBatch not yet implemented");
}

size_t ShowTablesExpression::getColumnCount() const {
  return kNumColumns;
}

SType ShowTablesExpression::getColumnType(size_t idx) const {
  assert(idx < kNumColumns);
  return SType::STRING;
}

bool ShowTablesExpression::next(SValue* row, size_t row_len) {
  if (counter_ < buf_.size()) {
    for (size_t i = 0; i < kNumColumns && i < row_len; ++i) {
      row[i] = buf_[counter_][i];
    }

    ++counter_;
    return true;
  } else {
    return false;
  }
}

}
