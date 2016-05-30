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
#include "eventql/eventql.h"
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/sql/result_cursor.h>
#include <eventql/sql/query_plan.h>
#include <eventql/sql/runtime/ValueExpression.h>
#include <eventql/sql/scheduler/execution_context.h>
#include <eventql/sql/qtree/nodes/create_table.h>
#include <eventql/sql/qtree/nodes/insert_into.h>
#include <eventql/sql/qtree/TableExpressionNode.h>
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
#include <eventql/sql/extensions/chartsql/chart_expression.h>
#include <eventql/sql/qtree/SelectExpressionNode.h>
#include <eventql/sql/qtree/SubqueryNode.h>
#include <eventql/sql/qtree/OrderByNode.h>
#include <eventql/sql/qtree/DescribeTableNode.h>
#include <eventql/sql/qtree/LimitNode.h>
#include <eventql/sql/qtree/GroupByNode.h>
#include <eventql/sql/qtree/JoinNode.h>
#include <eventql/sql/qtree/ChartStatementNode.h>

namespace csql {
class QueryPlan;

class Scheduler : public RefCounted {
public:
  virtual ~Scheduler() {};
  virtual ScopedPtr<ResultCursor> execute(
      QueryPlan* query_plan,
      ExecutionContext* execution_context,
      size_t stmt_idx) = 0;
};

class DefaultScheduler : public Scheduler {
public:

  ScopedPtr<ResultCursor> execute(
      QueryPlan* query_plan,
      ExecutionContext* execution_context,
      size_t stmt_idx) override;

protected:

  virtual ScopedPtr<ResultCursor> executeSelect(
      Transaction* txn,
      ExecutionContext* execution_context,
      RefPtr<TableExpressionNode> select);

  virtual ScopedPtr<ResultCursor> executeDraw(
      Transaction* txn,
      ExecutionContext* execution_context,
      RefPtr<ChartStatementNode> node);

  virtual ScopedPtr<ResultCursor> executeCreateTable(
      Transaction* txn,
      ExecutionContext* execution_context,
      RefPtr<CreateTableNode> create_table);

  virtual ScopedPtr<ResultCursor> executeInsertInto(
      Transaction* txn,
      ExecutionContext* execution_context,
      RefPtr<InsertIntoNode> insert_into);

  virtual ScopedPtr<TableExpression> buildTableExpression(
      Transaction* ctx,
      ExecutionContext* execution_context,
      RefPtr<TableExpressionNode> node);

  virtual ScopedPtr<TableExpression> buildLimit(
      Transaction* ctx,
      ExecutionContext* execution_context,
      RefPtr<LimitNode> node);

  virtual ScopedPtr<TableExpression> buildSelectExpression(
      Transaction* ctx,
      ExecutionContext* execution_context,
      RefPtr<SelectExpressionNode> node);

  virtual ScopedPtr<TableExpression> buildSubquery(
      Transaction* txn,
      ExecutionContext* execution_context,
      RefPtr<SubqueryNode> node);

  virtual ScopedPtr<TableExpression> buildOrderByExpression(
      Transaction* txn,
      ExecutionContext* execution_context,
      RefPtr<OrderByNode> node);

  virtual ScopedPtr<TableExpression> buildSequentialScan(
      Transaction* txn,
      ExecutionContext* execution_context,
      RefPtr<SequentialScanNode> node);

  virtual ScopedPtr<TableExpression> buildGroupByExpression(
      Transaction* txn,
      ExecutionContext* execution_context,
      RefPtr<GroupByNode> node);

  virtual ScopedPtr<TableExpression> buildJoinExpression(
      Transaction* txn,
      ExecutionContext* execution_context,
      RefPtr<JoinNode> node);

};

} // namespace csql
