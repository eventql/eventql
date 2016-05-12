/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/qtree/OrderByNode.h>

using namespace stx;

namespace csql {

OrderByNode::OrderByNode(
    Vector<SortSpec> sort_specs,
    RefPtr<QueryTreeNode> table) :
    sort_specs_(sort_specs),
    table_(table) {
  addChild(&table_);
}

RefPtr<QueryTreeNode> OrderByNode::inputTable() const {
  return table_;
}

Vector<String> OrderByNode::outputColumns() const {
  return table_.asInstanceOf<TableExpressionNode>()->outputColumns();
}

Vector<QualifiedColumn> OrderByNode::allColumns() const {
  return table_.asInstanceOf<TableExpressionNode>()->allColumns();
}

size_t OrderByNode::getColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  return table_.asInstanceOf<TableExpressionNode>()->getColumnIndex(
      column_name,
      allow_add);
}

const Vector<OrderByNode::SortSpec>& OrderByNode::sortSpecs() const {
  return sort_specs_;
}

RefPtr<QueryTreeNode> OrderByNode::deepCopy() const {
  return new OrderByNode(
      sort_specs_,
      table_->deepCopy().asInstanceOf<QueryTreeNode>());
}

String OrderByNode::toString() const {
  String str = "(order-by";
  for (const auto& spec : sort_specs_) {
    str += StringUtil::format(
        " (sort-spec $0 $1)",
        spec.expr->toSQL(),
        spec.descending ? "DESC" : "ASC");
  }

  str += " (subexpr " + table_->toString() + "))";

  return str;
}

void OrderByNode::encode(
    QueryTreeCoder* coder,
    const OrderByNode& node,
    stx::OutputStream* os) {
  os->appendUInt64(node.sort_specs_.size());
  for (const auto& spec : node.sort_specs_) {
    coder->encode(spec.expr.get(), os);
    os->appendUInt8(spec.descending);
  }

  coder->encode(node.table_, os);
}

RefPtr<QueryTreeNode> OrderByNode::decode (
    QueryTreeCoder* coder,
    stx::InputStream* is) {
  Vector<SortSpec> sort_specs;
  auto num_sort_specs = is->readUInt64();

  for (auto i = 0; i < num_sort_specs; ++i) {
    SortSpec spec;
    spec.expr = coder->decode(is).asInstanceOf<ValueExpressionNode>();
    spec.descending = (bool) is->readUInt8();
    sort_specs.emplace_back(spec);
  }

  auto table = coder->decode(is);

  return new OrderByNode(sort_specs, table);
}


} // namespace csql
