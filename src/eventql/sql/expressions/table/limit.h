/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/sql/runtime/defaultruntime.h>

namespace csql {

class Limit : public TableExpression {
public:

  Limit(
      size_t limit,
      size_t offset,
      ScopedPtr<TableExpression> input);

  ScopedPtr<ResultCursor> execute() override;

protected:
  bool next(SValue* row, size_t row_len);

  size_t limit_;
  size_t offset_;
  ScopedPtr<TableExpression> input_;
  size_t counter_;
  ScopedPtr<ResultCursor> input_cursor_;
};

}
