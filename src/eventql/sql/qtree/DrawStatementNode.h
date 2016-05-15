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
#include <eventql/sql/parser/astnode.h>
#include <eventql/sql/parser/token.h>
#include <eventql/sql/qtree/TableExpressionNode.h>

using namespace stx;

namespace csql {

class DrawStatementNode : public QueryTreeNode {
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
      Vector<RefPtr<QueryTreeNode>> tables);

  Vector<RefPtr<QueryTreeNode>> inputTables() const;

  ChartType chartType() const;

  const ASTNode* getProperty(Token::kTokenType key) const;

  const ASTNode* ast() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

  String toString() const override;

protected:
  ScopedPtr<ASTNode> ast_;
  Vector<RefPtr<QueryTreeNode>> tables_;
};

} // namespace csql
