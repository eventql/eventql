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
#pragma once
#include "eventql/eventql.h"
#include <eventql/util/stdtypes.h>
#include <eventql/sql/runtime/tablerepository.h>
#include <eventql/sql/backends/csv/CSVInputStream.h>
#include <eventql/sql/backends/csv/CSVTableScan.h>

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

//  TaskIDList buildSequentialScan(
//      Transaction* txn,
//      RefPtr<SequentialScanNode> seqscan,
//      TaskDAG* tasks) const override;
//
  void listTables(
      Function<void (const csql::TableInfo& table)> fn) const override;

  Option<csql::TableInfo> describe(const String& table_name) const override;

  Option<ScopedPtr<TableExpression>> buildSequentialScan(
      Transaction* ctx,
      ExecutionContext* execution_context,
      RefPtr<SequentialScanNode> seqscan) const override;

protected:

  csql::TableInfo tableInfo() const;

  const String table_name_;
  FactoryFn stream_factory_;
  Vector<String> headers_;
};

} // namespace csv
} // namespace backends
} // namespace csql
