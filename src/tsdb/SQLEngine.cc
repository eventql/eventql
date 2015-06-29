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
  installDefaultSymbols(&sql_runtime_);
}

RefPtr<csql::QueryPlan> SQLEngine::parseAndBuildQueryPlan(
    const String& tsdb_namespace,
    const String& query) {
  return runtime_parseAndBuildQueryPlan(
      query,
      tableProviderForNamespace(tsdb_namespace),
      std::bind(
          &SQLEngine::rewriteQuery,
          tsdb_namespace,
          std::placeholders::_1));
}

RefPtr<csql::QueryTreeNode> SQLEngine::rewriteQuery(
    RefPtr<csql::QueryTreeNode> query) {
  if (dynamic_cast<csql::TableExpressionNode*>(query.get())) {
    auto tbl_expr = query.asInstanceOf<csql::TableExpressionNode>();
    replaceAllSequentialScansWithUnions(&tbl_expr);
    return tbl_expr.get();
  }

  return query;
}

RefPtr<csql::TableProvider> SQLEngine::tableProviderForNamespace(
    const String& tsdb_namespace) {
  return new TSDBTableProvider(tsdb_namespace, tsdb_node_);
}

void SQLEngine::replaceAllSequentialScansWithUnions(
    RefPtr<csql::TableExpressionNode>* node) {
  if (dynamic_cast<csql::SequentialScanNode*>(node->get())) {
    replaceSequentialScanWithUnion(node);
    return;
  }

  auto ntables = node->get()->numInputTables();
  for (int i = 0; i < ntables; ++i) {
    replaceAllSequentialScansWithUnions(node->get()->mutableInputTable(i));
  }
}

void SQLEngine::replaceSequentialScanWithUnion(
    RefPtr<csql::TableExpressionNode>* node) {
  auto seqscan = node->asInstanceOf<csql::SequentialScanNode>();

  RAISE(kNotYetImplementedError);
}

}
