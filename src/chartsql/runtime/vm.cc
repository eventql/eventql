/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <chartsql/runtime/compiler.h>
#include <chartsql/svalue.h>
#include <chartsql/runtime/vm.h>
#include <stx/exception.h>

namespace csql {

VM::Program::Program(
    Instruction* entry,
    ScratchMemory&& static_storage,
    size_t dynamic_storage_size) :
    entry_(entry),
    static_storage_(std::move(static_storage)),
    dynamic_storage_size_(dynamic_storage_size),
    has_aggregate_(false) {
  VM::initProgram(this, entry_);
}

VM::Program::~Program() {
  VM::freeProgram(this, entry_);
}

VM::Instance VM::allocInstance(
    const Program* program,
    ScratchMemory* scratch) {
  Instance that;

  if (program->has_aggregate_) {
    that.scratch = scratch->alloc(program->dynamic_storage_size_);
    initInstance(program, program->entry_, &that);
  } else {
    that.scratch = scratch->construct<SValue>();
  }

  return that;
}

void VM::freeInstance(
    const Program* program,
    Instance* instance) {
  if (program->has_aggregate_) {
    freeInstance(program, program->entry_, instance);
  } else {
    ((SValue*) instance->scratch)->~SValue();
  }
}

void VM::reset(
    const Program* program,
    Instance* instance) {
  if (program->has_aggregate_) {
    resetInstance(program, program->entry_, instance);
  } else {
    *((SValue*) instance->scratch) = SValue();
  }
}

void VM::result(
    const Program* program,
    const Instance* instance,
    SValue* out) {
  if (program->has_aggregate_) {
    return evaluate(
        program,
        const_cast<Instance*>(instance),
        program->entry_,
        0,
        nullptr,
        out);
  } else {
    *out = *((SValue*) instance->scratch);
  }
}

void VM::accumulate(
    const Program* program,
    Instance* instance,
    int argc,
    const SValue* argv) {
  if (program->has_aggregate_) {
    return accumulate(program, instance, program->entry_, argc, argv);
  } else {
    return evaluate(
        program,
        nullptr,
        program->entry_,
        argc,
        argv,
        (SValue*) instance->scratch);
  }
}

void VM::evaluate(
    const Program* program,
    int argc,
    const SValue* argv,
    SValue* out) {
  return evaluate(program, nullptr, program->entry_, argc, argv, out);
}

void VM::merge(
    const Program* program,
    Instance* dst,
    const Instance* src) {
  if (program->has_aggregate_) {
    mergeInstance(program, program->entry_, dst, src);
  } else {
    *(SValue*) dst->scratch = *(SValue*) src->scratch;
  }
}

void VM::mergeInstance(
    const Program* program,
    Instruction* e,
    Instance* dst,
    const Instance* src) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      e->vtable.t_aggregate.merge(
          (char *) dst->scratch + (size_t) e->arg0,
          (char *) src->scratch + (size_t) e->arg0);
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    mergeInstance(program, cur, dst, src);
  }
}

void VM::initInstance(
    const Program* program,
    Instruction* e,
    Instance* instance) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      if (e->vtable.t_aggregate.init) {
        e->vtable.t_aggregate.init(
            (char *) instance->scratch + (size_t) e->arg0);
      }
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    initInstance(program, cur, instance);
  }
}

void VM::freeInstance(
    const Program* program,
    Instruction* e,
    Instance* instance) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      if (e->vtable.t_aggregate.free) {
        e->vtable.t_aggregate.free(
            (char *) instance->scratch + (size_t) e->arg0);
      }
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    freeInstance(program, cur, instance);
  }
}

void VM::resetInstance(
    const Program* program,
    Instruction* e,
    Instance* instance) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      e->vtable.t_aggregate.reset(
          (char *) instance->scratch + (size_t) e->arg0);
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    resetInstance(program, cur, instance);
  }
}

void VM::initProgram(
    Program* program,
    Instruction* e) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      program->has_aggregate_ = true;
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    initProgram(program, cur);
  }
}

void VM::freeProgram(
    const Program* program,
    Instruction* e) {
  switch (e->type) {
    case X_LITERAL:
      ((SValue*) e->arg0)->~SValue();
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    freeProgram(program, cur);
  }
}

void VM::evaluate(
    const Program* program,
    Instance* instance,
    Instruction* expr,
    int argc,
    const SValue* argv,
    SValue* out) {

  /* execute expression */
  switch (expr->type) {

    case X_IF: {
      SValue cond;
      auto cond_expr = expr->child;
      evaluate(program, instance, cond_expr, argc, argv, &cond);

      auto branch = cond_expr->next;
      if (!cond.getBoolWithConversion()) {
        branch = branch->next;
      }

      evaluate(program, instance, branch, argc, argv, out);
      return;
    }

    case X_CALL_PURE: {
      SValue* stackv = nullptr;
      auto stackn = expr->argn;
      if (stackn > 0) {
        stackv = reinterpret_cast<SValue*>(alloca(sizeof(SValue) * expr->argn));
        for (int i = 0; i < stackn; ++i) {
          new (stackv + i) SValue();
        }

        try {
          auto stackp = stackv;
          for (auto cur = expr->child; cur != nullptr; cur = cur->next) {
            evaluate(program, instance, cur, argc, argv, stackp++);
          }

          expr->vtable.t_pure.call(stackn, stackv, out);
        } catch (...) {
          for (int i = 0; i < stackn; ++i) {
            (stackv + i)->~SValue();
          }

          throw;
        };

        for (int i = 0; i < stackn; ++i) {
          (stackv + i)->~SValue();
        }
      }

      return;
    }

    case X_CALL_AGGREGATE: {
      if (!instance) {
        RAISE(
            kIllegalArgumentError,
            "non-static expression called without instance pointer");
      }

      auto scratch = (char *) instance->scratch + (size_t) expr->arg0;
      expr->vtable.t_aggregate.get(scratch, out);
      return;
    }

    case X_LITERAL: {
      *out = *static_cast<SValue*>(expr->arg0);
      return;
    }

    case X_INPUT: {
      auto index = reinterpret_cast<uint64_t>(expr->arg0);

      if (index >= argc) {
        RAISE(kRuntimeError, "invalid row index %i", index);
      }

      *out = argv[index];
      return;
    }

  }

}

void VM::accumulate(
    const Program* program,
    Instance* instance,
    Instruction* expr,
    int argc,
    const SValue* argv) {

  switch (expr->type) {

    case X_CALL_AGGREGATE: {
      SValue* stackv = nullptr;
      auto stackn = expr->argn;
      if (stackn > 0) {
        stackv = reinterpret_cast<SValue*>(
            alloca(sizeof(SValue) * expr->argn));

        for (int i = 0; i < stackn; ++i) {
          new (stackv + i) SValue();
        }

        auto stackp = stackv;
        for (auto cur = expr->child; cur != nullptr; cur = cur->next) {
          evaluate(
              program,
              instance,
              cur,
              argc,
              argv,
              stackp++);
        }
      }

      auto scratch = (char *) instance->scratch + (size_t) expr->arg0;
      expr->vtable.t_aggregate.accumulate(scratch, stackn, stackv);
      return;
    }

    default: {
      for (auto cur = expr->child; cur != nullptr; cur = cur->next) {
        accumulate(program, instance, cur, argc, argv);
      }

      return;
    }

  }
}

void VM::saveState(
    const Program* program,
    const Instance* instance,
    OutputStream* os) {
  if (program->has_aggregate_) {
    saveInstance(program, program->entry_, instance, os);
  } else {
    ((SValue*) instance->scratch)->encode(os);
  }
}

void VM::loadState(
    const Program* program,
    Instance* instance,
    InputStream* is) {
  if (program->has_aggregate_) {
    loadInstance(program, program->entry_, instance, is);
  } else {
    ((SValue*) instance->scratch)->decode(is);
  }
}

void VM::saveInstance(
    const Program* program,
    Instruction* e,
    const Instance* instance,
    OutputStream* os) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      e->vtable.t_aggregate.savestate(
          (char *) instance->scratch + (size_t) e->arg0,
          os);
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    saveInstance(program, cur, instance, os);
  }
}

void VM::loadInstance(
    const Program* program,
    Instruction* e,
    Instance* instance,
    InputStream* os) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      e->vtable.t_aggregate.loadstate(
          (char *) instance->scratch + (size_t) e->arg0,
          os);
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    loadInstance(program, cur, instance, os);
  }
}


}
