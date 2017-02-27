/**
 * Copyright (c) 2017 DeepCortex GmbH <legal@eventql.io>
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
#include "eventql/sql/qtree/nodes/create_partition.h"

namespace csql {

CreatePartitionNode::CreatePartitionNode(
    const std::string& partition_name,
    const std::string& table_name) :
    partition_name_(partition_name),
    table_name_(table_name) {}

CreatePartitionNode::CreatePartitionNode(const CreatePartitionNode& node) {};

const std::string& CreatePartitionNode::getPartitionName() const {
  return partition_name_;
}

const std::string& CreatePartitionNode::getTableName() const {
  return table_name_;
}

RefPtr<QueryTreeNode> CreatePartitionNode::deepCopy() const {
  return new CreatePartitionNode(*this);
}

std::string CreatePartitionNode::toString() const {
  return "(create-partition)";
}

} // namespace csql

