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
#include <chartsql/backends/backend.h>
#include <chartsql/backends/tableref.h>
#include <chartsql/runtime/TableExpression.h>
#include <chartsql/qtree/SequentialScanNode.h>

namespace csql {
class ImportStatement;
class QueryBuilder;

struct ColumnInfo {
  String column_name;
  String type;
  bool is_nullable;
  size_t type_size;
};

struct TableInfo {
  String table_name;
  Option<String> description;
  Vector<ColumnInfo> columns;
};

class TableProvider : public RefCounted {
public:

  virtual Option<ScopedPtr<TableExpression>> buildSequentialScan(
      RefPtr<SequentialScanNode> seqscan,
      QueryBuilder* runtime) const = 0;

  virtual void listTables(Function<void (const TableInfo& table)> fn) const = 0;

  virtual Option<TableInfo> describe(const String& table_name) const = 0;

};

class TableRepository : public TableProvider {
public:
  typedef
      Function<RefPtr<ScopedPtr<TableExpression>> (RefPtr<SequentialScanNode>)>
      TableFactoryFn;

  virtual TableRef* getTableRef(const std::string& table_name) const;

  void addTableRef(
      const std::string& table_name,
      std::unique_ptr<TableRef>&& table_ref);

  void import(
      const std::vector<std::string>& tables,
      const std::string& source_uri,
      const std::vector<std::unique_ptr<Backend>>& backends);

  void import(
      const ImportStatement& import_stmt,
      const std::vector<std::unique_ptr<Backend>>& backends);

  Option<ScopedPtr<TableExpression>> buildSequentialScan(
      RefPtr<SequentialScanNode> seqscan,
      QueryBuilder* runtime) const override;

  void addProvider(RefPtr<TableProvider> provider);

  void listTables(Function<void (const TableInfo& table)> fn) const override;

  Option<TableInfo> describe(const String& table_name) const override;

protected:
  std::unordered_map<std::string, std::unique_ptr<TableRef>> table_refs_;

  Vector<RefPtr<TableProvider>> providers_;
};

}
