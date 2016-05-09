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
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/SHA1.h>
#include <eventql/sql/svalue.h>
#include <eventql/sql/runtime/Statement.h>

using namespace stx;

namespace csql {

typedef Function<bool (const SValue* argv, int argc)> RowSinkFn;

class ResultCursor {
public:

  /**
   * Fetch the next row from the cursor. Returns true if a row was returned
   * into the provided storage and false if the last row of the query has been
   * read (EOF). If this method returns false the provided storage will not
   * be changed.
   *
   * This method will block until the next row is available. Use the polling/
   * callback interface below if you need async execution.
   */
  virtual bool next(SValue* row, int row_len) = 0;

  virtual size_t getNumColumns() = 0;

};

class DefaultResultCursor : public ResultCursor {
public:

  DefaultResultCursor(
      size_t num_columns,
      Function<bool(SValue*, int)> next_fn);

  bool next(SValue* row, int row_len) override;

  size_t getNumColumns() override;

protected:
  size_t num_columns_;
  Function<bool(SValue*, int)> next_fn_;
};

}
