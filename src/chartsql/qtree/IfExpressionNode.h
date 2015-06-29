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
#include <chartsql/qtree/ScalarExpressionNode.h>

using namespace fnord;

namespace csql {

class IfExpressionNode : public ScalarExpressionNode {
public:

  IfExpressionNode(
      RefPtr<ScalarExpressionNode> conditional_expr,
      RefPtr<ScalarExpressionNode> true_branch_expr,
      RefPtr<ScalarExpressionNode> false_branch_expr);

  Vector<RefPtr<ScalarExpressionNode>> arguments() const override;

  RefPtr<ScalarExpressionNode> conditional() const;
  RefPtr<ScalarExpressionNode> trueBranch() const;
  RefPtr<ScalarExpressionNode> falseBranch() const;

protected:
  RefPtr<ScalarExpressionNode> conditional_expr_;
  RefPtr<ScalarExpressionNode> true_branch_expr_;
  RefPtr<ScalarExpressionNode> false_branch_expr_;
};

} // namespace csql
