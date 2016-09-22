/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
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
#include <eventql/sql/qtree/nodes/describe_partitions.h>

namespace csql {

DescribePartitionsNode::DescribePartitionsNode(
        const String& table_name) :
        table_name_(table_name) {}

Vector<RefPtr<QueryTreeNode>> DescribePartitionsNode::inputTables() const {
  return Vector<RefPtr<QueryTreeNode>>{};
}

RefPtr<QueryTreeNode> DescribePartitionsNode::deepCopy() const {
  return new DescribePartitionsNode(table_name_);
}

const String& DescribePartitionsNode::tableName() const {
  return table_name_;
}

Vector<String> DescribePartitionsNode::getResultColumns() const {
  return Vector<String> {
    "Partition id",
    "Servers",
    "Keyrange Begin",
    "Keyrange End",
    "Extra info"
  };
}

Vector<QualifiedColumn> DescribePartitionsNode::getAvailableColumns() const {
  Vector<QualifiedColumn> cols;

  for (const auto& c : getResultColumns()) {
    QualifiedColumn  qc;
    qc.short_name = c;
    qc.qualified_name = c;
    cols.emplace_back(qc);
  }

  return cols;
}

size_t DescribePartitionsNode::getComputedColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  return -1; // FIXME
}

size_t DescribePartitionsNode::getNumComputedColumns() const {
  return 4;
}

String DescribePartitionsNode::toString() const {
  return StringUtil::format("(describe-partitions $0)", table_name_);;
}

void DescribePartitionsNode::encode(
    QueryTreeCoder* coder,
    const DescribePartitionsNode& node,
    OutputStream* os) {
  os->appendLenencString(node.table_name_);
}

RefPtr<QueryTreeNode> DescribePartitionsNode::decode (
    QueryTreeCoder* coder,
    InputStream* is) {
  return new DescribePartitionsNode(is->readLenencString());
}

} // namespace csql


