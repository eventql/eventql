/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/expressions/table/describe_table.h>
#include <eventql/sql/Transaction.h>

namespace csql {

DescribeTableStatement::DescribeTableStatement(
    Transaction* txn,
    const String& table_name) :
    txn_(txn),
    table_name_(table_name) {}

ScopedPtr<ResultCursor> DescribeTableStatement::execute() {
  RAISE(kNotYetImplementedError, "describe table not yet implemented");
}

bool DescribeTableStatement::next(SValue* row, size_t row_len) {
  return false;
}

//void DescribeTableStatement::onInputsReady() {
//  const auto& table_info = txn_->getTableProvider()->describe(table_name_);
//  if (table_info.isEmpty()) {
//    RAISEF(kRuntimeError, "table not found: '$0'", table_name_);
//  }
//
//  for (const auto& col : table_info.get().columns) {
//    Vector<SValue> row;
//    row.emplace_back(col.column_name);
//    row.emplace_back(col.type);
//    row.emplace_back(col.is_nullable ? "YES" : "NO");
//    row.emplace_back();
//    input_(row.data(), row.size());
//  }
//}

}
