/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/IfExpressionNode.h>

using namespace fnord;

namespace csql {

IfExpressionNode::IfExpressionNode(
    RefPtr<ScalarExpressionNode> conditional_expr,
    RefPtr<ScalarExpressionNode> true_branch_expr,
    RefPtr<ScalarExpressionNode> false_branch_expr) :
    conditional_expr_(conditional_expr),
    true_branch_expr_(true_branch_expr),
    false_branch_expr_(false_branch_expr) {}

Vector<RefPtr<ScalarExpressionNode>> IfExpressionNode::arguments() const {
  Vector<RefPtr<ScalarExpressionNode>> args;
  args.emplace_back(conditional_expr_);
  args.emplace_back(true_branch_expr_);
  args.emplace_back(false_branch_expr_);
  return args;
}

RefPtr<ScalarExpressionNode> IfExpressionNode::conditional() const {
  return conditional_expr_;
}

RefPtr<ScalarExpressionNode> IfExpressionNode::trueBranch() const {
  return true_branch_expr_;
}

RefPtr<ScalarExpressionNode> IfExpressionNode::falseBranch() const {
  return false_branch_expr_;
}

} // namespace csql

