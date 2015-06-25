/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <chartsql/expressions/aggregate.h>
#include <chartsql/svalue.h>

namespace csql {
namespace expressions {

void countExpr(void* scratchpad, int argc, SValue* argv, SValue* out) {
  uint64_t* count = (uint64_t*) scratchpad;
  *out = SValue((int64_t) ++(*count));
}

void countExprFree(void* scratchpad) {
  /* noop */
}

size_t countExprScratchpadSize() {
  return sizeof(uint64_t);
}

/**
 * SUM() expression
 */
struct sum_expr_scratchpad {
  SValue::kSValueType type;
  double val;
};

void sumExprAcc(void* scratchpad, int argc, SValue* argv) {
  SValue* val = argv;
  auto data = (sum_expr_scratchpad*) scratchpad;

  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for sum(). expected: 1, got: %i\n",
        argc);
  }

  switch(val->getType()) {
    case SValue::T_NULL:
      return;

    case SValue::T_INTEGER:
      data->type = SValue::T_INTEGER;
      data->val += val->getInteger();
      return;

    case SValue::T_FLOAT:
    default:
      data->type = SValue::T_FLOAT;
      data->val += val->getFloat();
      return;
  }
}

void sumExprGet(void* scratchpad, SValue* out) {
  auto data = (sum_expr_scratchpad*) scratchpad;

  switch(data->type) {
    case SValue::T_INTEGER:
      *out = SValue(SValue::IntegerType(data->val));
      return;

    case SValue::T_FLOAT:
      *out = SValue(SValue::FloatType(data->val));
      return;

    default:
      *out = SValue();
      return;
  }
}

void sumExprReset(void* scratchpad) {
  memset(scratchpad, 0, sizeof(sum_expr_scratchpad));
}

const AggregateExpression kSumExpr {
  .scratch_size = sizeof(sum_expr_scratchpad),
  .accumulate = &sumExprAcc,
  .get = &sumExprGet,
  .reset = &sumExprReset,
  .init = &sumExprReset,
  .free = nullptr,
};

/**
 * MEAN() expression
 */
union mean_expr_scratchpad {
  double sum;
  int count;
};

void meanExpr(void* scratchpad, int argc, SValue* argv, SValue* out) {
  SValue* val = argv;
  union mean_expr_scratchpad* data = (union mean_expr_scratchpad*) scratchpad;

  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for mean(). expected: 1, got: %i\n",
        argc);
  }

  switch(val->getType()) {
    case SValue::T_NULL:
      return;

    default:
      data->sum += val->getFloat();
      data->count += 1;
      *out = SValue(data->sum / data->count);
      return;
  }
}

void meanExprFree(void* scratchpad) {
  /* noop */
}

size_t meanExprScratchpadSize() {
  return sizeof(union mean_expr_scratchpad);
}

/**
 * MAX() expression
 */
union max_expr_scratchpad {
  double max;
  int count;
};

void maxExpr(void* scratchpad, int argc, SValue* argv, SValue* out) {
  SValue* val = argv;
  union max_expr_scratchpad* data = (union max_expr_scratchpad*) scratchpad;

  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for max(). expected: 1, got: %i\n",
        argc);
  }

  switch(val->getType()) {
    case SValue::T_NULL:
      return;

    default: {
      auto fval = val->getFloat();
      if (data->count == 0 || fval > data->max) {
        data->max = fval;
      }

      data->count = 1;
      *out = SValue(data->max);
      return;
    }
  }
}

void maxExprFree(void* scratchpad) {
  /* noop */
}

size_t maxExprScratchpadSize() {
  return sizeof(union max_expr_scratchpad);
}

/**
 * MIN() expression
 */
union min_expr_scratchpad {
  double min;
  int count;
};

void minExpr(void* scratchpad, int argc, SValue* argv, SValue* out) {
  SValue* val = argv;
  union min_expr_scratchpad* data = (union min_expr_scratchpad*) scratchpad;

  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for min(). expected: 1, got: %i\n",
        argc);
  }

  switch(val->getType()) {
    case SValue::T_NULL:
      return;

    default: {
      auto fval = val->getFloat();
      if (data->count == 0 || fval < data->min) {
        data->min = fval;
      }

      data->count = 1;
      *out = SValue(data->min);
      return;
    }
  }
}

void minExprFree(void* scratchpad) {
  /* noop */
}

size_t minExprScratchpadSize() {
  return sizeof(union min_expr_scratchpad);
}

}
}
