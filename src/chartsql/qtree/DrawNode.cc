/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/DrawNode.h>
#include <chartsql/parser/Token.h>

using namespace fnord;

namespace csql {

DrawNode::DrawNode(
    const DrawNode& other) :
    ast_(other.ast_->deepCopy()) {
  for (const auto& tbl : other.tables_) {
    tables_.emplace_back(tbl->deepCopyAs<TableExpressionNode>());
  }
}

DrawNode::DrawNode(
    ScopedPtr<ASTNode> ast,
    Vector<RefPtr<TableExpressionNode>> tables) :
    ast_(std::move(ast)),
    tables_(tables) {
  for (auto& table : tables_) {
    addInputTable(&table);
  }
}

Vector<RefPtr<TableExpressionNode>> DrawNode::inputTables() const {
  return tables_;
}

DrawNode::ChartType DrawNode::chartType() const {
  switch (ast_->getToken()->getType()) {
    case Token::T_AREACHART:
      return ChartType::AREACHART;
    case Token::T_BARCHART:
      return ChartType::BARCHART;
    case Token::T_LINECHART:
      return ChartType::LINECHART;
    case Token::T_POINTCHART:
      return ChartType::POINTCHART;
    default:
      RAISEF(
          kRuntimeError,
          "invalid chart type: $0",
          Token::getTypeName(ast_->getToken()->getType()));
  }
}

RefPtr<QueryTreeNode> DrawNode::deepCopy() const {
  return new DrawNode(*this);
}

} // namespace csql
