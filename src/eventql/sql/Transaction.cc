/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/util/wallclock.h>
#include <eventql/sql/Transaction.h>
#include <eventql/sql/runtime/runtime.h>

using namespace stx;

namespace csql {

Transaction::Transaction(
    Runtime* runtime) :
    runtime_(runtime),
    now_(WallClock::now()),
    table_providers_(new TableRepository()) {}

Runtime* Transaction::getRuntime() const {
  return runtime_;
}

QueryBuilder* Transaction::getCompiler() const {
  return runtime_->queryBuilder().get();
}

SymbolTable* Transaction::getSymbolTable() const {
  return runtime_->symbols();
}

UnixTime Transaction::now() const {
  return now_;
}

void Transaction::addTableProvider(RefPtr<TableProvider> provider) {
  table_providers_->addProvider(provider);
}

RefPtr<TableProvider> Transaction::getTableProvider() const {
  return table_providers_.get();
}

} // namespace csql
