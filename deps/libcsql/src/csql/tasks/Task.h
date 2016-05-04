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
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <stx/SHA1.h>
#include <csql/svalue.h>
#include <csql/runtime/ExecutionContext.h>
#include <csql/runtime/Statement.h>
#include <csql/runtime/rowsink.h>
#include <csql/tasks/TaskID.h>

using namespace stx;

namespace csql {

typedef Function<bool (const SValue* argv, int argc)> RowSinkFn;

class Task : public Statement {
public:

  virtual Option<SHA1Hash> cacheKey() const {
    return None<SHA1Hash>();
  }

  virtual void run() {}
  virtual bool nextRow(SValue* out, int out_len) = 0;

  //virtual void onInputsReady() {}

  //virtual bool onInputRow(
  //    const TaskID& input_id,
  //    const SValue* row,
  //    int row_len) { return true; };

};

}
