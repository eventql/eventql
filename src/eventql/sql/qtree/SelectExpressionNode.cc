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
#include <eventql/sql/qtree/SelectExpressionNode.h>

using namespace stx;

namespace csql {

SelectExpressionNode::SelectExpressionNode(
    Vector<RefPtr<SelectListNode>> select_list) :
    select_list_(select_list) {
  for (const auto& sl : select_list_) {
    column_names_.emplace_back(sl->columnName());
  }
}

Vector<RefPtr<SelectListNode>> SelectExpressionNode::selectList()
    const {
  return select_list_;
}

Vector<String> SelectExpressionNode::outputColumns() const {
  return column_names_;
}

Vector<QualifiedColumn> SelectExpressionNode::allColumns() const {
  String qualifier;

  Vector<QualifiedColumn> cols;
  for (const auto& c : column_names_) {
    QualifiedColumn qc;
    qc.short_name = c;
    qc.qualified_name = qualifier + c;
    cols.emplace_back(qc);
  }

  return cols;
}

size_t SelectExpressionNode::getColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  for (int i = 0; i < column_names_.size(); ++i) {
    if (column_names_[i] == column_name) {
      return i;
    }
  }

  return -1;
}

RefPtr<QueryTreeNode> SelectExpressionNode::deepCopy() const {
  Vector<RefPtr<SelectListNode>> args;
  for (const auto& arg : select_list_) {
    args.emplace_back(arg->deepCopyAs<SelectListNode>());
  }

  return new SelectExpressionNode(args);
}


String SelectExpressionNode::toString() const {
  String str = "(select-expr";

  for (const auto& e : select_list_) {
    str += " " + e->toString();
  }

  str += ")";
  return str;
}

void SelectExpressionNode::encode(
    QueryTreeCoder* coder,
    const SelectExpressionNode& node,
    stx::OutputStream* os) {
  os->appendVarUInt(node.select_list_.size());
  for (const auto& e : node.select_list_) {
    coder->encode(e.get(), os);
  }
}

RefPtr<QueryTreeNode> SelectExpressionNode::decode(
    QueryTreeCoder* coder,
    stx::InputStream* is) {
  Vector<RefPtr<SelectListNode>> select_list;
  auto select_list_size = is->readVarUInt();
  for (auto i = 0; i < select_list_size; ++i) {
    select_list.emplace_back(coder->decode(is).asInstanceOf<SelectListNode>());
  }

  return new SelectExpressionNode(select_list);
}

} // namespace csql
