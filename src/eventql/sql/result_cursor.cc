/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2016 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/result_cursor.h>

using namespace stx;

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

}
