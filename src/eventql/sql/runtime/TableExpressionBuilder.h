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
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/option.h>
#include <eventql/sql/qtree/QueryTreeNode.h>
#include <eventql/sql/qtree/GroupByNode.h>
#include <eventql/sql/qtree/GroupByMergeNode.h>
#include <eventql/sql/qtree/UnionNode.h>
#include <eventql/sql/qtree/LimitNode.h>
#include <eventql/sql/qtree/JoinNode.h>
#include <eventql/sql/qtree/OrderByNode.h>
#include <eventql/sql/qtree/SequentialScanNode.h>
#include <eventql/sql/qtree/DrawStatementNode.h>
#include <eventql/sql/qtree/SelectExpressionNode.h>
#include <eventql/sql/qtree/DescribeTableNode.h>
#include <eventql/sql/qtree/RemoteAggregateNode.h>
#include <eventql/sql/qtree/SubqueryNode.h>
#include <eventql/sql/tasks/Task.h>
#include <eventql/sql/runtime/tablerepository.h>
#include <eventql/sql/svalue.h>

using namespace stx;

namespace csql {
class QueryBuilder;

class TableExpressionBuilder : public RefCounted {
public:

  ScopedPtr<Task> build(
      Transaction* ctx,
      RefPtr<QueryTreeNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

protected:

  ScopedPtr<Task> buildGroupBy(
      Transaction* ctx,
      RefPtr<GroupByNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

  ScopedPtr<Task> buildGroupMerge(
      Transaction* ctx,
      RefPtr<GroupByMergeNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

  ScopedPtr<Task> buildJoin(
      Transaction* ctx,
      RefPtr<JoinNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

  ScopedPtr<Task> buildUnion(
      Transaction* ctx,
      RefPtr<UnionNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

  ScopedPtr<Task> buildLimit(
      Transaction* ctx,
      RefPtr<LimitNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

  ScopedPtr<Task> buildSequentialScan(
      Transaction* ctx,
      RefPtr<SequentialScanNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

  ScopedPtr<Task> buildSubquery(
      Transaction* ctx,
      RefPtr<SubqueryNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

  ScopedPtr<Task> buildSelectExpression(
      Transaction* ctx,
      RefPtr<SelectExpressionNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

  ScopedPtr<Task> buildDescribeTableStatment(
      Transaction* ctx,
      RefPtr<DescribeTableNode> node,
      QueryBuilder* runtime,
      TableProvider* tables);

  ScopedPtr<Task> buildRemoteAggregate(
      Transaction* ctx,
      RefPtr<RemoteAggregateNode> node,
      QueryBuilder* runtime);

};

} // namespace csql
