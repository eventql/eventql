/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <eventql/util/option.h>
#include <eventql/sql/qtree/SequentialScanNode.h>
#include <eventql/sql/TableInfo.h>
#include <eventql/sql/expressions/table_expression.h>

namespace csql {

class TableProvider : public RefCounted {
public:

  virtual Option<ScopedPtr<TableExpression>> buildSequentialScan(
      Transaction* ctx,
      RefPtr<SequentialScanNode> seqscan) const = 0;

  virtual void listTables(Function<void (const TableInfo& table)> fn) const = 0;

  virtual Option<TableInfo> describe(const String& table_name) const = 0;

};

}
