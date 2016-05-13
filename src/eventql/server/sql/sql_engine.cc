/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/util/http/httpclient.h>
#include <eventql/util/protobuf/msg.h>
#include <eventql/z1.h>
#include <eventql/server/sql/sql_engine.h>
#include <eventql/core/TSDBService.h>
#include <eventql/core/TimeWindowPartitioner.h>
#include <eventql/core/FixedShardPartitioner.h>
#include <eventql/sql/defaults.h>
#include <eventql/sql/qtree/CallExpressionNode.h>
#include <eventql/sql/qtree/ColumnReferenceNode.h>
#include <eventql/z1stats.h>

namespace zbase {

//RefPtr<csql::QueryTreeNode> SQLEngine::rewriteQuery(
//    csql::Runtime* runtime,
//    PartitionMap* partition_map,
//    ReplicationScheme* replication_scheme,
//    AnalyticsAuth* auth,
//    const String& tsdb_namespace,
//    RefPtr<csql::QueryTreeNode> query) {
//  rewriteTableTimeSuffix(query);
//  return query;
//}

RefPtr<csql::TableProvider> SQLEngine::tableProviderForNamespace(
    PartitionMap* partition_map,
    ReplicationScheme* replication_scheme,
    AnalyticsAuth* auth,
    const String& tsdb_namespace) {
  RAISE(kNotYetImplementedError, "not yet implemented");
  //return new TSDBTableProvider(
  //    tsdb_namespace,
  //    partition_map,
  //    replication_scheme,
  //    auth);
}

//void SQLEngine::rewriteTableTimeSuffix(
//      RefPtr<csql::QueryTreeNode> node) {
//  auto seqscan = dynamic_cast<csql::SequentialScanNode*>(node.get());
//  if (seqscan) {
//    auto table_ref = TSDBTableRef::parse(seqscan->tableName());
//    if (!table_ref.timerange_begin.isEmpty() &&
//        !table_ref.timerange_limit.isEmpty()) {
//      seqscan->setTableName(table_ref.table_key);
//
//      auto pred = mkRef(
//          new csql::CallExpressionNode(
//              "logical_and",
//              Vector<RefPtr<csql::ValueExpressionNode>>{
//                new csql::CallExpressionNode(
//                    "gte",
//                    Vector<RefPtr<csql::ValueExpressionNode>>{
//                      new csql::ColumnReferenceNode("time"),
//                      new csql::LiteralExpressionNode(
//                          csql::SValue(csql::SValue::IntegerType(
//                              table_ref.timerange_begin.get().unixMicros())))
//                    }),
//                new csql::CallExpressionNode(
//                    "lte",
//                    Vector<RefPtr<csql::ValueExpressionNode>>{
//                      new csql::ColumnReferenceNode("time"),
//                      new csql::LiteralExpressionNode(
//                          csql::SValue(csql::SValue::IntegerType(
//                              table_ref.timerange_limit.get().unixMicros())))
//                    })
//              }));
//
//      auto where_expr = seqscan->whereExpression();
//      if (!where_expr.isEmpty()) {
//        pred = mkRef(
//            new csql::CallExpressionNode(
//                "logical_and",
//                Vector<RefPtr<csql::ValueExpressionNode>>{
//                  where_expr.get(),
//                  pred.asInstanceOf<csql::ValueExpressionNode>()
//                }));
//      }
//
//      seqscan->setWhereExpression(
//          pred.asInstanceOf<csql::ValueExpressionNode>());
//    }
//  }
//
//  auto ntables = node->numChildren();
//  for (int i = 0; i < ntables; ++i) {
//    rewriteTableTimeSuffix(node->child(i));
//  }
//}
//
//RefPtr<csql::ExecutionStrategy> SQLEngine::getExecutionStrategy(
//    csql::Runtime* runtime,
//    PartitionMap* partition_map,
//    ReplicationScheme* replication_scheme,
//    AnalyticsAuth* auth,
//    const String& customer) {
//  auto strategy = mkRef(new csql::DefaultExecutionStrategy());
//
//  strategy->addTableProvider(
//      tableProviderForNamespace(
//          partition_map,
//          replication_scheme,
//          auth,
//          customer));
//
//  strategy->addQueryTreeRewriteRule(
//      std::bind(
//          &zbase::SQLEngine::rewriteQuery,
//          runtime,
//          partition_map,
//          replication_scheme,
//          auth,
//          customer,
//          std::placeholders::_1));
//
//  return strategy.get();
//}
//

void z1VersionExpr(sql_txn* ctx, int argc, csql::SValue* argv, csql::SValue* out) {
  *out = csql::SValue::newString(zbase::kVersionString);
}

}
