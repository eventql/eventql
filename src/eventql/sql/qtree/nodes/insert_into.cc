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
#include "eventql/eventql.h"
#include <eventql/sql/qtree/nodes/insert_into.h>

namespace csql {

InsertIntoNode::InsertIntoNode(
    const String& table_name,
    Vector<InsertIntoNode::InsertValueSpec> values_spec) :
    table_name_(table_name),
    values_spec_(values_spec) {}

InsertIntoNode::InsertIntoNode(
    const InsertIntoNode& node) :
    values_spec_(node.values_spec_) {}

const String& InsertIntoNode::getTableName() const {
  return table_name_;
}

Vector<InsertIntoNode::InsertValueSpec> InsertIntoNode::getValuesSpec() const {
  return values_spec_;
}

RefPtr<QueryTreeNode> InsertIntoNode::deepCopy() const {
  return new InsertIntoNode(*this);
}

String InsertIntoNode::toString() const {
  return "(insert-into)";
}

} // namespace csql


