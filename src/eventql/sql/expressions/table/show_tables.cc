/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/expressions/table/show_tables.h>
#include <eventql/sql/Transaction.h>

namespace csql {

ShowTablesExpression::ShowTablesExpression(
    Transaction* txn) :
    txn_(txn) {}

ScopedPtr<ResultCursor> ShowTablesExpression::execute() {
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

  return mkScoped(
      new DefaultResultCursor(
          k_num_columns_,
          std::bind(
              &ShowTablesExpression::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
}


bool ShowTablesExpression::next(SValue* row, size_t row_len) {
  if (pos_ < buf_.size() && row_len >= k_num_columns_) {
    for (size_t i = 0; i < k_num_columns_; ++i) {
      row[i] = buf_[pos_][i];
    }
    ++pos_;
    return true;
  } else {
    return false;
  }
}

}
