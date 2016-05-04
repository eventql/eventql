/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/UnixTime.h>
#include <csql/csql.h>
#include <csql/runtime/tablerepository.h>

using namespace stx;

namespace csql {
class Runtime;
class SymbolTable;

class Transaction {
public:

  static inline sql_txn* get(Transaction* ctx) {
    return (sql_txn*) ctx;
  }

  Transaction(Runtime* runtime);

  UnixTime now() const;

  Runtime* getRuntime() const;

  SymbolTable* getSymbolTable() const;

  void setTableProvider(RefPtr<TableProvider> provider);
  RefPtr<TableProvider> getTableProvider() const;

protected:
  Runtime* runtime_;
  UnixTime now_;
  RefPtr<TableProvider> table_provider_;
};


} // namespace csql
