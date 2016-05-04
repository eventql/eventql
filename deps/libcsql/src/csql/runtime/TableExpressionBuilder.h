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
#include <csql/qtree/QueryTreeNode.h>
#include <csql/qtree/GroupByNode.h>
#include <csql/qtree/GroupByMergeNode.h>
#include <csql/qtree/UnionNode.h>
#include <csql/qtree/LimitNode.h>
#include <csql/qtree/JoinNode.h>
#include <csql/qtree/OrderByNode.h>
#include <csql/qtree/SequentialScanNode.h>
#include <csql/qtree/DrawStatementNode.h>
#include <csql/qtree/SelectExpressionNode.h>
#include <csql/qtree/DescribeTableNode.h>
#include <csql/qtree/RemoteAggregateNode.h>
#include <csql/qtree/SubqueryNode.h>
#include <csql/tasks/Task.h>
#include <csql/runtime/tablerepository.h>
#include <csql/svalue.h>

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
