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
#include <eventql/sql/svalue.h>

namespace csql {

class TableIterator {
public:
  virtual bool nextRow(SValue* row) = 0;
  virtual size_t findColumn(const String& name) = 0;
  virtual size_t numColumns() const = 0;
};

} // namespace csql
