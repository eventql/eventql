/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/tasks/show_tables.h>
#include <csql/Transaction.h>

namespace csql {

ShowTables::ShowTables(
    Transaction* txn) :
    txn_(txn) {}

bool ShowTables::nextRow(SValue* out, int out_len) {
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

RefPtr<Task> ShowTablesFactory::build(
    Transaction* txn,
    HashMap<TaskID, ScopedPtr<ResultCursor>> input) const {
  return new ShowTables(txn);
}

}
