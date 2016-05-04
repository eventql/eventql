/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/wallclock.h>
#include <csql/Transaction.h>
#include <csql/runtime/runtime.h>

using namespace stx;

namespace csql {

Transaction::Transaction(
    Runtime* runtime) :
    runtime_(runtime),
    now_(WallClock::now()) {}

Runtime* Transaction::getRuntime() const {
  return runtime_;
}

SymbolTable* Transaction::getSymbolTable() const {
  return runtime_->symbols();
}

UnixTime Transaction::now() const {
  return now_;
}

void Transaction::setTableProvider(RefPtr<TableProvider> provider) {
  table_provider_ = provider;
}

RefPtr<TableProvider> Transaction::getTableProvider() const {
  if (table_provider_.get() == nullptr) {
    RAISE(kRuntimeError, "no table provider configured");
  }

  return table_provider_;
}

} // namespace csql
