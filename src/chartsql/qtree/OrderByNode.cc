/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/OrderByNode.h>

using namespace fnord;

namespace csql {

OrderByNode::OrderByNode(
    Vector<SortSpec> sort_specs,
    size_t max_output_column_index,
    RefPtr<TableExpressionNode> table) :
    sort_specs_(sort_specs),
    max_output_column_index_(max_output_column_index),
    table_(table) {
  addInputTable(&table_);
}

RefPtr<TableExpressionNode> OrderByNode::inputTable() const {
  return table_;
}

const Vector<OrderByNode::SortSpec>& OrderByNode::sortSpecs() const {
  return sort_specs_;
}

size_t OrderByNode::maxOutputColumnIndex() const {
  return max_output_column_index_;
}

RefPtr<QueryTreeNode> OrderByNode::deepCopy() const {
  return new OrderByNode(
      sort_specs_,
      max_output_column_index_,
      table_->deepCopy().asInstanceOf<TableExpressionNode>());
}

} // namespace csql
