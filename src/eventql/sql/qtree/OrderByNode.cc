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
#include <eventql/sql/qtree/OrderByNode.h>

#include "eventql/eventql.h"

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

Vector<String> OrderByNode::getResultColumns() const {
  return table_.asInstanceOf<TableExpressionNode>()->getResultColumns();
}

Vector<QualifiedColumn> OrderByNode::getAvailableColumns() const {
  return table_.asInstanceOf<TableExpressionNode>()->getAvailableColumns();
}

size_t OrderByNode::getComputedColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  return table_.asInstanceOf<TableExpressionNode>()->getComputedColumnIndex(
      column_name,
      allow_add);
}

size_t OrderByNode::getNumComputedColumns() const {
  return table_.asInstanceOf<TableExpressionNode>()->getNumComputedColumns();
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
    OutputStream* os) {
  os->appendVarUInt(node.sort_specs_.size());
  for (const auto& spec : node.sort_specs_) {
    coder->encode(spec.expr.get(), os);
    os->appendUInt8(spec.descending);
  }

  coder->encode(node.table_, os);
}

RefPtr<QueryTreeNode> OrderByNode::decode (
    QueryTreeCoder* coder,
    InputStream* is) {
  Vector<SortSpec> sort_specs;
  auto num_sort_specs = is->readVarUInt();

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
