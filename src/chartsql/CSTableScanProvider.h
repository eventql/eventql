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
#include <chartsql/runtime/tablerepository.h>
#include <cstable/CSTableReader.h>

using namespace fnord;

namespace csql {

struct CSTableScanProvider : public TableProvider {
public:

  CSTableScanProvider(
      const String& table_name,
      const String& cstable_file);

  Option<ScopedPtr<TableExpression>> buildSequentialScan(
        RefPtr<SequentialScanNode> node,
        QueryBuilder* runtime) const override;

protected:
  const String table_name_;
  const String cstable_file_;
};


} // namespace csql
