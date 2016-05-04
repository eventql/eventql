/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <csql/tasks/Task.h>
#include <csql/runtime/defaultruntime.h>
#include <csql/qtree/JoinNode.h>

namespace csql {

class NestedLoopJoin : public Task {
public:

  NestedLoopJoin(
      Transaction* txn,
      JoinType join_type,
      ScopedPtr<Task> base_tbl,
      ScopedPtr<Task> joined_tbl,
      const Vector<JoinNode::InputColumnRef>& input_map,
      const Vector<String>& column_names,
      Vector<ValueExpression> select_expressions,
      Option<ValueExpression> join_cond_expr,
      Option<ValueExpression> where_expr);

protected:

  void executeCartesianJoin(
      ExecutionContext* context,
      Function<bool (int argc, const SValue* argv)> fn,
      const List<Vector<SValue>>& t1,
      const List<Vector<SValue>>& t2);

  void executeInnerJoin(
      ExecutionContext* context,
      Function<bool (int argc, const SValue* argv)> fn,
      const List<Vector<SValue>>& t1,
      const List<Vector<SValue>>& t2);

  void executeOuterJoin(
      ExecutionContext* context,
      Function<bool (int argc, const SValue* argv)> fn,
      const List<Vector<SValue>>& t1,
      const List<Vector<SValue>>& t2);

  Transaction* txn_;
  JoinType join_type_;
  ScopedPtr<Task> base_table_;
  ScopedPtr<Task> joined_table_;
  Vector<JoinNode::InputColumnRef> input_map_;
  Vector<String> column_names_;
  Vector<ValueExpression> select_exprs_;
  Option<ValueExpression> join_cond_expr_;
  Option<ValueExpression> where_expr_;
};

}
