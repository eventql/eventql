/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
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
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/sql/result_cursor.h>
#include <eventql/sql/runtime/QueryPlan.h>
#include "eventql/eventql.h"
#include <eventql/sql/expressions/table_expression.h>
#include <eventql/sql/qtree/QueryTreeNode.h>
#include <eventql/sql/expressions/table/select.h>
#include <eventql/sql/expressions/table/subquery.h>
#include <eventql/sql/expressions/table/orderby.h>
#include <eventql/sql/expressions/table/show_tables.h>
#include <eventql/sql/expressions/table/limit.h>
#include <eventql/sql/expressions/table/describe_table.h>
#include <eventql/sql/expressions/table/groupby.h>
#include <eventql/sql/expressions/table/nested_loop_join.h>
#include <eventql/sql/qtree/SelectExpressionNode.h>
#include <eventql/sql/qtree/SubqueryNode.h>
#include <eventql/sql/qtree/OrderByNode.h>
#include <eventql/sql/qtree/DescribeTableNode.h>
#include <eventql/sql/qtree/LimitNode.h>
#include <eventql/sql/qtree/GroupByNode.h>
#include <eventql/sql/qtree/JoinNode.h>
#include <eventql/sql/runtime/queryplan.h>
#include <eventql/db/partition_map.h>
#include <eventql/AnalyticsAuth.h>


namespace eventql {

class Scheduler : public csql::Scheduler {
public:

  const size_t kMaxConcurrency = 8;

  Scheduler(
      PartitionMap* pmap,
      AnalyticsAuth* auth,
      ReplicationScheme* repl_scheme);

  ScopedPtr<csql::ResultCursor> execute(
      csql::QueryPlan* query_plan,
      size_t stmt_idx) override;

protected:

  ScopedPtr<csql::TableExpression> buildExpression(
      csql::Transaction* ctx,
      RefPtr<csql::QueryTreeNode> node);

  ScopedPtr<csql::TableExpression> buildLimit(
      csql::Transaction* ctx,
      RefPtr<csql::LimitNode> node);

  ScopedPtr<csql::TableExpression> buildSelectExpression(
      csql::Transaction* ctx,
      RefPtr<csql::SelectExpressionNode> node);

  ScopedPtr<csql::TableExpression> buildSubquery(
      csql::Transaction* txn,
      RefPtr<csql::SubqueryNode> node);

  ScopedPtr<csql::TableExpression> buildOrderByExpression(
      csql::Transaction* txn,
      RefPtr<csql::OrderByNode> node);

  ScopedPtr<csql::TableExpression> buildSequentialScan(
      csql::Transaction* txn,
      RefPtr<csql::SequentialScanNode> node);

  ScopedPtr<csql::TableExpression> buildGroupByExpression(
      csql::Transaction* txn,
      RefPtr<csql::GroupByNode> node);

  ScopedPtr<csql::TableExpression> buildPartialGroupByExpression(
      csql::Transaction* txn,
      RefPtr<csql::GroupByNode> node);

  ScopedPtr<csql::TableExpression> buildPipelineGroupByExpression(
      csql::Transaction* txn,
      RefPtr<csql::GroupByNode> node);

  ScopedPtr<csql::TableExpression> buildJoinExpression(
      csql::Transaction* txn,
      RefPtr<csql::JoinNode> node);

  struct PipelinedQueryTree {
    bool is_local;
    RefPtr<csql::QueryTreeNode> qtree;
    Vector<ReplicaRef> hosts;
  };

  Vector<PipelinedQueryTree> pipelineExpression(
      csql::Transaction* txn,
      RefPtr<csql::QueryTreeNode> qtree);

  bool isPipelineable(const csql::QueryTreeNode& qtree);

  PartitionMap* pmap_;
  AnalyticsAuth* auth_;
  ReplicationScheme* repl_scheme_;
};

} // namespace eventql

