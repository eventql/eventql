/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
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
#include <eventql/sql/runtime/tablerepository.h>
#include <eventql/auth/internal_auth.h>
#include <eventql/db/TSDBTableRef.h>
#include <eventql/db/partition_map.h>
#include <eventql/db/CompactionWorker.h>
#include <eventql/db/TableConfig.pb.h>
#include <eventql/db/TSDBTableInfo.h>
#include "eventql/server/sql/table_scan.h"

#include "eventql/eventql.h"

namespace eventql {
class TSDBService;

struct TSDBTableProvider : public csql::TableProvider {
public:

  TSDBTableProvider(
      const String& tsdb_namespace,
      PartitionMap* partition_map,
      ReplicationScheme* replication_scheme,
      InternalAuth* auth);

  Option<ScopedPtr<csql::TableExpression>> buildSequentialScan(
      csql::Transaction* ctx,
      csql::ExecutionContext* execution_context,
      RefPtr<csql::SequentialScanNode> seqscan) const override;

  void listTables(
      Function<void (const csql::TableInfo& table)> fn) const override;

  Option<csql::TableInfo> describe(const String& table_name) const override;

  Status createTable(const csql::CreateTableNode& req) override;

  const String& getNamespace() const;

protected:

  csql::TableInfo tableInfoForTable(const TSDBTableInfo& table) const;

  String tsdb_namespace_;
  PartitionMap* partition_map_;
  ReplicationScheme* replication_scheme_;
  InternalAuth* auth_;
};


} // namespace csql
