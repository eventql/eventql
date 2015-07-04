/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/LimitNode.h>

using namespace fnord;

namespace csql {

LimitNode::LimitNode(
    size_t limit,
    size_t offset,
    RefPtr<TableExpressionNode> table) :
    limit_(limit),
    offset_(offset),
    table_(table) {
  addInputTable(&table_);
}

RefPtr<TableExpressionNode> LimitNode::inputTable() const {
  return table_;
}

RefPtr<QueryTreeNode> LimitNode::deepCopy() const {
  return new LimitNode(
      limit_,
      offset_,
      table_->deepCopy().asInstanceOf<TableExpressionNode>());
}

size_t LimitNode::limit() const {
  return limit_;
}

size_t LimitNode::offset() const {
  return offset_;
}


} // namespace csql
