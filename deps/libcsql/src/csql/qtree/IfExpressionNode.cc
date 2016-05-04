/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/qtree/IfExpressionNode.h>

using namespace stx;

namespace csql {

IfExpressionNode::IfExpressionNode(
    RefPtr<ValueExpressionNode> conditional_expr,
    RefPtr<ValueExpressionNode> true_branch_expr,
    RefPtr<ValueExpressionNode> false_branch_expr) :
    conditional_expr_(conditional_expr),
    true_branch_expr_(true_branch_expr),
    false_branch_expr_(false_branch_expr) {}

Vector<RefPtr<ValueExpressionNode>> IfExpressionNode::arguments() const {
  Vector<RefPtr<ValueExpressionNode>> args;
  args.emplace_back(conditional_expr_);
  args.emplace_back(true_branch_expr_);
  args.emplace_back(false_branch_expr_);
  return args;
}

RefPtr<ValueExpressionNode> IfExpressionNode::conditional() const {
  return conditional_expr_;
}

RefPtr<ValueExpressionNode> IfExpressionNode::trueBranch() const {
  return true_branch_expr_;
}

RefPtr<ValueExpressionNode> IfExpressionNode::falseBranch() const {
  return false_branch_expr_;
}

RefPtr<QueryTreeNode> IfExpressionNode::deepCopy() const {
  return new IfExpressionNode(
      conditional_expr_->deepCopyAs<ValueExpressionNode>(),
      true_branch_expr_->deepCopyAs<ValueExpressionNode>(),
      false_branch_expr_->deepCopyAs<ValueExpressionNode>());
}

String IfExpressionNode::toSQL() const {
  return StringUtil::format(
      "if($0, $1, $2)",
      conditional_expr_->toSQL(),
      true_branch_expr_->toSQL(),
      false_branch_expr_->toSQL());
}

} // namespace csql

