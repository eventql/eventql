/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include "eventql/eventql.h"
#include <eventql/server/sql_service.h>
#include <eventql/server/sql/table_provider.h>
#include <eventql/sql/runtime/runtime.h>

namespace eventql {

SQLService::SQLService(
    csql::Runtime* sql,
    PartitionMap* pmap,
    ConfigDirectory* cdir,
    ReplicationScheme* repl,
    InternalAuth* auth,
    TableService* table_service) :
    sql_(sql),
    pmap_(pmap),
    cdir_(cdir),
    repl_(repl),
    auth_(auth),
    table_service_(table_service) {}

ScopedPtr<csql::Transaction> SQLService::startTransaction(Session* session) {
  auto txn = sql_->newTransaction();
  txn->setUserData(session);
  txn->setTableProvider(
      new TSDBTableProvider(
          session->getEffectiveNamespace(),
          pmap_,
          cdir_,
          repl_,
          table_service_,
          auth_));

  return txn;
}

} // namespace eventql

