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
#include <eventql/sql/qtree/UnionNode.h>

using namespace util;

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

Vector<String> UnionNode::outputColumns() const {
  if (tables_.empty()) {
    return Vector<String>{};
  } else {
    return tables_[0].asInstanceOf<TableExpressionNode>()->outputColumns();
  }
}

Vector<QualifiedColumn> UnionNode::allColumns() const {
  if (tables_.empty()) {
    return Vector<QualifiedColumn>{};
  } else {
    return tables_[0].asInstanceOf<TableExpressionNode>()->allColumns();
  }
}

size_t UnionNode::getColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  size_t idx = -1;

  for (auto& tbl : tables_) {
    auto tidx = tbl.asInstanceOf<TableExpressionNode>()->getColumnIndex(
        column_name,
        allow_add);

    if (idx != size_t(-1) && tidx != idx) {
      RAISEF(
          kRuntimeError,
          "column not found in UNION tables: '$0'",
          column_name);
    }

    idx = tidx;
  }

  return idx;
}

RefPtr<QueryTreeNode> UnionNode::deepCopy() const {
  return new UnionNode(*this);
}

String UnionNode::toString() const {
  String str = "(union";

  for (const auto& tbl : tables_) {
    str += " (subexpr " + tbl->toString() + ")";
  }

  str += ")";
  return str;
}

} // namespace csql
