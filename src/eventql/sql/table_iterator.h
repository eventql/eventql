/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stdlib.h>
#include <string>
#include <vector>
#include <assert.h>
#include <eventql/sql/parser/token.h>
#include <eventql/sql/parser/astnode.h>
#include <eventql/sql/runtime/queryplannode.h>
#include <eventql/sql/runtime/tablerepository.h>
#include <eventql/sql/runtime/compiler.h>
#include <eventql/sql/runtime/vm.h>
#include <eventql/util/exception.h>

namespace csql {

class TableIterator {
public:
  virtual bool nextRow(SValue* row) = 0;
  virtual size_t findColumn(const String& name) = 0;
  virtual size_t numColumns() const = 0;
};

} // namespace csql
