/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/UnixTime.h>
#include <eventql/util/return_code.h>
#include <eventql/sql/csql.h>
#include <eventql/sql/runtime/tablerepository.h>

#include "eventql/eventql.h"

namespace csql {
class Runtime;
class SymbolTable;
class QueryBuilder;
class TableProvider;
class TableRepository;
class QueryCache;

class Transaction {
public:

  static inline sql_txn* get(Transaction* ctx) {
    return (sql_txn*) ctx;
  }

  Transaction(Runtime* runtime);

  ~Transaction();

  UnixTime now() const;

  Runtime* getRuntime() const;
  QueryBuilder* getCompiler() const;
  SymbolTable* getSymbolTable() const;

  void setTableProvider(RefPtr<TableProvider> provider);
  RefPtr<TableProvider> getTableProvider() const;

  void setUserData(
      void* user_data,
      Function<void (void*)> free_fn = [] (void*) {});

  void* getUserData();

  void setHeartbeatCallback(std::function<ReturnCode ()> cb);
  ReturnCode triggerHeartbeat();

protected:
  Runtime* runtime_;
  UnixTime now_;
  RefPtr<TableProvider> table_provider_;
  void* user_data_;
  Function<void (void*)> free_user_data_fn_;
  std::function<ReturnCode ()> heartbeat_cb_;
};


} // namespace csql
