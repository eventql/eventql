/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/ScalarExpression.h>
#include <fnord/inspect.h>

using namespace fnord;

namespace csql {

ScalarExpression::ScalarExpression(
    Instruction* expr,
    size_t scratchpad_size) :
    expr_(expr),
    scratchpad_size_(scratchpad_size) {}

ScalarExpression::~ScalarExpression() {
  // FIXPAUL free instructions...
}

ScalarExpression::Instance ScalarExpression::allocInstance(
    ScratchMemory* scratch) const {
  Instance that;
  that.scratch = scratch->alloc(scratchpad_size_);
  fnord::iputs("init scratch @ $0", (uint64_t) that.scratch);
  init(expr_, &that);
  return that;
}

void ScalarExpression::freeInstance(Instance* instance) const {
  free(expr_, instance);
}

void ScalarExpression::reset(Instance* instance) const {
  reset(expr_, instance);
}

void ScalarExpression::init(Instruction* e, Instance* instance) const {
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
    init(cur, instance);
  }
}

void ScalarExpression::free(Instruction* e, Instance* instance) const {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      if (e->vtable.t_aggregate.free) {
        e->vtable.t_aggregate.free(
            (char *) instance->scratch + (size_t) e->arg0);
      }
      break;

    case X_LITERAL:
        fnord::iputs("free literal: $0", (uint64_t) e);

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    free(cur, instance);
  }
}

void ScalarExpression::reset(Instruction* e, Instance* instance) const {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      e->vtable.t_aggregate.reset(
          (char *) instance->scratch + (size_t) e->arg0);
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    reset(cur, instance);
  }
}

void ScalarExpression::evaluate(
    Instance* instance,
    int argc,
    const SValue* argv,
    SValue* out) const {
  return evaluate(instance, expr_, argc, argv, out);
}

void ScalarExpression::accumulate(
    Instance* instance,
    int argc,
    const SValue* argv) const {
  return accumulate(instance, expr_, argc, argv);
}

void ScalarExpression::evaluate(
    Instance* instance,
    Instruction* expr,
    int argc,
    const SValue* argv,
    SValue* out) const {

  /* execute expression */
  switch (expr->type) {

    case X_CALL_PURE: {
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

      expr->vtable.t_pure.call(stackn, stackv, out);
      return;
    }

    case X_CALL_AGGREGATE: {
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


void ScalarExpression::accumulate(
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
