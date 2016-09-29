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
#include <eventql/sql/qtree/DrawStatementNode.h>

#include "eventql/eventql.h"

namespace csql {

DrawStatementNode::DrawStatementNode(
    const DrawStatementNode& other) :
    ast_(other.ast_->deepCopy()) {
  for (const auto& tbl : other.tables_) {
    tables_.emplace_back(tbl->deepCopyAs<QueryTreeNode>());
  }

  for (auto& table : tables_) {
    addChild(&table);
  }
}

DrawStatementNode::DrawStatementNode(
    ScopedPtr<ASTNode> ast,
    Vector<RefPtr<QueryTreeNode>> tables) :
    ast_(std::move(ast)),
    tables_(tables) {
  for (auto& table : tables_) {
    addChild(&table);
  }
}

Vector<RefPtr<QueryTreeNode>> DrawStatementNode::inputTables() const {
  return tables_;
}

DrawStatementNode::ChartType DrawStatementNode::chartType() const {
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

ASTNode const* DrawStatementNode::getProperty(Token::kTokenType key) const {
  for (const auto& child : ast_->getChildren()) {
    if (child->getType() != ASTNode::T_PROPERTY) {
      continue;
    }

    if (child->getToken()->getType() != key) {
      continue;
    }

    const auto& values = child->getChildren();
    if (values.size() != 1) {
      RAISE(kRuntimeError, "corrupt AST: T_PROPERTY has != 1 child");
    }

    return values[0];
  }

  return nullptr;
}

const ASTNode* DrawStatementNode::ast() const {
  return ast_.get();
}

RefPtr<QueryTreeNode> DrawStatementNode::deepCopy() const {
  return new DrawStatementNode(*this);
}

String DrawStatementNode::toString() const {
  String str = "(draw";

  for (const auto& tbl : tables_) {
    str += " (subexpr " + tbl->toString() + ")";
  }

  str += ")";
  return str;
}

} // namespace csql
