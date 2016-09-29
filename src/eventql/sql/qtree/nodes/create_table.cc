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
#include "eventql/eventql.h"
#include <eventql/sql/qtree/nodes/create_table.h>

namespace csql {

CreateTableNode::CreateTableNode(
    const String& table_name,
    TableSchema table_schema) :
    table_name_(table_name),
    table_schema_(std::move(table_schema)) {}

CreateTableNode::CreateTableNode(
    const CreateTableNode& node) :
    table_schema_(node.table_schema_) {}

const String& CreateTableNode::getTableName() const {
  return table_name_;
}

const TableSchema& CreateTableNode::getTableSchema() const {
  return table_schema_;
}

const Vector<String> CreateTableNode::getPrimaryKey() const {
  return primary_key_;
}

void CreateTableNode::setPrimaryKey(const Vector<String>& columns) {
  primary_key_ = columns;
}

RefPtr<QueryTreeNode> CreateTableNode::deepCopy() const {
  return new CreateTableNode(*this);
}

String CreateTableNode::toString() const {
  return "(create-table)";
}

const std::vector<std::pair<std::string, std::string>>&
    CreateTableNode::getProperties() const {
  return properties_;
}

void CreateTableNode::addProperty(
    const std::string& key,
    const std::string& value) {
  properties_.emplace_back(key, value);
}

} // namespace csql

