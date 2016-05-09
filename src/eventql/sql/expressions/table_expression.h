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
#include <eventql/sql/runtime/ExecutionContext.h>
#include <eventql/sql/runtime/Statement.h>

using namespace stx;

namespace csql {

class TableExpression : public Statement {
public:

  virtual ScopedPtr<ResultCursor> execute() = 0;

};

}
