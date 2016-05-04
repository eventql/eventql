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
#include <cstable/CSTableReader.h>

using namespace stx;

namespace csql {

struct CSTableScanProvider : public TableProvider {
public:

  CSTableScanProvider(
      const String& table_name,
      const String& cstable_file);

  TaskIDList buildSequentialScan(
      Transaction* txn,
      RefPtr<SequentialScanNode> seqscan,
      TaskDAG* tasks) const override;

  void listTables(
      Function<void (const csql::TableInfo& table)> fn) const override;

  Option<csql::TableInfo> describe(const String& table_name) const override;

  csql::TableInfo tableInfo() const;

protected:
  const String table_name_;
  const String cstable_file_;
};


} // namespace csql
