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
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <stx/option.h>
#include <chartsql/qtree/QueryTreeNode.h>
#include <chartsql/qtree/GroupByNode.h>
#include <chartsql/qtree/GroupByMergeNode.h>
#include <chartsql/qtree/UnionNode.h>
#include <chartsql/qtree/LimitNode.h>
#include <chartsql/qtree/OrderByNode.h>
#include <chartsql/qtree/SequentialScanNode.h>
#include <chartsql/qtree/DrawStatementNode.h>
#include <chartsql/qtree/SelectExpressionNode.h>
#include <chartsql/qtree/DescribeTableNode.h>
#include <chartsql/runtime/TableExpression.h>
#include <chartsql/runtime/tablerepository.h>
#include <chartsql/svalue.h>

using namespace stx;

namespace csql {
class QueryBuilder;

class TableExpressionBuilder : public RefCounted {
public:

  ScopedPtr<TableExpression> build(
      RefPtr<QueryTreeNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

protected:

  ScopedPtr<TableExpression> buildGroupBy(
      RefPtr<GroupByNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

  ScopedPtr<TableExpression> buildGroupMerge(
      RefPtr<GroupByMergeNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

  ScopedPtr<TableExpression> buildUnion(
      RefPtr<UnionNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

  ScopedPtr<TableExpression> buildLimit(
      RefPtr<LimitNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

  ScopedPtr<TableExpression> buildOrderBy(
      RefPtr<OrderByNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

  ScopedPtr<TableExpression> buildSequentialScan(
      RefPtr<SequentialScanNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

  ScopedPtr<TableExpression> buildSelectExpression(
      RefPtr<SelectExpressionNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

  ScopedPtr<TableExpression> buildDescribeTableStatment(
      RefPtr<DescribeTableNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

};

} // namespace csql
