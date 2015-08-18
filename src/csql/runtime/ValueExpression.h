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
#include <stx/io/inputstream.h>
#include <stx/io/outputstream.h>
#include <csql/svalue.h>
#include <csql/SFunction.h>
#include <csql/runtime/ScratchMemory.h>
#include <csql/runtime/vm.h>

using namespace stx;

namespace csql {

class ValueExpression {
public:

  ValueExpression();
  ValueExpression(ScopedPtr<VM::Program> program);
  ValueExpression(ValueExpression&& move);

  ValueExpression& operator=(ValueExpression&& other);

  VM::Program* program() const;

protected:
  ScopedPtr<VM::Program> program_;
};

}
