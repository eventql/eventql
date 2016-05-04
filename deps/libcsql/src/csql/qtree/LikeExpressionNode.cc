/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/qtree/LikeExpressionNode.h>

using namespace stx;

namespace csql {

LikeExpressionNode::LikeExpressionNode(
    RefPtr<ValueExpressionNode> subject,
    const String& pattern) :
    subject_(subject),
    pattern_(pattern) {}

Vector<RefPtr<ValueExpressionNode>> LikeExpressionNode::arguments() const {
  return Vector<RefPtr<ValueExpressionNode>>{ subject_ };
}

RefPtr<ValueExpressionNode> LikeExpressionNode::subject() const { 
  return subject_;
}

const String& LikeExpressionNode::pattern() const { 
  return pattern_;
}

RefPtr<QueryTreeNode> LikeExpressionNode::deepCopy() const {
  return new LikeExpressionNode(
      subject_->deepCopyAs<ValueExpressionNode>(),
      pattern_);
}

String LikeExpressionNode::toSQL() const {
  return StringUtil::format(
      "($0 LIKE $1)",
      subject_->toSQL(),
      pattern_);
}

} // namespace csql

