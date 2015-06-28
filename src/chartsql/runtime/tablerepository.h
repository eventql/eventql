/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_QUERY_TABLEREPOSITORY_H
#define _FNORDMETRIC_QUERY_TABLEREPOSITORY_H
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <chartsql/backends/backend.h>
#include <chartsql/backends/tableref.h>
#include <chartsql/runtime/TableExpression.h>
#include <chartsql/qtree/SequentialScanNode.h>

namespace csql {
class ImportStatement;

class TableRepository {
public:
  virtual ~TableRepository() {}

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

  ScopedPtr<TableExpression> scanTable(
      RefPtr<SequentialScanNode> seqscan) const;

protected:
  std::unordered_map<std::string, std::unique_ptr<TableRef>> table_refs_;
};

}
#endif
