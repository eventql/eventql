/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <eventql/sql/runtime/tablerepository.h>
#include <eventql/auth/internal_auth.h>
#include <eventql/db/TSDBTableRef.h>
#include <eventql/db/partition_map.h>
#include <eventql/db/compaction_worker.h>
#include <eventql/db/TableConfig.pb.h>
#include <eventql/db/TSDBTableInfo.h>
#include <eventql/db/table_service.h>
#include "eventql/server/sql/table_scan.h"
#include <eventql/db/metadata_client.h>

namespace eventql {
class TableService;

struct TSDBTableProvider : public csql::TableProvider {
public:

  TSDBTableProvider(
      const String& tsdb_namespace,
      PartitionMap* partition_map,
      ConfigDirectory* cdir,
      TableService* table_service,
      InternalAuth* auth);

  Option<ScopedPtr<csql::TableExpression>> buildSequentialScan(
      csql::Transaction* ctx,
      csql::ExecutionContext* execution_context,
      RefPtr<csql::SequentialScanNode> seqscan) const override;

  static KeyRange findKeyRange(
      KeyspaceType keyspace,
      const String& partition_key,
      const Vector<csql::ScanConstraint>& constraints);

  void listTables(
      Function<void (const csql::TableInfo& table)> fn) const override;

  Option<csql::TableInfo> describe(const String& table_name) const override;

  Status createTable(const csql::CreateTableNode& req) override;
  Status createDatabase(const String& database_name) override;
  Status alterTable(const csql::AlterTableNode& alter_table) override;

  Status insertRecord(
      const String& table_name,
      Vector<Pair<String, csql::SValue>> data) override;

  Status insertRecord(
      const String& table_name,
      const String& json_str) override;

  const String& getNamespace() const;

protected:

  csql::TableInfo tableInfoForTable(const TSDBTableInfo& table) const;

  RefPtr<csql::ValueExpressionNode> simplifyWhereExpression(
      RefPtr<Table> table,
      const String& keyrange_begin,
      const String& keyrange_end,
      RefPtr<csql::ValueExpressionNode> expr) const;

  String tsdb_namespace_;
  PartitionMap* partition_map_;
  ConfigDirectory* cdir_;
  TableService* table_service_;
  InternalAuth* auth_;
};

void evqlVersionExpr(sql_txn* ctx, int argc, csql::SValue* argv, csql::SValue* out);

} // namespace csql
