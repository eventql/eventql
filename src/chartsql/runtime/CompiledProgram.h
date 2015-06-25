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
#include <chartsql/svalue.h>
#include <chartsql/runtime/ScratchMemory.h>

using namespace fnord;

namespace csql {

enum kCompiledExpressionType {
  X_CALL,
  X_LITERAL,
  X_INPUT,
  X_MULTI
};

struct CompiledExpression {
  kCompiledExpressionType type;
  void (*call)(void*, int, SValue*, SValue*);
  void* arg0;
  CompiledExpression* next;
  CompiledExpression* child;
  ScalarExpression* expr;
};

class CompiledProgram {
public:

  CompiledProgram(CompiledExpression* expr, size_t scratchpad_size);

  void* alloc(ScratchMemory* scratch) const;

  void init(void* scratchpad);
  void free(void* scratchpad);

  void accumulate(
      void* scratchpad,
      int argc,
      const SValue* argv);

  void evaluate(
      void* scratchpad,
      int argc,
      const SValue* argv,
      SValue* out);

  void reset(void* scratchpad);

protected:
  CompiledExpression* expr_;
  size_t scratchpad_size_;
};

}
