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
#include <chartsql/runtime/runtime.h>
#include <tsdb/TSDBTableProvider.h>

namespace tsdb {
class TSDBNode;

class SQLEngine : public csql::Runtime {
public:

  SQLEngine(TSDBNode* tsdb_node);

  RefPtr<csql::QueryPlan> parseAndBuildQueryPlan(
      const String& tsdb_namespace,
      const String& query);

protected:

  RefPtr<csql::TableProvider> tableProviderForNamespace(
      const String& tsdb_namespace);

  RefPtr<csql::QueryTreeNode> rewriteQuery(
      RefPtr<csql::QueryTreeNode> query) override;

  void replaceAllSequentialScansWithUnions(RefPtr<csql::QueryTreeNode>* node);

  void replaceSequentialScanWithUnion(RefPtr<csql::QueryTreeNode>* node);

  RefPtr<csql::TableProvider> defaultTableProvider() override;

  TSDBNode* tsdb_node_;
};

}
