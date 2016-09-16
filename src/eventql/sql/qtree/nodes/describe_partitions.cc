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

DescribePartitionsNode::DescribePartitionsNode(
    const DescribePartitionsNode& node) {}

const String& DescribePartitionsNode::getTableName() const {
  return table_name_;
}

RefPtr<QueryTreeNode> DescribePartitionsNode::deepCopy() const {
  return new DescribePartitionsNode(*this);
}

String DescribePartitionsNode::toString() const {
  return "(describe-partitions)";
}

} // namespace csql


