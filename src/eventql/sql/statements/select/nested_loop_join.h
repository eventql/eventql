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

  static const size_t kOutputBatchSize = 1024;

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

  ReturnCode nextBatch(SVector* columns, size_t* len) override;

protected:

  ReturnCode executeCartesianJoin(SVector* output, size_t* output_len);
  ReturnCode executeInnerJoin(SVector* output, size_t* output_len);
  ReturnCode executeOuterJoin(SVector* output, size_t* output_len);

  ReturnCode readJoinedTable();
  ReturnCode readBaseTableBatch();

  Transaction* txn_;
  JoinType join_type_;
  Vector<JoinNode::InputColumnRef> input_map_;
  Vector<String> column_names_;
  Vector<ValueExpression> select_exprs_;
  Option<ValueExpression> join_cond_expr_;
  Option<ValueExpression> where_expr_;
  ScopedPtr<TableExpression> base_tbl_;
  ScopedPtr<TableExpression> joined_tbl_;
  std::vector<SVector> joined_tbl_data_;
  std::vector<const void*> joined_tbl_cur_;
  size_t joined_tbl_size_;
  size_t joined_tbl_pos_;
  bool joined_tbl_row_found_;
  std::vector<SVector> base_tbl_buffer_;
  std::vector<const void*> base_tbl_cur_;
  size_t base_tbl_buffer_size_;
  VMStack vm_stack_;
};

}
