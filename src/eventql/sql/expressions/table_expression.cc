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
#include "eventql/sql/expressions/table_expression.h"

namespace csql {

ReturnCode TableExpression::nextBatch(
    SVector** columns,
    size_t* nrecords) {
  return ReturnCode::error("ERUNTIME", "not yet implemented");
}

bool TableExpression::next(SValue* out, size_t out_len) {
  std::vector<std::unique_ptr<SVector>> column_buffers;
  std::vector<SVector*> column_buffer_ptrs;
  for (size_t i = 0; i < getColumnCount(); ++i) {
    column_buffers.emplace_back(new SVector());
    column_buffer_ptrs.emplace_back(column_buffers.back().get());
  }

  size_t nrecords = 1;
  auto rc = nextBatch(column_buffer_ptrs.data(), &nrecords);
  if (!rc.isSuccess()) {
    RAISE(kRuntimeError, rc.getMessage());
  }

  if (nrecords == 0) {
    return false;
  }

  for (size_t i = 0; i < getColumnCount(); ++i) {
    if (i >= out_len) {
      break;
    }

    switch (getColumnType(i)) {

      case SType::UINT64:
        out[i] = SValue::newUInt64(
            *((const uint64_t*) column_buffers[i]->getData()));
        break;

      case SType::TIMESTAMP64:
        out[i] = SValue::newTimestamp(
            *((const uint64_t*) column_buffers[i]->getData()));
        break;

      case SType::INT64:
        out[i] = SValue::newInt64(
            *((const int64_t*) column_buffers[i]->getData()));
        break;

      case SType::FLOAT64:
        out[i] = SValue::newFloat(
            *((const double*) column_buffers[i]->getData()));
        break;

      default:
        assert(false);
    }
  }

  return true;
}

} // namespace csql

