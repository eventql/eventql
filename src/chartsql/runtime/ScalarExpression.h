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
#include <chartsql/SFunction.h>
#include <chartsql/runtime/ScratchMemory.h>

using namespace fnord;

namespace csql {

enum kInstructionType {
  X_CALL,
  X_CALL_PURE,
  X_CALL_AGGREGATE,
  X_LITERAL,
  X_INPUT,
  X_MULTI
};

struct Instruction {
  kInstructionType type;
  void* arg0;
  size_t argn;
  Instruction* next;
  Instruction* child;
  union {
    PureFunction t_pure;
    AggregateFunction t_aggregate;
  } vtable;
  void (*call)(void*, int, SValue*, SValue*); // delete me
};

class ScalarExpression {
public:

  struct Instance {
    void* scratch;
  };

  ScalarExpression(Instruction* expr, size_t scratchpad_size);
  ~ScalarExpression();

  Instance allocInstance(ScratchMemory* scratch) const;
  void freeInstance(Instance* instance) const;

  void accumulate(
      Instance* instance,
      int argc,
      const SValue* argv) const;

  void evaluate(
      Instance* instance,
      int argc,
      const SValue* argv,
      SValue* out) const;

  void reset(Instance* instance) const;

  void evaluateStatic(
      int argc,
      const SValue* argv,
      SValue* out) const;

protected:

  void evaluate(
      Instance* instance,
      Instruction* expr,
      int argc,
      const SValue* argv,
      SValue* out) const;

  void accumulate(
      Instance* instance,
      Instruction* expr,
      int argc,
      const SValue* argv) const;

  void init(Instruction* e, Instance* instance) const;
  void free(Instruction* e, Instance* instance) const;
  void reset(Instruction* e, Instance* instance) const;

  Instruction* expr_;
  size_t scratchpad_size_;
};

}
