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
#include <eventql/sql/qtree/IfExpressionNode.h>

#include "eventql/eventql.h"

namespace csql {

ReturnCode IfExpressionNode::newNode(
    RefPtr<ValueExpressionNode> conditional_expr,
    RefPtr<ValueExpressionNode> true_branch_expr,
    RefPtr<ValueExpressionNode> false_branch_expr,
  RefPtr<ValueExpressionNode>* node) {
  if (conditional_expr->getReturnType() != SType::BOOL) {
    return ReturnCode::error(
        "EARG",
        "conditional of if statment must return bool");
  }

  if (true_branch_expr->getReturnType() != false_branch_expr->getReturnType()) {
    return ReturnCode::error(
        "EARG",
        "if statement branches return different types");
  }

  auto return_type = true_branch_expr->getReturnType();

  *node = RefPtr<ValueExpressionNode>(
      new IfExpressionNode(
          return_type,
          conditional_expr,
          true_branch_expr,
          false_branch_expr));

  return ReturnCode::success();
}

IfExpressionNode::IfExpressionNode(
    SType return_type,
    RefPtr<ValueExpressionNode> conditional_expr,
    RefPtr<ValueExpressionNode> true_branch_expr,
    RefPtr<ValueExpressionNode> false_branch_expr) :
    return_type_(return_type),
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
      return_type_,
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

SType IfExpressionNode::getReturnType() const {
  return return_type_;
}


void IfExpressionNode::encode(
    QueryTreeCoder* coder,
    const IfExpressionNode& node,
    OutputStream* os) {
  os->appendVarUInt((uint8_t) node.getReturnType());
  coder->encode(node.conditional_expr_.get(), os);
  coder->encode(node.true_branch_expr_.get(), os);
  coder->encode(node.false_branch_expr_.get(), os);
}

RefPtr<QueryTreeNode> IfExpressionNode::decode (
    QueryTreeCoder* coder,
    InputStream* is) {
  auto return_type = (SType) is->readVarUInt();;
  auto conditional_expr = coder->decode(is).asInstanceOf<ValueExpressionNode>();
  auto true_branch_expr = coder->decode(is).asInstanceOf<ValueExpressionNode>();
  auto false_branch_expr = coder->decode(is).asInstanceOf<ValueExpressionNode>();

  return new IfExpressionNode(
      return_type,
      conditional_expr,
      true_branch_expr,
      false_branch_expr);
}

} // namespace csql

