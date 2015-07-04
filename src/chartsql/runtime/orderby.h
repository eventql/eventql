/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <fnord/stdtypes.h>
#include <chartsql/runtime/TableExpression.h>

namespace csql {

class OrderBy : public TableExpression {
public:

  struct SortSpec {
    size_t column;
    bool descending; // false == ASCENDING, true == DESCENDING
  };

  OrderBy(
      Vector<SortSpec> sort_specs,
      ScopedPtr<TableExpression> child);

  //void execute() override;
  //bool nextRow(SValue* row, int row_len) override;
  //size_t getNumCols() const override;

protected:
  Vector<SortSpec> sort_specs_;
  ScopedPtr<TableExpression> child_;
  Vector<Vector<SValue>> rows_;
};

}
