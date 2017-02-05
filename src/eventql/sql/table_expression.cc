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

ReturnCode TableExpression::execute() {
  return ReturnCode::success();
}

bool TableExpression::next(SValue* out, size_t out_len) {
  std::vector<SVector> column_buffers;
  for (size_t i = 0; i < getColumnCount(); ++i) {
    column_buffers.emplace_back(getColumnType(i));
  }

  size_t nrecords = 0;
  auto rc = nextBatch(1, column_buffers.data(), &nrecords);
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

    *out = SValue(getColumnType(i));
    out->copyFrom(column_buffers[i].getData());
  }

  return true;
}

} // namespace csql

