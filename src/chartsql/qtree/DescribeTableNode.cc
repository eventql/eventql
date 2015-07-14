/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/qtree/DescribeTableNode.h>

using namespace fnord;

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

} // namespace csql
