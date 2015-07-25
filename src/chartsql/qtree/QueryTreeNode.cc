/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/QueryTreeNode.h>

using namespace stx;

namespace csql {

size_t QueryTreeNode::numChildren() const {
  return children_.size();
}

RefPtr<QueryTreeNode> QueryTreeNode::child(size_t index) {
  if (index > children_.size()) {
    RAISE(kIndexError, "invalid table index");
  }

  return *children_[index];
}

RefPtr<QueryTreeNode>* QueryTreeNode::mutableChild(
    size_t index) {
  if (index > children_.size()) {
    RAISE(kIndexError, "invalid table index");
  }

  return children_[index];
}

void QueryTreeNode::addChild(RefPtr<QueryTreeNode>* table) {
  children_.emplace_back(table);
}

} // namespace csql

