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
#include <csql/runtime/compiler.h>
#include <csql/runtime/LikePattern.h>
#include <csql/svalue.h>
#include <csql/runtime/vm.h>
#include <stx/exception.h>
#include <stx/RegExp.h>

#ifndef HAVE_PCRE
#error "PCRE is required"
#endif

namespace csql {

VM::Program::Program(
    Transaction* ctx,
    Instruction* entry,
    ScratchMemory&& static_storage,
    size_t dynamic_storage_size) :
    ctx_(ctx),
    entry_(entry),
    static_storage_(std::move(static_storage)),
    dynamic_storage_size_(dynamic_storage_size),
    has_aggregate_(false) {
  VM::initProgram(ctx_, this, entry_);
}

VM::Program::~Program() {
  VM::freeProgram(ctx_, this, entry_);
}

VM::Instance VM::allocInstance(
    Transaction* ctx,
    const Program* program,
    ScratchMemory* scratch) {
  Instance that;

  if (program->has_aggregate_) {
    that.scratch = scratch->alloc(program->dynamic_storage_size_);
    initInstance(ctx, program, program->entry_, &that);
  } else {
    that.scratch = scratch->construct<SValue>();
  }

  return that;
}

void VM::freeInstance(
    Transaction* ctx,
    const Program* program,
    Instance* instance) {
  if (program->has_aggregate_) {
    freeInstance(ctx, program, program->entry_, instance);
  } else {
    ((SValue*) instance->scratch)->~SValue();
  }
}

void VM::reset(
    Transaction* ctx,
    const Program* program,
    Instance* instance) {
  if (program->has_aggregate_) {
    resetInstance(ctx, program, program->entry_, instance);
  } else {
    *((SValue*) instance->scratch) = SValue();
  }
}

void VM::result(
    Transaction* ctx,
    const Program* program,
    const Instance* instance,
    SValue* out) {
  if (program->has_aggregate_) {
    return evaluate(
        ctx,
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
    Transaction* ctx,
    const Program* program,
    Instance* instance,
    int argc,
    const SValue* argv) {
  if (program->has_aggregate_) {
    return accumulate(ctx, program, instance, program->entry_, argc, argv);
  } else {
    return evaluate(
        ctx,
        program,
        nullptr,
        program->entry_,
        argc,
        argv,
        (SValue*) instance->scratch);
  }
}

void VM::merge(
    Transaction* ctx,
    const Program* program,
    Instance* dst,
    const Instance* src) {
  if (program->has_aggregate_) {
    mergeInstance(ctx, program, program->entry_, dst, src);
  } else {
    *(SValue*) dst->scratch = *(SValue*) src->scratch;
  }
}

void VM::mergeInstance(
    Transaction* ctx,
    const Program* program,
    Instruction* e,
    Instance* dst,
    const Instance* src) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      e->vtable.t_aggregate.merge(
          Transaction::get(ctx),
          (char *) dst->scratch + (size_t) e->arg0,
          (char *) src->scratch + (size_t) e->arg0);
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    mergeInstance(ctx, program, cur, dst, src);
  }
}

void VM::initInstance(
    Transaction* ctx,
    const Program* program,
    Instruction* e,
    Instance* instance) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      if (e->vtable.t_aggregate.init) {
        e->vtable.t_aggregate.init(
            Transaction::get(ctx),
            (char *) instance->scratch + (size_t) e->arg0);
      }
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    initInstance(ctx, program, cur, instance);
  }
}

void VM::freeInstance(
    Transaction* ctx,
    const Program* program,
    Instruction* e,
    Instance* instance) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      if (e->vtable.t_aggregate.free) {
        e->vtable.t_aggregate.free(
            Transaction::get(ctx),
            (char *) instance->scratch + (size_t) e->arg0);
      }
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    freeInstance(ctx, program, cur, instance);
  }
}

void VM::resetInstance(
    Transaction* ctx,
    const Program* program,
    Instruction* e,
    Instance* instance) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      e->vtable.t_aggregate.reset(
          Transaction::get(ctx),
          (char *) instance->scratch + (size_t) e->arg0);
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    resetInstance(ctx, program, cur, instance);
  }
}

void VM::initProgram(
    Transaction* ctx,
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
    initProgram(ctx, program, cur);
  }
}

void VM::freeProgram(
    Transaction* ctx,
    const Program* program,
    Instruction* e) {
  switch (e->type) {
    case X_LITERAL:
      ((SValue*) e->arg0)->~SValue();
      break;

    case X_REGEX:
      ((RegExp*) e->arg0)->~RegExp();
      break;

    case X_LIKE:
      ((LikePattern*) e->arg0)->~LikePattern();
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    freeProgram(ctx, program, cur);
  }
}

void VM::evaluate(
    Transaction* ctx,
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
      evaluate(ctx, program, instance, cond_expr, argc, argv, &cond);

      auto branch = cond_expr->next;
      if (!cond.getBool()) {
        branch = branch->next;
      }

      evaluate(ctx, program, instance, branch, argc, argv, out);
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
            evaluate(ctx, program, instance, cur, argc, argv, stackp++);
          }

          expr->vtable.t_pure.call(Transaction::get(ctx), stackn, stackv, out);
        } catch (...) {
          for (int i = 0; i < stackn; ++i) {
            (stackv + i)->~SValue();
          }

          throw;
        };

        for (int i = 0; i < stackn; ++i) {
          (stackv + i)->~SValue();
        }
      } else {
        expr->vtable.t_pure.call(Transaction::get(ctx), 0, nullptr, out);
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
      expr->vtable.t_aggregate.get(Transaction::get(ctx), scratch, out);
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

    case X_REGEX: {
      SValue subj;
      auto subj_expr = expr->child;
      evaluate(ctx, program, instance, subj_expr, argc, argv, &subj);

      auto match = ((RegExp*) expr->arg0)->match(subj.getString());
      *out = SValue(SValue::BoolType(match));

      return;
    }

    case X_LIKE: {
      SValue subj;
      auto subj_expr = expr->child;
      evaluate(ctx, program, instance, subj_expr, argc, argv, &subj);

      auto match = ((LikePattern*) expr->arg0)->match(subj.getString());
      *out = SValue(SValue::BoolType(match));

      return;
    }

  }

}

void VM::accumulate(
    Transaction* ctx,
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
              ctx,
              program,
              instance,
              cur,
              argc,
              argv,
              stackp++);
        }
      }

      auto scratch = (char *) instance->scratch + (size_t) expr->arg0;
      expr->vtable.t_aggregate.accumulate(
          Transaction::get(ctx),
          scratch,
          stackn,
          stackv);
      return;
    }

    default: {
      for (auto cur = expr->child; cur != nullptr; cur = cur->next) {
        accumulate(ctx, program, instance, cur, argc, argv);
      }

      return;
    }

  }
}

void VM::saveState(
    Transaction* ctx,
    const Program* program,
    const Instance* instance,
    OutputStream* os) {
  if (program->has_aggregate_) {
    saveInstance(ctx, program, program->entry_, instance, os);
  } else {
    ((SValue*) instance->scratch)->encode(os);
  }
}

void VM::loadState(
    Transaction* ctx,
    const Program* program,
    Instance* instance,
    InputStream* is) {
  if (program->has_aggregate_) {
    loadInstance(ctx, program, program->entry_, instance, is);
  } else {
    ((SValue*) instance->scratch)->decode(is);
  }
}

void VM::saveInstance(
    Transaction* ctx,
    const Program* program,
    Instruction* e,
    const Instance* instance,
    OutputStream* os) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      e->vtable.t_aggregate.savestate(
          Transaction::get(ctx),
          (char *) instance->scratch + (size_t) e->arg0,
          os);
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    saveInstance(ctx, program, cur, instance, os);
  }
}

void VM::loadInstance(
    Transaction* ctx,
    const Program* program,
    Instruction* e,
    Instance* instance,
    InputStream* os) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      e->vtable.t_aggregate.loadstate(
          Transaction::get(ctx),
          (char *) instance->scratch + (size_t) e->arg0,
          os);
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    loadInstance(ctx, program, cur, instance, os);
  }
}


}
