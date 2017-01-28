/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <assert.h>
#include <eventql/sql/qtree/SubqueryNode.h>
#include <eventql/sql/qtree/ColumnReferenceNode.h>

#include "eventql/eventql.h"

namespace csql {

SubqueryNode::SubqueryNode(
    RefPtr<QueryTreeNode> subquery,
    Vector<RefPtr<SelectListNode>> select_list,
    Option<RefPtr<ValueExpressionNode>> where_expr) :
    subquery_(subquery),
    select_list_(select_list),
    where_expr_(where_expr) {
  for (const auto& sl : select_list_) {
    column_names_.emplace_back(sl->columnName());
  }

  addChild(&subquery_);
}

SubqueryNode::SubqueryNode(
    const SubqueryNode& other) :
    subquery_(other.subquery_->deepCopy()),
    column_names_(other.column_names_) {
  for (const auto& e : other.select_list_) {
    select_list_.emplace_back(e->deepCopyAs<SelectListNode>());
  }

  if (!other.where_expr_.isEmpty()) {
    where_expr_ = Some(
        other.where_expr_.get()->deepCopyAs<ValueExpressionNode>());
  }

  addChild(&subquery_);
}

RefPtr<QueryTreeNode> SubqueryNode::subquery() const {
  return subquery_;
}

const String& SubqueryNode::tableAlias() const {
  return alias_;
}

void SubqueryNode::setTableAlias(const String& alias) {
  alias_ = alias;
}

Vector<RefPtr<SelectListNode>> SubqueryNode::selectList() const {
  return select_list_;
}

Vector<String> SubqueryNode::getResultColumns() const {
  return column_names_;
}

Vector<QualifiedColumn> SubqueryNode::getAvailableColumns() const {
  String qualifier;
  if (!alias_.empty()) {
    qualifier = alias_ + ".";
  }

  Vector<QualifiedColumn> cols;
  for (const auto& c :
        subquery_.asInstanceOf<TableExpressionNode>()->getResultColumns()) {
    QualifiedColumn qc;
    qc.short_name = c;
    qc.qualified_name = qualifier + c;
    cols.emplace_back(qc);
  }

  return cols;
}

size_t SubqueryNode::getComputedColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  auto col = column_name;
  if (!alias_.empty()) {
    if (StringUtil::beginsWith(col, alias_ + ".")) {
      col = col.substr(alias_.size() + 1);
    }
  }

  for (int i = 0; i < column_names_.size(); ++i) {
    if (column_names_[i] == col || column_names_[i] == column_name) {
      return i;
    }
  }

  auto child_idx = subquery_
      .asInstanceOf<TableExpressionNode>()
      ->getComputedColumnIndex(col, false);

  if (child_idx != size_t(-1)) {
    auto slnode = new SelectListNode(
        new ColumnReferenceNode(
            child_idx,
            subquery_.asInstanceOf<TableExpressionNode>()->getColumnType(child_idx)));

    slnode->setAlias(col);
    select_list_.emplace_back(slnode);
    return select_list_.size() - 1;
  }

  return -1;
}

size_t SubqueryNode::getNumComputedColumns() const {
  return select_list_.size();
}

SType SubqueryNode::getColumnType(size_t idx) const {
  assert(idx < select_list_.size());
  return select_list_[idx]->expression()->getReturnType();
}

Option<RefPtr<ValueExpressionNode>> SubqueryNode::whereExpression() const {
  return where_expr_;
}

RefPtr<QueryTreeNode> SubqueryNode::deepCopy() const {
  return new SubqueryNode(*this);
}

String SubqueryNode::toString() const {
  auto str = StringUtil::format(
      "(select (subquery $0) (select-list", subquery_->toString());
  for (const auto& e : select_list_) {
    str += " " + e->toString();
  }
  str += ")";

  if (!where_expr_.isEmpty()) {
    str += StringUtil::format(" (where $0)", where_expr_.get()->toString());
  }

  str += ")";
  return str;
};

void SubqueryNode::encode(
    QueryTreeCoder* coder,
    const SubqueryNode& node,
    OutputStream* os) {
  coder->encode(node.subquery_, os);

  os->appendVarUInt(node.select_list_.size());
  for (const auto& e : node.select_list_) {
    coder->encode(e.get(), os);
  }

  if (!node.where_expr_.isEmpty()) {
    os->appendUInt8(1);
    coder->encode(node.where_expr_.get().get(), os);
  } else {
    os->appendUInt8(0);
  }
}

RefPtr<QueryTreeNode> SubqueryNode::decode (
    QueryTreeCoder* coder,
    InputStream* is) {
  auto subquery = coder->decode(is);

  Vector<RefPtr<SelectListNode>> select_list;
  auto select_list_size = is->readVarUInt();
  for (auto i = 0; i < select_list_size; ++i) {
    select_list.emplace_back(coder->decode(is).asInstanceOf<SelectListNode>());
  }

  Option<RefPtr<ValueExpressionNode>> where_expr;
  if (is->readUInt8()) {
    where_expr = coder->decode(is).asInstanceOf<ValueExpressionNode>();
  }

  return new SubqueryNode(subquery, select_list, where_expr);
}

} // namespace csql
