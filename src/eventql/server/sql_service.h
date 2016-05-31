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
#pragma once
#include "eventql/eventql.h"
#include <eventql/util/stdtypes.h>
#include <eventql/db/TableConfig.pb.h>
#include <eventql/db/Partition.h>
#include <eventql/db/TSDBNodeConfig.pb.h>
#include <eventql/db/TSDBTableInfo.h>
#include <eventql/db/PartitionInfo.pb.h>
#include <eventql/db/RecordEnvelope.pb.h>
#include <eventql/db/table_service.h>
#include <eventql/db/partition_map.h>
#include <eventql/db/TimeWindowPartitioner.h>
#include <eventql/sql/transaction.h>
#include <eventql/auth/internal_auth.h>

namespace eventql {

using TableService = TableService;

class SQLService {
public:

  SQLService(
      csql::Runtime* sql,
      PartitionMap* pmap,
      ConfigDirectory* cdir,
      ReplicationScheme* repl,
      InternalAuth* auth,
      TableService* table_service);

  ScopedPtr<csql::Transaction> startTransaction(Session* session);

protected:
  csql::Runtime* sql_;
  PartitionMap* pmap_;
  ConfigDirectory* cdir_;
  ReplicationScheme* repl_;
  InternalAuth* auth_;
  TableService* table_service_;
};

} // namespace eventql

