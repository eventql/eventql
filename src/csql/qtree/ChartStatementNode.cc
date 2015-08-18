/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/qtree/ChartStatementNode.h>

using namespace stx;

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
