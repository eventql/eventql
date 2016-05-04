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
#include <csql/runtime/tablescan.h>
#include <csql/backends/csv/CSVInputStream.h>

using namespace stx;

namespace csql {
namespace backends {
namespace csv {

struct CSVTableScan : public TableIterator {
public:

  CSVTableScan(
      const Vector<String>& headers,
      ScopedPtr<CSVInputStream> csv);

  bool nextRow(SValue* row) override;

  size_t findColumn(const String& name) override;
  size_t numColumns() const override;

protected:
  Vector<String> headers_;
  ScopedPtr<CSVInputStream> csv_;
};

} // namespace csv
} // namespace backends
} // namespace csql
