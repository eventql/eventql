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
#include <tsdb/TimeWindowPartitioner.h>
#include <chartsql/defaults.h>

namespace tsdb {

SQLEngine::SQLEngine(TSDBNode* tsdb_node) : tsdb_node_(tsdb_node) {
  installDefaultSymbols(&sql_runtime_);
}

void SQLEngine::executeQuery(
    const String& tsdb_namespace,
    const String& query,
    RefPtr<csql::ResultFormat> result_format) {
  auto qplan = parseAndBuildQueryPlan(tsdb_namespace, query);
  csql::ExecutionContext context;
  result_format->formatResults(qplan, &context);
}

RefPtr<csql::QueryPlan> SQLEngine::parseAndBuildQueryPlan(
    const String& tsdb_namespace,
    const String& query) {
  return sql_runtime_.parseAndBuildQueryPlan(
      query,
      tableProviderForNamespace(tsdb_namespace),
      std::bind(
          &SQLEngine::rewriteQuery,
          this,
          tsdb_namespace,
          std::placeholders::_1));
}

RefPtr<csql::QueryTreeNode> SQLEngine::rewriteQuery(
    const String& tsdb_namespace,
    RefPtr<csql::QueryTreeNode> query) {
  if (dynamic_cast<csql::TableExpressionNode*>(query.get())) {
    auto tbl_expr = query.asInstanceOf<csql::TableExpressionNode>();
    replaceAllSequentialScansWithUnions(tsdb_namespace, &tbl_expr);
    return tbl_expr.get();
  }

  return query;
}

RefPtr<csql::TableProvider> SQLEngine::tableProviderForNamespace(
    const String& tsdb_namespace) {
  return new TSDBTableProvider(tsdb_namespace, tsdb_node_);
}

void SQLEngine::replaceAllSequentialScansWithUnions(
    const String& tsdb_namespace,
    RefPtr<csql::TableExpressionNode>* node) {
  if (dynamic_cast<csql::SequentialScanNode*>(node->get())) {
    replaceSequentialScanWithUnion(tsdb_namespace, node);
    return;
  }

  auto ntables = node->get()->numInputTables();
  for (int i = 0; i < ntables; ++i) {
    replaceAllSequentialScansWithUnions(
        tsdb_namespace,
        node->get()->mutableInputTable(i));
  }
}

void SQLEngine::replaceSequentialScanWithUnion(
    const String& tsdb_namespace,
    RefPtr<csql::TableExpressionNode>* node) {
  auto seqscan = node->asInstanceOf<csql::SequentialScanNode>();

  auto stream_key = seqscan->tableName();
  if (StringUtil::beginsWith(stream_key, "tsdb://")) {
    return;
  }

  auto partitions = TimeWindowPartitioner::partitionKeysFor(
      stream_key,
      WallClock::unixMicros() - 30 * kMicrosPerDay,
      WallClock::unixMicros(),
      4 * kMicrosPerHour);

  Vector<RefPtr<csql::TableExpressionNode>> union_tables;
  for (const auto& partition : partitions) {
    auto table_name = StringUtil::format(
        "tsdb://$0/$1",
        URI::urlEncode(stream_key),
        partition.toString());

    auto copy = seqscan->deepCopyAs<csql::SequentialScanNode>();
    copy->setTableName(table_name);
    union_tables.emplace_back(copy.get());
  }

  *node = RefPtr<csql::TableExpressionNode>(new csql::UnionNode(union_tables));
}

}
