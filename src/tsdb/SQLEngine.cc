/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <tsdb/SQLEngine.h>
#include <tsdb/TSDBNode.h>
#include <chartsql/defaults.h>

namespace tsdb {

SQLEngine::SQLEngine(TSDBNode* tsdb_node) : tsdb_node_(tsdb_node) {
  csql::installDefaultSymbols(&symbol_table_);
}

RefPtr<csql::QueryPlan> SQLEngine::parseAndBuildQueryPlan(
    const String& tsdb_namespace,
    const String& query) {
  return Runtime::parseAndBuildQueryPlan(
      query,
      tableProviderForNamespace(tsdb_namespace));
}

RefPtr<csql::QueryTreeNode> SQLEngine::rewriteQuery(
    RefPtr<csql::QueryTreeNode> query) {
  return query;
}

RefPtr<csql::TableProvider> SQLEngine::tableProviderForNamespace(
    const String& tsdb_namespace) {
  return new TSDBTableProvider(tsdb_namespace, tsdb_node_);
}

RefPtr<csql::TableProvider> SQLEngine::defaultTableProvider() {
  RAISE(kRuntimeError, "must provide a namespace");
}

}
