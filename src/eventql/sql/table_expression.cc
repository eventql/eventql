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
#include <assert.h>
#include "eventql/sql/table_expression.h"

namespace csql {

SimpleTableExpression::SimpleTableExpression(
    const std::vector<ColumnDefinition>& columns) :
    columns_(columns),
    row_count_(0) {
  for (const auto& c : columns_) {
    row_data_.emplace_back(c.second);
  }
}

ReturnCode SimpleTableExpression::nextBatch(SVector* columns, size_t* len) {
  if (row_count_ > 0) {
    for (size_t i = 0; i < columns_.size(); ++i) {
      columns[i].copyFrom(&row_data_[i]);
    }

    *len = row_count_;
    row_count_ = 0;
  } else {
    *len = 0;
  }

  return ReturnCode::success();
}

size_t SimpleTableExpression::getColumnCount() const {
  return columns_.size();
}

SType SimpleTableExpression::getColumnType(size_t idx) const {
  assert(idx < columns_.size());
  return columns_[idx].second;
}

void SimpleTableExpression::addRow(const SValue* row) {
  ++row_count_;
  for (size_t i = 0; i < columns_.size(); ++i) {
    row_data_[i].append(row[i]);
  }
}

} // namespace csql

