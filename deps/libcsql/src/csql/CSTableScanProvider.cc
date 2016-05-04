/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/CSTableScanProvider.h>
#include <csql/CSTableScan.h>
#include <csql/qtree/SequentialScanNode.h>

using namespace stx;

namespace csql {

CSTableScanProvider::CSTableScanProvider(
    const String& table_name,
    const String& cstable_file) :
    table_name_(table_name),
    cstable_file_(cstable_file) {}

Option<ScopedPtr<TableExpression>>
    CSTableScanProvider::buildSequentialScan(
        Transaction* ctx,
        RefPtr<SequentialScanNode> node,
        QueryBuilder* runtime) const {
  return Option<ScopedPtr<TableExpression>>(
      mkScoped(
          new CSTableScan(
              ctx,
              node,
              cstable_file_,
              runtime)));
}

void CSTableScanProvider::listTables(
    Function<void (const csql::TableInfo& table)> fn) const {
  fn(tableInfo());
}

Option<csql::TableInfo> CSTableScanProvider::describe(
    const String& table_name) const {
  if (table_name == table_name_) {
    return Some(tableInfo());
  } else {
    return None<csql::TableInfo>();
  }
}

csql::TableInfo CSTableScanProvider::tableInfo() const {
  auto cstable = cstable::CSTableReader::openFile(cstable_file_);

  csql::TableInfo ti;
  ti.table_name = table_name_;

  for (const auto& col : cstable->columns()) {
    csql::ColumnInfo ci;
    ci.column_name = col.column_name;
    ci.type_size = 0;
    ci.is_nullable = true;

    switch (col.logical_type) {

      case cstable::ColumnType::BOOLEAN:
        ci.type = "bool";
        break;

      case cstable::ColumnType::UNSIGNED_INT:
        ci.type = "uint64";
        break;

      case cstable::ColumnType::SIGNED_INT:
        ci.type = "int64";
        break;

      case cstable::ColumnType::FLOAT:
        ci.type = "double";
        break;

      case cstable::ColumnType::STRING:
        ci.type = "string";
        break;

      case cstable::ColumnType::DATETIME:
        ci.type = "datetime";
        break;

    }

    ti.columns.emplace_back(ci);
  }

  return ti;
}

} // namespace csql
