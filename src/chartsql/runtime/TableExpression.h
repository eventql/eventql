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
#include <fnord/stdtypes.h>
#include <fnord/autoref.h>
#include <chartsql/svalue.h>
#include <chartsql/runtime/ExecutionContext.h>

using namespace fnord;

namespace csql {

class TableExpression : public RefCounted {
public:

  virtual Vector<String> columnNames() const = 0;

  virtual size_t numColumns() const = 0;

  virtual void execute(
      ExecutionContext* context,
      Function<bool (int argc, const SValue* argv)> fn) = 0;

};

}
