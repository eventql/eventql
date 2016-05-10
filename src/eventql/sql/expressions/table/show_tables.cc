/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/expressions/table/show_tables.h>
#include <eventql/sql/Transaction.h>

namespace csql {

ShowTables::ShowTables(
    Transaction* txn) :
    txn_(txn) {}

ScopedPtr<ResultCursor> ShowTables::execute() {
  RAISE(kNotYetImplementedError, "nyi");
}


bool ShowTables::next(SValue* row, size_t row_len) {
  return false;
}

//void ShowTables::onInputsReady() {
//  auto table_provider = txn_->getTableProvider();
//
//  table_provider->listTables([this] (const TableInfo& table) {
//    Vector<SValue> row;
//    row.emplace_back(table.table_name);
//
//    if (table.description.isEmpty()) {
//      row.emplace_back();
//    } else {
//      row.emplace_back(table.description.get());
//    }
//
//    input_(row.data(), row.size());
//  });
//}


}
