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
#include <stdlib.h>
#include <string>
#include <vector>
#include <assert.h>
#include <csql/svalue.h>
#include <csql/parser/token.h>
#include <csql/parser/astnode.h>

namespace csql {

class RowSink : public RefCounted {
public:
  virtual bool nextRow(SValue* row, int row_len) { return true; };
};

}
