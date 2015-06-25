/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/CompiledProgram.h>

using namespace fnord;

namespace csql {

CompiledProgram::CompiledProgram(
    CompiledExpression* expr,
    size_t scratchpad_size) :
    expr_(expr),
    scratchpad_size_(scratchpad_size) {}

CompiledProgram::~CompiledProgram() {
  // FIXPAUL free instructions...
}

CompiledProgram::Instance CompiledProgram::allocInstance(
    ScratchMemory* scratch) const {
  Instance that;
  that.scratch = scratch->alloc(scratchpad_size_);
  init(expr_, &that);
  return that;
}

void CompiledProgram::freeInstance(Instance* instance) const {
  free(expr_, instance);
}

//void accumulate(
//    Instance* instance,
//    int argc,
//    const SValue* argv);
//
//void evaluate(
//    Instance* instance,
//    int argc,
//    const SValue* argv,
//    SValue* out);
//
//void reset(Instance* instance);

}
