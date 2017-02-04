/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Christian Parpart <trapni@dawanda.com>
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
#include <eventql/sql/result_cursor.h>
#include <eventql/sql/expressions/table_expression.h>

#include "eventql/eventql.h"

namespace csql {

ResultCursor::ResultCursor() :
    started_(false) {}

ResultCursor::ResultCursor(
    ScopedPtr<TableExpression> table_expression) :
    table_expression_(std::move(table_expression)),
    started_(false) {}

bool ResultCursor::next(SValue* row) {
  if (!table_expression_) {
    return false;
  }

  if (!started_) {
    auto rc = table_expression_->execute();
    if (!rc.isSuccess()) {
      RAISE(kRuntimeError, rc.getMessage());
    }
  }

  return table_expression_->next(row, table_expression_->getColumnCount());
}

size_t ResultCursor::getColumnCount() const {
  return table_expression_->getColumnCount();
}

SType ResultCursor::getColumnType(size_t idx) const {
  return table_expression_->getColumnType(idx);
}

} // namespace csql

