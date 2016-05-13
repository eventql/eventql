/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/backends/csv/CSVTableProvider.h>
#include <eventql/sql/expressions/table/tablescan.h>

using namespace stx;

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
