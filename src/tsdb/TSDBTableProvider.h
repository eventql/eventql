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
#include <fnord/stdtypes.h>
#include <chartsql/runtime/tablerepository.h>

using namespace fnord;

namespace tsdb {

struct TSDBTableProvider : public csql::TableProvider {
public:

  TSDBTableProvider(const String& tsdb_namespace);

  Option<ScopedPtr<csql::TableExpression>> buildSequentialScan(
        RefPtr<csql::SequentialScanNode> node,
        csql::Runtime* runtime) const override;

protected:
  String tsdb_namespace_;
};


} // namespace csql
