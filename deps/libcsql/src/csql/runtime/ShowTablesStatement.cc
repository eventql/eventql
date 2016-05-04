/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/runtime/ShowTablesStatement.h>

namespace csql {

ShowTablesStatement::ShowTablesStatement(
    RefPtr<TableProvider> tables) :
    tables_(tables) {}

void ShowTablesStatement::prepare(ExecutionContext* context) {}

void ShowTablesStatement::execute(
    ExecutionContext* context,
    Function<bool (int argc, const SValue* argv)> fn) {
  tables_->listTables([fn] (const TableInfo& table) {
    Vector<SValue> row;
    row.emplace_back(table.table_name);

    if (table.description.isEmpty()) {
      row.emplace_back();
    } else {
      row.emplace_back(table.description.get());
    }

    fn(row.size(), row.data());
  });
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
