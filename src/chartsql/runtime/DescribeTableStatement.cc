/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/DescribeTableStatement.h>

namespace csql {

DescribeTableStatement::DescribeTableStatement(
    TableInfo table_info) :
    table_info_(table_info) {}

void DescribeTableStatement::execute(
    ExecutionContext* context,
    Function<bool (int argc, const SValue* argv)> fn) {
  for (const auto& col : table_info_.columns) {
    Vector<SValue> row;
    row.emplace_back(col.column_name);
    fn(row.size(), row.data());
  }
}

Vector<String> DescribeTableStatement::columnNames() const {
  return Vector<String> {
    "column_name"
  };
}

size_t DescribeTableStatement::numColumns() const {
  return columnNames().size();
}

}
