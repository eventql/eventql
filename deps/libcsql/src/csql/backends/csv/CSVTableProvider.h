/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <csql/runtime/tablerepository.h>
#include <csql/backends/csv/CSVInputStream.h>
#include <csql/backends/csv/CSVTableScan.h>

using namespace stx;

namespace csql {
namespace backends {
namespace csv {

struct CSVTableProvider : public TableProvider {
public:

  typedef Function<std::unique_ptr<CSVInputStream> ()> FactoryFn;

  CSVTableProvider(
      const String& table_name,
      const std::string& file_path,
      char column_separator = ';',
      char row_separator = '\n',
      char quote_char = '"');

  CSVTableProvider(
      const String& table_name,
      FactoryFn stream_factory);

  Option<ScopedPtr<TableExpression>> buildSequentialScan(
        Transaction* ctx,
        RefPtr<SequentialScanNode> node,
        QueryBuilder* runtime) const override;

  void listTables(
      Function<void (const csql::TableInfo& table)> fn) const override;

  Option<csql::TableInfo> describe(const String& table_name) const override;

protected:

  csql::TableInfo tableInfo() const;

  const String table_name_;
  FactoryFn stream_factory_;
  Vector<String> headers_;
};

} // namespace csv
} // namespace backends
} // namespace csql
