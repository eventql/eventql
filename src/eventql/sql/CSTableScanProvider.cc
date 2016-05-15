/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include <eventql/sql/CSTableScanProvider.h>
#include <eventql/sql/CSTableScan.h>
#include <eventql/sql/qtree/SequentialScanNode.h>

using namespace util;

namespace csql {

CSTableScanProvider::CSTableScanProvider(
    const String& table_name,
    const String& cstable_file) :
    table_name_(table_name),
    cstable_file_(cstable_file) {}

Option<ScopedPtr<TableExpression>> CSTableScanProvider::buildSequentialScan(
    Transaction* txn,
    RefPtr<SequentialScanNode> node) const {
  if (node->tableName() != table_name_) {
    return None<ScopedPtr<TableExpression>>();
  }

  return Option<ScopedPtr<TableExpression>>(
      ScopedPtr<TableExpression>(
          new CSTableScan(
              txn,
              node,
              cstable_file_)));
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
