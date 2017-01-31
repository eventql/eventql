/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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

  ReturnCode execute() override;

  size_t getColumnCount() const override;
  SType getColumnType(size_t idx) const override;

  bool next(SValue* row, size_t row_len) override;
  ReturnCode nextBatch(SVector* columns, size_t* nrecords) override;

protected:

  ReturnCode executeCartesianJoin();
  ReturnCode executeInnerJoin();
  ReturnCode executeOuterJoin();

  Transaction* txn_;
  JoinType join_type_;
  Vector<JoinNode::InputColumnRef> input_map_;
  Vector<SValue> input_buf_;
  Vector<String> column_names_;
  Vector<ValueExpression> select_exprs_;
  Option<ValueExpression> join_cond_expr_;
  Option<ValueExpression> where_expr_;
  ScopedPtr<TableExpression> base_tbl_;
  Vector<SValue> base_tbl_row_;
  size_t base_tbl_mincols_;
  ScopedPtr<TableExpression> joined_tbl_;
  Vector<Vector<SValue>> joined_tbl_data_;
  size_t joined_tbl_pos_;
  size_t joined_tbl_mincols_;
  bool joined_tbl_row_found_;
  std::function<bool (SValue*, int)> cursor_;
};

}
