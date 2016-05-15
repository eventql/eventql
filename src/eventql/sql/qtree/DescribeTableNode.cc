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
#include <eventql/sql/qtree/DescribeTableNode.h>

using namespace util;

namespace csql {

DescribeTableNode::DescribeTableNode(
    const String& table_name) :
    table_name_(table_name) {}

Vector<RefPtr<QueryTreeNode>> DescribeTableNode::inputTables() const {
  return Vector<RefPtr<QueryTreeNode>>{};
}

RefPtr<QueryTreeNode> DescribeTableNode::deepCopy() const {
  return new DescribeTableNode(table_name_);
}

const String& DescribeTableNode::tableName() const {
  return table_name_;
}

Vector<String> DescribeTableNode::outputColumns() const {
  return Vector<String> {
    "column_name",
    "type",
    "nullable",
    "description"
  };
}

Vector<QualifiedColumn> DescribeTableNode::allColumns() const {
  Vector<QualifiedColumn> cols;

  for (const auto& c : outputColumns()) {
    QualifiedColumn  qc;
    qc.short_name = c;
    qc.qualified_name = c;
    cols.emplace_back(qc);
  }

  return cols;
}

size_t DescribeTableNode::getColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  return -1;
}

String DescribeTableNode::toString() const {
  return StringUtil::format("(describe-table $0)", table_name_);;
}

void DescribeTableNode::encode(
    QueryTreeCoder* coder,
    const DescribeTableNode& node,
    util::OutputStream* os) {
  os->appendLenencString(node.table_name_);
}

RefPtr<QueryTreeNode> DescribeTableNode::decode (
    QueryTreeCoder* coder,
    util::InputStream* is) {
  return new DescribeTableNode(is->readLenencString());
}


} // namespace csql
