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
#include <eventql/sql/qtree/TableExpressionNode.h>
#include <eventql/sql/qtree/ValueExpressionNode.h>
#include <eventql/sql/qtree/SelectListNode.h>

using namespace stx;

namespace csql {

class GroupByNode : public TableExpressionNode {
public:

  GroupByNode(
      Vector<RefPtr<SelectListNode>> select_list,
      Vector<RefPtr<ValueExpressionNode>> group_exprs,
      RefPtr<QueryTreeNode> table);

  GroupByNode(const GroupByNode& other);

  Vector<RefPtr<SelectListNode>> selectList() const;

  Vector<String> outputColumns() const override;

  Vector<QualifiedColumn> allColumns() const override;

  Vector<RefPtr<ValueExpressionNode>> groupExpressions() const;

  RefPtr<QueryTreeNode> inputTable() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

  String toString() const override;

  size_t getColumnIndex(
      const String& column_name,
      bool allow_add = false) override;

  static void encode(
      QueryTreeCoder* coder,
      const GroupByNode& node,
      stx::OutputStream* os);

  static RefPtr<QueryTreeNode> decode (
      QueryTreeCoder* coder,
      stx::InputStream* is);

protected:
  Vector<RefPtr<SelectListNode>> select_list_;
  Vector<String> column_names_;
  Vector<RefPtr<ValueExpressionNode>> group_exprs_;
  RefPtr<QueryTreeNode> table_;
};

} // namespace csql
