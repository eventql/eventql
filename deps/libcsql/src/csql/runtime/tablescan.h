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
#include <stdlib.h>
#include <string>
#include <vector>
#include <assert.h>
#include <csql/parser/token.h>
#include <csql/parser/astnode.h>
#include <csql/runtime/queryplannode.h>
#include <csql/runtime/tablerepository.h>
#include <csql/runtime/compiler.h>
#include <csql/runtime/vm.h>
#include <stx/exception.h>

namespace csql {

class TableIterator {
public:
  virtual bool nextRow(SValue* row) = 0;
  virtual size_t findColumn(const String& name) = 0;
  virtual size_t numColumns() const = 0;
};

class TableScan : public Task {
public:

  TableScan(
      Transaction* txn,
      RefPtr<SequentialScanNode> stmt,
      ScopedPtr<TableIterator> iter,
      RowSinkFn output);

  void onInputsReady() override;

protected:

  Transaction* txn_;
  ScopedPtr<TableIterator> iter_;
  Vector<ValueExpression> select_exprs_;
  Option<ValueExpression> where_expr_;
  RowSinkFn output_;
};

} // namespace csql
