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
#include <csql/runtime/TableExpression.h>
#include <csql/runtime/defaultruntime.h>
#include <csql/qtree/JoinNode.h>

namespace csql {

class NestedLoopJoin : public TableExpression {
public:

  NestedLoopJoin(
      Transaction* txn,
      JoinType join_type,
      ScopedPtr<TableExpression> base_tbl,
      ScopedPtr<TableExpression> joined_tbl,
      const Vector<JoinNode::InputColumnRef>& input_map,
      const Vector<String>& column_names,
      Vector<ValueExpression> select_expressions,
      Option<ValueExpression> join_cond_expr,
      Option<ValueExpression> where_expr);

  void prepare(ExecutionContext* context) override;

  void execute(
      ExecutionContext* context,
      Function<bool (int argc, const SValue* argv)> fn) override;

  Vector<String> columnNames() const override;

  size_t numColumns() const override;

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
  ScopedPtr<TableExpression> base_table_;
  ScopedPtr<TableExpression> joined_table_;
  Vector<JoinNode::InputColumnRef> input_map_;
  Vector<String> column_names_;
  Vector<ValueExpression> select_exprs_;
  Option<ValueExpression> join_cond_expr_;
  Option<ValueExpression> where_expr_;
};

}
