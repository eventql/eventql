/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <fnord/stdtypes.h>
#include <chartsql/parser/astnode.h>
#include <chartsql/parser/token.h>
#include <chartsql/qtree/TableExpressionNode.h>

using namespace fnord;

namespace csql {

class DrawStatementNode : public TableExpressionNode {
public:

  enum class ChartType {
    AREACHART,
    BARCHART,
    LINECHART,
    POINTCHART
  };

  DrawStatementNode(const DrawStatementNode& other);
  DrawStatementNode(
      ScopedPtr<ASTNode> ast,
      Vector<RefPtr<TableExpressionNode>> tables);

  Vector<RefPtr<TableExpressionNode>> inputTables() const;

  ChartType chartType() const;

  const ASTNode* getProperty(Token::kTokenType key) const;

  const ASTNode* ast() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

protected:
  ScopedPtr<ASTNode> ast_;
  Vector<RefPtr<TableExpressionNode>> tables_;
};

} // namespace csql
