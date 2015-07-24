/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/UnionNode.h>

using namespace fnord;

namespace csql {

UnionNode::UnionNode(const UnionNode& other) {
  for (const auto& tbl : other.tables_) {
    tables_.emplace_back(tbl->deepCopy());
  }

  for (auto& table : tables_) {
    addChild(&table);
  }
}

UnionNode::UnionNode(
    Vector<RefPtr<QueryTreeNode>> tables) :
    tables_(tables) {
  for (auto& table : tables_) {
    addChild(&table);
  }
}

Vector<RefPtr<QueryTreeNode>> UnionNode::inputTables() const {
  return tables_;
}

RefPtr<QueryTreeNode> UnionNode::deepCopy() const {
  return new UnionNode(*this);
}

} // namespace csql
