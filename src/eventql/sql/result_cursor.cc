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

DefaultResultCursor::DefaultResultCursor(
    size_t num_columns,
    Function<bool(SValue*, int)> next_fn) :
    num_columns_(num_columns),
    next_fn_(next_fn) {};

size_t DefaultResultCursor::getNumColumns() {
  return num_columns_;
}

bool DefaultResultCursor::next(SValue* row, int row_len) {
  return next_fn_(row, row_len);
}

TableExpressionResultCursor::TableExpressionResultCursor(
    ScopedPtr<TableExpression> table_expression) :
    table_expression_(std::move(table_expression)),
    cursor_(table_expression_->execute()) {}

bool TableExpressionResultCursor::next(SValue* row, int row_len) {
  return cursor_->next(row, row_len);
}

size_t TableExpressionResultCursor::getNumColumns() {
  return cursor_->getNumColumns();
}

bool EmptyResultCursor::next(SValue* row, int row_len) {
  return false;
}

size_t EmptyResultCursor::getNumColumns() {
  return 0;
}

}
