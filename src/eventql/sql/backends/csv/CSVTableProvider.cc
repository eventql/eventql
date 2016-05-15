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
#include <eventql/sql/backends/csv/CSVTableProvider.h>
#include <eventql/sql/expressions/table/tablescan.h>

#include "eventql/eventql.h"

namespace csql {
namespace backends {
namespace csv {

CSVTableProvider::CSVTableProvider(
    const String& table_name,
    FactoryFn factory) :
    table_name_(table_name),
    stream_factory_(factory) {
  auto stream = stream_factory_();

  if (!stream->readNextRow(&headers_)) {
    RAISE(kRuntimeError, "can't read CSV headers");
  }
}

CSVTableProvider::CSVTableProvider(
      const String& table_name,
      const std::string& file_path,
      char column_separator /* = ';' */,
      char row_separator /* = '\n' */,
      char quote_char /* = '"' */) :
      CSVTableProvider(
          table_name,
          [file_path, column_separator, row_separator, quote_char] () {
            return CSVInputStream::openFile(
                file_path,
                column_separator,
                row_separator,
                quote_char);
          }) {}

void CSVTableProvider::listTables(
    Function<void (const TableInfo& table)> fn) const {
  fn(tableInfo());
}

Option<TableInfo> CSVTableProvider::describe(
    const String& table_name) const {
  if (table_name == table_name_) {
    return Some(tableInfo());
  } else {
    return None<TableInfo>();
  }
}

TableInfo CSVTableProvider::tableInfo() const {
  TableInfo ti;
  ti.table_name = table_name_;

  for (const auto& col : headers_) {
    ColumnInfo ci;
    ci.column_name = col;
    ci.type_size = 0;
    ci.is_nullable = true;
    ci.type = "string";
    ti.columns.emplace_back(ci);
  }

  return ti;
}

Option<ScopedPtr<TableExpression>> CSVTableProvider::buildSequentialScan(
    Transaction* txn,
    RefPtr<SequentialScanNode> seqscan) const {
  if (seqscan->tableName() != table_name_) {
    return None<ScopedPtr<TableExpression>>();
  }

  auto stream = stream_factory_();
  stream->skipNextRow();

  return Option<ScopedPtr<TableExpression>>(
      ScopedPtr<TableExpression>(
          new TableScan(
              txn,
              seqscan,
              mkScoped(new CSVTableScan(headers_, std::move(stream))))));

}


} // namespace csv
} // namespace backends
} // namespace csql
