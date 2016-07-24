/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/util/option.h>
#include <eventql/sql/backends/backend.h>
#include <eventql/sql/backends/tableref.h>
#include <eventql/sql/TableInfo.h>
#include <eventql/sql/table_provider.h>

namespace csql {
class QueryBuilder;
class Transaction;
class SequentialScanNode;

#include "eventql/eventql.h"

class TableRepository : public TableProvider {
public:

  virtual TableRef* getTableRef(const std::string& table_name) const;

  void addTableRef(
      const std::string& table_name,
      std::unique_ptr<TableRef>&& table_ref);

  void import(
      const std::vector<std::string>& tables,
      const std::string& source_uri,
      const std::vector<std::unique_ptr<Backend>>& backends);

  void addProvider(RefPtr<TableProvider> provider);

  void listTables(Function<void (const TableInfo& table)> fn) const override;

  Option<TableInfo> describe(const String& table_name) const override;

  Option<ScopedPtr<TableExpression>> buildSequentialScan(
      Transaction* ctx,
      ExecutionContext* execution_context,
      RefPtr<SequentialScanNode> seqscan) const override;

protected:
  std::unordered_map<std::string, std::unique_ptr<TableRef>> table_refs_;

  Vector<RefPtr<TableProvider>> providers_;
};

} // namespace csql
