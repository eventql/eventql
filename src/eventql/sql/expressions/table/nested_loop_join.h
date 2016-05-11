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
#include <eventql/util/stdtypes.h>
#include <eventql/sql/runtime/defaultruntime.h>
#include <eventql/sql/qtree/JoinNode.h>

namespace csql {

class NestedLoopJoin : public TableExpression {
public:

  NestedLoopJoin(
      Transaction* txn,
      JoinType join_type,
      const Vector<JoinNode::InputColumnRef>& input_map,
      Vector<ValueExpression> select_expressions,
      Option<ValueExpression> join_cond_expr,
      Option<ValueExpression> where_expr,
      ScopedPtr<TableExpression> base_tbl,
      ScopedPtr<TableExpression> joined_tbl);

  //bool onInputRow(
  //    const TaskID& input_id,
  //    const SValue* row,
  //    int row_len) override;

  //void onInputsReady() override;
  ScopedPtr<ResultCursor> execute() override;


protected:

  ScopedPtr<ResultCursor> executeCartesianJoin();
  ScopedPtr<ResultCursor> executeInnerJoin();
  ScopedPtr<ResultCursor> executeOuterJoin();

  Transaction* txn_;
  JoinType join_type_;
  Vector<JoinNode::InputColumnRef> input_map_;
  Vector<SValue> input_buf_;
  Vector<String> column_names_;
  Vector<ValueExpression> select_exprs_;
  Option<ValueExpression> join_cond_expr_;
  Option<ValueExpression> where_expr_;
  ScopedPtr<TableExpression> base_tbl_;
  ScopedPtr<ResultCursor> base_tbl_cursor_;
  Vector<SValue> base_tbl_row_;
  ScopedPtr<TableExpression> joined_tbl_;
  Vector<Vector<SValue>> joined_tbl_data_;
  size_t joined_tbl_pos_;
};

//class NestedLoopJoinFactory  : public TaskFactory {
//public:
//
//  NestedLoopJoinFactory(
//      JoinType join_type,
//      const Set<TaskID>& base_tbl_ids,
//      const Set<TaskID>& joined_tbl_ids,
//      const Vector<JoinNode::InputColumnRef>& input_map,
//      Vector<RefPtr<SelectListNode>> select_exprs,
//      Option<RefPtr<ValueExpressionNode>> join_cond_expr,
//      Option<RefPtr<ValueExpressionNode>> where_expr);
//
//  RefPtr<Task> build(
//      Transaction* txn,
//      HashMap<TaskID, ScopedPtr<ResultCursor>> input) const override;
//
//protected:
//  JoinType join_type_;
//  Set<TaskID> base_tbl_ids_;
//  Set<TaskID> joined_tbl_ids_;
//  Vector<JoinNode::InputColumnRef> input_map_;
//  Vector<RefPtr<SelectListNode>> select_exprs_;
//  Option<RefPtr<ValueExpressionNode>> join_cond_expr_;
//  Option<RefPtr<ValueExpressionNode>> where_expr_;
//};
//
}
