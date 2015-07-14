
/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/ShowTablesStatement.h>

namespace csql {

ShowTablesStatement::ShowTablesStatement(
    RefPtr<TableProvider> tables) :
    tables_(tables) {}

void ShowTablesStatement::execute(
    ExecutionContext* context,
    Function<bool (int argc, const SValue* argv)> fn) {

}

Vector<String> ShowTablesStatement::columnNames() const {
  return Vector<String> {
    "table_name",
    "description"
  };
}

size_t ShowTablesStatement::numColumns() const {
  return columnNames().size();
}

}
