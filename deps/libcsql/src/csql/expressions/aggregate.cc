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
#include <csql/expressions/aggregate.h>
#include <csql/svalue.h>

namespace csql {
namespace expressions {

/**
 * COUNT() expression
 */
void countExprAcc(sql_txn* ctx, void* scratchpad, int argc, SValue* argv) {
  switch(argv->getType()) {
    case SQL_NULL:
      return;

    default:
      ++(*(uint64_t*) scratchpad);
      return;
  }
}

void countExprGet(sql_txn* ctx, void* scratchpad, SValue* out) {
  *out = SValue(SValue::IntegerType(*((uint64_t*) scratchpad)));
}

void countExprReset(sql_txn* ctx, void* scratchpad) {
  memset(scratchpad, 0, sizeof(uint64_t));
}

void countExprMerge(sql_txn* ctx, void* scratchpad, const void* other) {
  *(uint64_t*) scratchpad += *(uint64_t*) other;
}

void countExprSave(sql_txn* ctx, void* scratchpad, OutputStream* os) {
  os->appendVarUInt(*(uint64_t*) scratchpad);
}

void countExprLoad(sql_txn* ctx, void* scratchpad, InputStream* is) {
  *(uint64_t*) scratchpad = is->readVarUInt();
}

const AggregateFunction kCountExpr {
  .scratch_size = sizeof(uint64_t),
  .accumulate = &countExprAcc,
  .get = &countExprGet,
  .reset = &countExprReset,
  .init = &countExprReset,
  .free = nullptr,
  .merge = &countExprMerge,
  .savestate = &countExprSave,
  .loadstate = &countExprLoad
};


/**
 * SUM() expression
 */
struct sum_expr_scratchpad {
  sql_type type;
  double val;
};

void sumExprAcc(sql_txn* ctx, void* scratchpad, int argc, SValue* argv) {
  SValue* val = argv;
  auto data = (sum_expr_scratchpad*) scratchpad;

  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for sum(). expected: 1, got: %i\n",
        argc);
  }

  switch(val->getType()) {
    case SQL_NULL:
      return;

    case SQL_INTEGER:
      data->type = SQL_INTEGER;
      data->val += val->getInteger();
      return;

    case SQL_FLOAT:
    default:
      data->type = SQL_FLOAT;
      data->val += val->getFloat();
      return;
  }
}

void sumExprGet(sql_txn* ctx, void* scratchpad, SValue* out) {
  auto data = (sum_expr_scratchpad*) scratchpad;

  switch(data->type) {
    case SQL_INTEGER:
      *out = SValue(SValue::IntegerType(data->val));
      return;

    case SQL_FLOAT:
      *out = SValue(SValue::FloatType(data->val));
      return;

    default:
      *out = SValue();
      return;
  }
}

void sumExprReset(sql_txn* ctx, void* scratchpad) {
  memset(scratchpad, 0, sizeof(sum_expr_scratchpad));
}

void sumExprMerge(sql_txn* ctx, void* scratchpad, const void* other) {
  auto this_data = (sum_expr_scratchpad*) scratchpad;
  auto other_data = (const sum_expr_scratchpad*) other;

  if (this_data->type == SQL_INTEGER &&
      other_data->type == SQL_INTEGER) {
    this_data->type = SQL_INTEGER;
  } else {
    this_data->type = SQL_FLOAT;
  }

  this_data->val += other_data->val;
}

void sumExprSave(sql_txn* ctx, void* scratchpad, OutputStream* os) {
  auto data = (sum_expr_scratchpad*) scratchpad;
  os->appendVarUInt(data->type);
  os->appendDouble(data->val);
}

void sumExprLoad(sql_txn* ctx, void* scratchpad, InputStream* is) {
  auto data = (sum_expr_scratchpad*) scratchpad;
  data->type = (sql_type) is->readVarUInt();
  data->val = is->readDouble();
}

const AggregateFunction kSumExpr {
  .scratch_size = sizeof(sum_expr_scratchpad),
  .accumulate = &sumExprAcc,
  .get = &sumExprGet,
  .reset = &sumExprReset,
  .init = &sumExprReset,
  .free = nullptr,
  .merge = &sumExprMerge,
  .savestate = &sumExprSave,
  .loadstate = &sumExprLoad
};

/**
 * MEAN() expression
 */
struct mean_expr_scratchpad {
  double sum;
  int count;
};

void meanExprAcc(sql_txn* ctx, void* scratchpad, int argc, SValue* argv) {
  SValue* val = argv;
  auto data = (mean_expr_scratchpad*) scratchpad;

  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for mean(). expected: 1, got: %i\n",
        argc);
  }

  switch(val->getType()) {
    case SQL_NULL:
      return;

    default:
      data->sum += val->getFloat();
      data->count += 1;
      return;
  }
}

void meanExprGet(sql_txn* ctx, void* scratchpad, SValue* out) {
  auto data = (mean_expr_scratchpad*) scratchpad;
  *out = SValue(data->sum / data->count);
}

void meanExprReset(sql_txn* ctx, void* scratchpad) {
  memset(scratchpad, 0, sizeof(mean_expr_scratchpad));
}

void meanExprFree(sql_txn* ctx, void* scratchpad) {
  /* noop */
}

size_t meanExprScratchpadSize() {
  return sizeof(mean_expr_scratchpad);
}

void meanExprMerge(sql_txn* ctx, void* scratchpad, const void* other) {
  auto this_data = (mean_expr_scratchpad*) scratchpad;
  auto other_data = (const mean_expr_scratchpad*) other;

  this_data->sum += other_data->sum;
  this_data->count += other_data->count;
}

void meanExprSave(sql_txn* ctx, void* scratchpad, OutputStream* os) {
  auto data = (mean_expr_scratchpad*) scratchpad;
  os->appendVarUInt(data->count);
  os->appendDouble(data->sum);
}

void meanExprLoad(sql_txn* ctx, void* scratchpad, InputStream* is) {
  auto data = (mean_expr_scratchpad*) scratchpad;
  data->count = is->readVarUInt();
  data->sum = is->readDouble();
}

const AggregateFunction kMeanExpr {
  .scratch_size = sizeof(mean_expr_scratchpad),
  .accumulate = &meanExprAcc,
  .get = &meanExprGet,
  .reset = &meanExprReset,
  .init = &meanExprReset,
  .free = nullptr,
  .merge = &meanExprMerge,
  .savestate = &meanExprSave,
  .loadstate = &meanExprLoad
};

/**
 * MAX() expression
 */
union max_expr_scratchpad {
  double max;
  int count;
};

void maxExpr(sql_txn* ctx, void* scratchpad, int argc, SValue* argv, SValue* out) {
  SValue* val = argv;
  union max_expr_scratchpad* data = (union max_expr_scratchpad*) scratchpad;

  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for max(). expected: 1, got: %i\n",
        argc);
  }

  switch(val->getType()) {
    case SQL_NULL:
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

void maxExprFree(sql_txn* ctx, void* scratchpad) {
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

void minExpr(sql_txn* ctx, void* scratchpad, int argc, SValue* argv, SValue* out) {
  SValue* val = argv;
  union min_expr_scratchpad* data = (union min_expr_scratchpad*) scratchpad;

  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for min(). expected: 1, got: %i\n",
        argc);
  }

  switch(val->getType()) {
    case SQL_NULL:
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

void minExprFree(sql_txn* ctx, void* scratchpad) {
  /* noop */
}

size_t minExprScratchpadSize() {
  return sizeof(union min_expr_scratchpad);
}

}
}
