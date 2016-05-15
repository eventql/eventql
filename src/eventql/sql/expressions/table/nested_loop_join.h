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
  bool joined_tbl_row_found_;
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
