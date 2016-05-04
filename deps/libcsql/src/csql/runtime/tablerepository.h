/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/option.h>
#include <csql/backends/backend.h>
#include <csql/backends/tableref.h>
#include <csql/tasks/Task.h>
#include <csql/qtree/SequentialScanNode.h>
#include <csql/TableInfo.h>

namespace csql {
class QueryBuilder;
class Transaction;

class TableProvider : public RefCounted {
public:

  virtual TaskIDList buildSequentialScan(
      Transaction* txn,
      RefPtr<SequentialScanNode> seqscan,
      TaskDAG* tasks) const = 0;

  virtual void listTables(Function<void (const TableInfo& table)> fn) const = 0;

  virtual Option<TableInfo> describe(const String& table_name) const = 0;

};

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

  TaskIDList buildSequentialScan(
      Transaction* txn,
      RefPtr<SequentialScanNode> seqscan,
      TaskDAG* tasks) const override;

  void addProvider(RefPtr<TableProvider> provider);

  void listTables(Function<void (const TableInfo& table)> fn) const override;

  Option<TableInfo> describe(const String& table_name) const override;

protected:
  std::unordered_map<std::string, std::unique_ptr<TableRef>> table_refs_;

  Vector<RefPtr<TableProvider>> providers_;
};

} // namespace csql
