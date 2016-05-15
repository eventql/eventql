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
#include <eventql/util/option.h>
#include <eventql/sql/svalue.h>
#include <eventql/sql/qtree/TableExpressionNode.h>
#include <eventql/sql/qtree/ValueExpressionNode.h>
#include <eventql/sql/qtree/SelectListNode.h>

using namespace util;

namespace csql {

enum class JoinType {
  CARTESIAN, INNER, OUTER
};

class JoinNode : public TableExpressionNode {
public:

  struct InputColumnRef {
    String column;
    size_t table_idx;
    size_t column_idx;
  };

  static const uint8_t kHasWhereExprFlag = 1;
  static const uint8_t kHasJoinExprFlag = 2;

  JoinNode(
      JoinType join_type,
      RefPtr<QueryTreeNode> base_table,
      RefPtr<QueryTreeNode> joined_table,
      Vector<RefPtr<SelectListNode>> select_list,
      Option<RefPtr<ValueExpressionNode>> where_expr,
      Option<RefPtr<ValueExpressionNode>> join_expr);

  JoinNode(const JoinNode& other);

  JoinType joinType() const;

  RefPtr<QueryTreeNode> baseTable() const;
  RefPtr<QueryTreeNode> joinedTable() const;

  Vector<RefPtr<SelectListNode>> selectList() const;

  Vector<String> outputColumns() const override;

  Vector<QualifiedColumn> allColumns() const override;

  const Vector<InputColumnRef>& inputColumnMap() const;

  size_t getColumnIndex(
      const String& column_name,
      bool allow_add = false) override;

  size_t getInputColumnIndex(
      const String& column_name,
      bool allow_add = false);

  Option<RefPtr<ValueExpressionNode>> whereExpression() const;
  Option<RefPtr<ValueExpressionNode>> joinCondition() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

  String toString() const override;

  static void encode(
      QueryTreeCoder* coder,
      const JoinNode& node,
      util::OutputStream* os);

  static RefPtr<QueryTreeNode> decode(
      QueryTreeCoder* coder,
      util::InputStream* is);

protected:


  JoinType join_type_;
  RefPtr<QueryTreeNode> base_table_;
  RefPtr<QueryTreeNode> joined_table_;
  Vector<InputColumnRef> input_map_;
  Vector<String> column_names_;
  Vector<RefPtr<SelectListNode>> select_list_;
  Option<RefPtr<ValueExpressionNode>> where_expr_;
  Option<RefPtr<ValueExpressionNode>> join_cond_;
};

} // namespace csql
