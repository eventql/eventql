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
#include <stx/stdtypes.h>
#include <chartsql/qtree/ShowTablesNode.h>
#include <chartsql/runtime/TableExpression.h>
#include <chartsql/runtime/tablerepository.h>

namespace csql {

class ShowTablesStatement : public TableExpression {
public:

  ShowTablesStatement(RefPtr<TableProvider> tables);

  void execute(
      ExecutionContext* context,
      Function<bool (int argc, const SValue* argv)> fn) override;

  Vector<String> columnNames() const override;

  size_t numColumns() const override;

protected:
  RefPtr<TableProvider> tables_;
};

}
