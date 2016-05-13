/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/sql/runtime/tablerepository.h>
#include <eventql/AnalyticsAuth.h>
#include <eventql/core/TSDBTableRef.h>
#include <eventql/core/PartitionMap.h>
#include <eventql/core/CompactionWorker.h>
#include <eventql/core/TableConfig.pb.h>
#include <eventql/core/TSDBTableInfo.h>
#include "eventql/server/sql/table_scan.h"

using namespace stx;

namespace zbase {
class TSDBService;

struct TSDBTableProvider : public csql::TableProvider {
public:

  TSDBTableProvider(
      const String& tsdb_namespace,
      PartitionMap* partition_map,
      ReplicationScheme* replication_scheme,
      AnalyticsAuth* auth);

  Option<ScopedPtr<csql::TableExpression>> buildSequentialScan(
      csql::Transaction* ctx,
      RefPtr<csql::SequentialScanNode> seqscan) const override;

  void listTables(
      Function<void (const csql::TableInfo& table)> fn) const override;

  Option<csql::TableInfo> describe(const String& table_name) const override;

protected:

  csql::TableInfo tableInfoForTable(const TSDBTableInfo& table) const;

  String tsdb_namespace_;
  PartitionMap* partition_map_;
  ReplicationScheme* replication_scheme_;
  AnalyticsAuth* auth_;
};


} // namespace csql
