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
#include <eventql/sql/qtree/LikeExpressionNode.h>

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

