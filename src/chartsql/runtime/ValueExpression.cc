/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/ValueExpression.h>
#include <stx/inspect.h>

using namespace stx;

namespace csql {

ValueExpression::ValueExpression(
    Instruction* entry,
    ScratchMemory&& static_storage,
    size_t dynamic_storage_size) :
    entry_(entry),
    static_storage_(std::move(static_storage)),
    dynamic_storage_size_(dynamic_storage_size),
    has_aggregate_(false) {
  initProgram(entry_);
}

ValueExpression::~ValueExpression() {
  freeProgram(entry_);
}

ValueExpression::Instance ValueExpression::allocInstance(
    ScratchMemory* scratch) const {
  Instance that;

  if (has_aggregate_) {
    that.scratch = scratch->alloc(dynamic_storage_size_);
    initInstance(entry_, &that);
  } else {
    that.scratch = scratch->construct<SValue>();
  }

  return that;
}

void ValueExpression::freeInstance(Instance* instance) const {
  if (has_aggregate_) {
    freeInstance(entry_, instance);
  } else {
    ((SValue*) instance->scratch)->~SValue();
  }
}

void ValueExpression::reset(Instance* instance) const {
  if (has_aggregate_) {
    resetInstance(entry_, instance);
  } else {
    *((SValue*) instance->scratch) = SValue();
  }
}

void ValueExpression::result(
    Instance* instance,
    SValue* out) const {
  if (has_aggregate_) {
    return evaluate(instance, entry_, 0, nullptr, out);
  } else {
    *out = *((SValue*) instance->scratch);
  }
}

void ValueExpression::accumulate(
    Instance* instance,
    int argc,
    const SValue* argv) const {
  if (has_aggregate_) {
    return accumulate(instance, entry_, argc, argv);
  } else {
    return evaluate(nullptr, entry_, argc, argv, (SValue*) instance->scratch);
  }
}

void ValueExpression::evaluate(
    int argc,
    const SValue* argv,
    SValue* out) const {
  return evaluate(nullptr, entry_, argc, argv, out);
}

void ValueExpression::initInstance(Instruction* e, Instance* instance) const {
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
    initInstance(cur, instance);
  }
}

void ValueExpression::freeInstance(Instruction* e, Instance* instance) const {
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
    freeInstance(cur, instance);
  }
}

void ValueExpression::resetInstance(Instruction* e, Instance* instance) const {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      e->vtable.t_aggregate.reset(
          (char *) instance->scratch + (size_t) e->arg0);
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    resetInstance(cur, instance);
  }
}

void ValueExpression::initProgram(Instruction* e) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      has_aggregate_ = true;
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    initProgram(cur);
  }
}

void ValueExpression::freeProgram(Instruction* e) const {
  switch (e->type) {
    case X_LITERAL:
      ((SValue*) e->arg0)->~SValue();
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    freeProgram(cur);
  }
}

void ValueExpression::evaluate(
    Instance* instance,
    Instruction* expr,
    int argc,
    const SValue* argv,
    SValue* out) const {

  /* execute expression */
  switch (expr->type) {

    case X_IF: {
      SValue cond;
      auto cond_expr = expr->child;
      evaluate(instance, cond_expr, argc, argv, &cond);

      auto branch = cond_expr->next;
      if (!cond.getBoolWithConversion()) {
        branch = branch->next;
      }

      evaluate(instance, branch, argc, argv, out);
      return;
    }

    case X_CALL_PURE: {
      SValue* stackv = nullptr;
      auto stackn = expr->argn;
      if (stackn > 0) {
        // FIXPAUL free...
        stackv = reinterpret_cast<SValue*>(alloca(sizeof(SValue) * expr->argn));
        for (int i = 0; i < stackn; ++i) {
          new (stackv + i) SValue();
        }

        auto stackp = stackv;
        for (auto cur = expr->child; cur != nullptr; cur = cur->next) {
          evaluate(instance, cur, argc, argv, stackp++);
        }
      }

      expr->vtable.t_pure.call(stackn, stackv, out);
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

void ValueExpression::accumulate(
    Instance* instance,
    Instruction* expr,
    int argc,
    const SValue* argv) const {

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
        accumulate(instance, cur, argc, argv);
      }

      return;
    }

  }

}

}
