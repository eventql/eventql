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
#include <eventql/util/stdtypes.h>
#include <eventql/util/UnixTime.h>
#include <eventql/sql/csql.h>
#include <eventql/sql/runtime/tablerepository.h>

using namespace stx;

namespace csql {
class Runtime;
class SymbolTable;
class QueryBuilder;

class Transaction {
public:

  static inline sql_txn* get(Transaction* ctx) {
    return (sql_txn*) ctx;
  }

  Transaction(Runtime* runtime);

  UnixTime now() const;

  Runtime* getRuntime() const;
  QueryBuilder* getCompiler() const;
  SymbolTable* getSymbolTable() const;

  void addTableProvider(RefPtr<TableProvider> provider);
  RefPtr<TableProvider> getTableProvider() const;

protected:
  Runtime* runtime_;
  UnixTime now_;
  RefPtr<TableRepository> table_providers_;
};


} // namespace csql
