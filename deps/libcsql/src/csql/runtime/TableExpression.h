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

using namespace stx;

namespace csql {

class TableExpression : public Statement {
public:

  virtual Vector<String> columnNames() const = 0;

  virtual size_t numColumns() const = 0;

  virtual size_t getColumnIndex(const String& column_name) const;

  virtual void execute(
      ExecutionContext* context,
      Function<bool (int argc, const SValue* argv)> fn) = 0;

  virtual Option<SHA1Hash> cacheKey() const {
    return None<SHA1Hash>();
  }

};

}
