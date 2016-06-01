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
#include <eventql/sql/qtree/nodes/alter_table.h>

namespace csql {

AlterTableNode::AlterTableNode(
    const String& table_name,
    const Vector<String> drop_columns,
    const TableSchema::ColumnList add_columns) :
    table_name_(table_name),
    drop_columns_(drop_columns),
    add_columns_(add_columns) {}

AlterTableNode::AlterTableNode(const AlterTableNode& node) {} //FIXME

const String& AlterTableNode::getTableName() const {
  return table_name_;
}

const Vector<String> AlterTableNode::getColumnsToDrop() const {
  return drop_columns_;
}

const TableSchema::ColumnList AlterTableNode::getColumnsToAdd() const {
  return add_columns_;
}

RefPtr<QueryTreeNode> AlterTableNode::deepCopy() const {
  return new AlterTableNode(*this);
}

String AlterTableNode::toString() const {
  return "(alter-table)";
}

} // namespace csql



