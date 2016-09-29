/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/sql/backends/csv/CSVTableScan.h>

#include "eventql/eventql.h"

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
