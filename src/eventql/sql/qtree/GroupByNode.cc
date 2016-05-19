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
#include <eventql/sql/qtree/GroupByNode.h>
#include <eventql/sql/qtree/ColumnReferenceNode.h>

#include "eventql/eventql.h"

namespace csql {

GroupByNode::GroupByNode(
    Vector<RefPtr<SelectListNode>> select_list,
    Vector<RefPtr<ValueExpressionNode>> group_exprs,
    RefPtr<QueryTreeNode> table) :
    select_list_(select_list),
    group_exprs_(group_exprs),
    table_(table),
    is_partial_aggregation_(false) {
  for (const auto& sl : select_list_) {
    column_names_.emplace_back(sl->columnName());
  }

  addChild(&table_);
}

GroupByNode::GroupByNode(
    const GroupByNode& other) :
    column_names_(other.column_names_),
    table_(other.table_->deepCopyAs<QueryTreeNode>()) {
  for (const auto& e : other.select_list_) {
    select_list_.emplace_back(e->deepCopyAs<SelectListNode>());
  }

  for (const auto& e : other.group_exprs_) {
    group_exprs_.emplace_back(e->deepCopyAs<ValueExpressionNode>());
  }

  addChild(&table_);
}

Vector<RefPtr<SelectListNode>> GroupByNode::selectList() const {
  return select_list_;
}

Vector<String> GroupByNode::outputColumns() const {
  return column_names_;
}

Vector<QualifiedColumn> GroupByNode::allColumns() const {
  return table_.asInstanceOf<TableExpressionNode>()->allColumns();
}

size_t GroupByNode::getColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  for (size_t i = 0; i < select_list_.size(); ++i) {
    if (select_list_[i]->columnName() == column_name) {
      return i;
    }
  }

  if (!allow_add) {
    return -1;
  }

  auto child_idx = table_
      .asInstanceOf<TableExpressionNode>()
      ->getColumnIndex(column_name, allow_add);

  if (child_idx != size_t(-1)) {
    auto slnode = new SelectListNode(new ColumnReferenceNode(child_idx));
    slnode->setAlias(column_name);
    select_list_.emplace_back(slnode);
    return select_list_.size() - 1;
  }

  return -1;
}

size_t GroupByNode::getNumComputedColumns() const {
  return select_list_.size();
}

Vector<RefPtr<ValueExpressionNode>> GroupByNode::groupExpressions() const {
  return group_exprs_;
}

RefPtr<QueryTreeNode> GroupByNode::inputTable() const {
  return table_;
}

RefPtr<QueryTreeNode> GroupByNode::deepCopy() const {
  return new GroupByNode(*this);
}

String GroupByNode::toString() const {
  String str = "(group-by (select-list";

  for (const auto& e : select_list_) {
    str += " " + e->toString();
  }

  str += ") (group-list";
  for (const auto& e : group_exprs_) {
    str += " " + e->toString();
  }

  str += ") (subexpr " + table_->toString() + "))";

  return str;
}

void GroupByNode::setIsPartialAggreagtion(bool is_partial_aggregation) {
  is_partial_aggregation_ = is_partial_aggregation;
}

bool GroupByNode::isPartialAggregation() const {
  return is_partial_aggregation_;
}


void GroupByNode::encode(
    QueryTreeCoder* coder,
    const GroupByNode& node,
    OutputStream* os) {
  os->appendVarUInt(node.select_list_.size());
  for (const auto& e : node.select_list_) {
    coder->encode(e.get(), os);
  }

  os->appendVarUInt(node.group_exprs_.size());
  for (const auto& e : node.group_exprs_) {
    coder->encode(e.get(), os);
  }

  coder->encode(node.table_, os);

  os->appendUInt8((uint8_t) node.isPartialAggregation());
}

RefPtr<QueryTreeNode> GroupByNode::decode (
    QueryTreeCoder* coder,
    InputStream* is) {
  Vector<RefPtr<SelectListNode>> select_list;
  auto select_list_size = is->readVarUInt();
  for (auto i = 0; i < select_list_size; ++i) {
    select_list.emplace_back(coder->decode(is).asInstanceOf<SelectListNode>());
  }

  Vector<RefPtr<ValueExpressionNode>> group_exprs;
  auto group_exprs_size = is->readVarUInt();
  for (auto i = 0; i < group_exprs_size; ++i) {
    group_exprs.emplace_back(
        coder->decode(is).asInstanceOf<ValueExpressionNode>());
  }

  auto table = coder->decode(is);
  auto is_partial_aggregation = (bool) is->readUInt8();

  auto node = new GroupByNode(select_list, group_exprs, table);
  node->setIsPartialAggreagtion(is_partial_aggregation);
  return node;
}


} // namespace csql
