/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/backends/csv/CSVTableScan.h>

using namespace stx;

namespace csql {
namespace backends {
namespace csv {

CSVTableScan::CSVTableScan(
    const Vector<String>& headers,
    ScopedPtr<CSVInputStream> csv) :
    headers_(headers),
    csv_(std::move(csv)) {}

bool CSVTableScan::nextRow(SValue* row) {
  Vector<String> inrow;
  if (!csv_->readNextRow(&inrow)) {
    return false;
  }


  auto ncols = std::min(headers_.size(), inrow.size());
  for (size_t i = 0; i < ncols; i++) {
    row[i] = SValue(inrow[i]);
  }

  for (size_t i = ncols; i < headers_.size(); ++i) {
    row[i] = SValue();
  }

  return true;
}

size_t CSVTableScan::findColumn(const String& name) {
  for (size_t i = 0; i < headers_.size(); i++) {
    if (headers_[i] == name) {
      return i;
    }
  }

  return -1;
}

size_t CSVTableScan::numColumns() const {
  return headers_.size();
}

} // namespace csv
} // namespace backends
} // namespace csql
