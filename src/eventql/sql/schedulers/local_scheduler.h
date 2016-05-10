/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth, zScale Technology GmbH
 *
 * libcsql is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <eventql/sql/scheduler.h>
#include <eventql/sql/expressions/table_expression.h>
#include <eventql/sql/qtree/QueryTreeNode.h>
#include <eventql/sql/expressions/table/select.h>
#include <eventql/sql/expressions/table/subquery.h>
#include <eventql/sql/expressions/table/orderby.h>
#include <eventql/sql/expressions/table/describe_table.h>
#include <eventql/sql/qtree/SelectExpressionNode.h>
#include <eventql/sql/qtree/SubqueryNode.h>
#include <eventql/sql/qtree/OrderByNode.h>
#include <eventql/sql/qtree/DescribeTableNode.h>



using namespace stx;

namespace csql {
class Transaction;

class LocalScheduler : public Scheduler {
public:

  ScopedPtr<ResultCursor> execute(QueryPlan* query_plan, size_t stmt_idx) override;

protected:

  ScopedPtr<TableExpression> buildExpression(
      Transaction* ctx,
      RefPtr<QueryTreeNode> node);

  ScopedPtr<TableExpression> buildSelectExpression(
      Transaction* ctx,
      RefPtr<SelectExpressionNode> node);

  ScopedPtr<TableExpression> buildSubquery(
      Transaction* ctx,
      RefPtr<SubqueryNode> node);

  ScopedPtr<TableExpression> buildSequentialScan(
    Transaction* txn,
    RefPtr<SequentialScanNode> node);

  ScopedPtr<TableExpression> buildOrderByExpression(
    Transaction* txn,
    RefPtr<OrderByNode> node);

  ScopedPtr<TableExpression> buildDescribeTableStatement(
    Transaction* txn,
    RefPtr<DescribeTableNode> node);
  
};

class LocalResultCursor : public ResultCursor {
public:

  LocalResultCursor(ScopedPtr<TableExpression> table_expression);

  bool next(SValue* row, int row_len) override;

  size_t getNumColumns() override;

protected:
  ScopedPtr<TableExpression> table_expression_;
  ScopedPtr<ResultCursor> cursor_;
};

} // namespace csql
