/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/qtree/DescribeTableNode.h>

using namespace stx;

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

Vector<TaskID> DescribeTableNode::build(Transaction* txn, TaskDAG* tree) const {
  RAISE(kNotYetImplementedError, "not yet implemented");
}

String DescribeTableNode::toString() const {
  return StringUtil::format("(describe-table $0)", table_name_);;
}

} // namespace csql
