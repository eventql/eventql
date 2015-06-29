/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/TableExpressionNode.h>

using namespace fnord;

namespace csql {

size_t TableExpressionNode::numInputTables() const {
  return input_tables_.size();
}

RefPtr<TableExpressionNode> TableExpressionNode::inputTable(size_t index) {
  if (index > input_tables_.size()) {
    RAISE(kIndexError, "invalid table index");
  }

  return *input_tables_[index];
}

RefPtr<TableExpressionNode>* TableExpressionNode::mutableInputTable(
    size_t index) {
  if (index > input_tables_.size()) {
    RAISE(kIndexError, "invalid table index");
  }

  return input_tables_[index];
}

void TableExpressionNode::addInputTable(RefPtr<TableExpressionNode>* table) {
  input_tables_.emplace_back(table);
}

} // namespace csql
