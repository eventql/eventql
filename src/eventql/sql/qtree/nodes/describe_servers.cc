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
#include <eventql/sql/qtree/nodes/describe_servers.h>

namespace csql {

Vector<RefPtr<QueryTreeNode>> DescribeServersNode::inputTables() const {
  return Vector<RefPtr<QueryTreeNode>>{};
}

RefPtr<QueryTreeNode> DescribeServersNode::deepCopy() const {
  return new DescribeServersNode();
}

Vector<String> DescribeServersNode::getResultColumns() const {
  return Vector<String> {
    "Name",
    "Status",
    "ListenAddr",
    "BuildInfo",
    "Load",
    "Disk Used",
    "Disk Free",
    "Partitions"
  };
}

Vector<QualifiedColumn> DescribeServersNode::getAvailableColumns() const {
  Vector<QualifiedColumn> cols;

  for (const auto& c : getResultColumns()) {
    QualifiedColumn  qc;
    qc.short_name = c;
    qc.qualified_name = c;
    cols.emplace_back(qc);
  }

  return cols;
}

size_t DescribeServersNode::getComputedColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  return -1; // FIXME
}

size_t DescribeServersNode::getNumComputedColumns() const {
  return 4;
}

String DescribeServersNode::toString() const {
  return "(describe-servers)";
}

void DescribeServersNode::encode(
    QueryTreeCoder* coder,
    const DescribeServersNode& node,
    OutputStream* os) {
}

RefPtr<QueryTreeNode> DescribeServersNode::decode (
    QueryTreeCoder* coder,
    InputStream* is) {
  return new DescribeServersNode();
}

} // namespace csql



