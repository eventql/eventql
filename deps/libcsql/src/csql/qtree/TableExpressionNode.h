/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/option.h>
#include <csql/qtree/QueryTreeNode.h>
#include <csql/tasks/TaskDAG.h>

using namespace stx;

namespace csql {
class Transaction;

struct QualifiedColumn {
  String qualified_name;
  String short_name;
};

class TableExpressionNode : public QueryTreeNode {
public:

  virtual Vector<String> outputColumns() const = 0;

  virtual Vector<QualifiedColumn> allColumns() const = 0;

  virtual size_t getColumnIndex(
      const String& column_name,
      bool allow_add = false) = 0;

  size_t numColumns() const;

  virtual Vector<TaskID> build(Transaction* txn, TaskDAG* tree) const = 0;

};

} // namespace csql
