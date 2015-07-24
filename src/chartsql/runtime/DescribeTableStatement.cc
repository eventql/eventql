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
    row.emplace_back(col.type);
    //if (col.type_size == 0) {
    //  row.emplace_back();
    //} else {
    //  row.emplace_back(SValue::IntegerType(col.type_size));
    //}
    row.emplace_back(col.is_nullable ? "YES" : "NO");
    row.emplace_back();
    fn(row.size(), row.data());
  }
}

Vector<String> DescribeTableStatement::columnNames() const {
  return Vector<String> {
    "Field",
    "Type",
    //"Type Size",
    "Null",
    "Description"
  };
}

size_t DescribeTableStatement::numColumns() const {
  return columnNames().size();
}

}
