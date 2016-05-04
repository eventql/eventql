/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/qtree/ShowTablesNode.h>

using namespace stx;

namespace csql {

Vector<RefPtr<QueryTreeNode>> ShowTablesNode::inputTables() const {
  return Vector<RefPtr<QueryTreeNode>>{};
}

RefPtr<QueryTreeNode> ShowTablesNode::deepCopy() const {
  return new ShowTablesNode();
}

Vector<String> ShowTablesNode::outputColumns() const {
  return Vector<String>{
    "table_name",
    "description"
  };
}

Vector<QualifiedColumn> ShowTablesNode::allColumns() const {
  Vector<QualifiedColumn> cols;

  for (const auto& c : outputColumns()) {
    QualifiedColumn  qc;
    qc.short_name = c;
    qc.qualified_name = c;
    cols.emplace_back(qc);
  }

  return cols;
}

Vector<TaskID> ShowTablesNode::build(Transaction* txn, TaskDAG* tree) const {
  RAISE(kNotYetImplementedError, "not yet implemented");
}

size_t ShowTablesNode::getColumnIndex(
    const String& column_name,
    bool allow_add /* = false */) {
  return -1;
}

String ShowTablesNode::toString() const {
  return "(show-tables)";
}

} // namespace csql
