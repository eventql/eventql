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
#include <stx/stdtypes.h>
#include <csql/runtime/tablerepository.h>
#include <zbase/core/TSDBTableRef.h>
#include <zbase/core/PartitionMap.h>
#include <zbase/core/CSTableIndex.h>
#include <zbase/core/TableConfig.pb.h>
#include <zbase/core/TSDBTableInfo.h>

using namespace stx;

namespace zbase {
class TSDBService;

struct TSDBTableProvider : public csql::TableProvider {
public:

  TSDBTableProvider(
      const String& tsdb_namespace,
      PartitionMap* partition_map,
      CSTableIndex* cstable_index);

  Option<ScopedPtr<csql::TableExpression>> buildSequentialScan(
        RefPtr<csql::SequentialScanNode> node,
        csql::QueryBuilder* runtime) const override;

  void listTables(
      Function<void (const csql::TableInfo& table)> fn) const override;

  Option<csql::TableInfo> describe(const String& table_name) const override;

protected:

  csql::TableInfo tableInfoForTable(const TSDBTableInfo& table) const;

  String tsdb_namespace_;
  PartitionMap* partition_map_;
  CSTableIndex* cstable_index_;
};


} // namespace csql
