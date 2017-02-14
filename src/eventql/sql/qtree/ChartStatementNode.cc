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
#include <eventql/sql/qtree/ChartStatementNode.h>

#include "eventql/eventql.h"

namespace csql {

ChartStatementNode::ChartStatementNode(const ChartStatementNode& other) {
  for (const auto& draw_stmt : other.draw_stmts_) {
    draw_stmts_.emplace_back(draw_stmt->deepCopy());
  }

  for (auto& stmt : draw_stmts_) {
    addChild(&stmt);
  }
}

ChartStatementNode::ChartStatementNode(
    Vector<RefPtr<QueryTreeNode>> draw_stmts) :
    draw_stmts_(draw_stmts) {
  for (auto& stmt : draw_stmts_) {
    addChild(&stmt);
  }
}

const Vector<RefPtr<QueryTreeNode>>& ChartStatementNode::getDrawStatements() {
  return draw_stmts_;
}

Vector<String> ChartStatementNode::getResultColumns() const {
  return { kColumnName };
}

Vector<QualifiedColumn> ChartStatementNode::getAvailableColumns() const {
  return {{ kColumnName, kColumnName, SType::STRING }};
}

size_t ChartStatementNode::getComputedColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {

  if (column_name == kColumnName) {
    return 0;
  } else {
    return -1;
  }
}

size_t ChartStatementNode::getNumComputedColumns() const {
  return 1;
}

SType ChartStatementNode::getColumnType(size_t idx) const {
  return SType::STRING;
}

RefPtr<QueryTreeNode> ChartStatementNode::deepCopy() const {
  return new ChartStatementNode(*this);
}

String ChartStatementNode::toString() const {
  String str = "(chart";

  for (const auto& stmt : draw_stmts_) {
    str += stmt->toString();
  }

  str += ")";
  return str;
}

} // namespace csql
