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
#include <eventql/util/http/httpclient.h>
#include <eventql/util/protobuf/msg.h>
#include <eventql/eventql.h>
#include <eventql/server/sql/sql_engine.h>
#include <eventql/server/sql/table_provider.h>
#include <eventql/core/TSDBService.h>
#include <eventql/core/TimeWindowPartitioner.h>
#include <eventql/core/FixedShardPartitioner.h>
#include <eventql/sql/defaults.h>
#include <eventql/sql/qtree/CallExpressionNode.h>
#include <eventql/sql/qtree/ColumnReferenceNode.h>
#include <eventql/z1stats.h>

namespace eventql {

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
  return new TSDBTableProvider(
      tsdb_namespace,
      partition_map,
      replication_scheme,
      auth);
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
//          &eventql::SQLEngine::rewriteQuery,
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
  *out = csql::SValue::newString(eventql::kVersionString);
}

}
