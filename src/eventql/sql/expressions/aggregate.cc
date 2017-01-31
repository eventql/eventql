/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <stdlib.h>
#include <eventql/sql/expressions/aggregate.h>
#include <eventql/sql/svalue.h>
#include <eventql/sql/runtime/vm.h>

namespace csql {
namespace expressions {

/**
 * COUNT() expression
 */
void countExprAcc(sql_txn* ctx, void* scratchpad, int argc, void** argv) {
  ++(*(uint64_t*) scratchpad);
}

void countExprGet(sql_txn* ctx, void* scratchpad, VMRegister* out) {
  *((uint64_t*) out->data) = *((uint64_t*) scratchpad);
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

const SFunction kCountExpr(
    { SType::UINT64 },
    SType::UINT64,
    sizeof(uint64_t),
    &countExprAcc,
    &countExprGet,
    &countExprReset,
    &countExprReset,
    nullptr,
    &countExprMerge,
    &countExprSave,
    &countExprLoad);


/**
 * SUM(int64) expression
 */
void sum_int64_acc(sql_txn* ctx, void* scratchpad, int argc, void** argv) {
  *static_cast<int64_t*>(scratchpad) += *static_cast<int64_t*>(argv[0]);
}

void sum_int64_get(sql_txn* ctx, void* scratchpad, VMRegister* out) {
  *((int64_t*) out->data) = *((int64_t*) scratchpad);
}

void sum_int64_reset(sql_txn* ctx, void* scratchpad) {
  memset(scratchpad, 0, sizeof(int64_t));
}

void sum_int64_merge(sql_txn* ctx, void* scratchpad, const void* other) {
  auto this_data = (int64_t*) scratchpad;
  auto other_data = (const int64_t*) other;
  *this_data += *other_data;
}

void sum_int64_save(sql_txn* ctx, void* scratchpad, OutputStream* os) {
  os->appendVarUInt(*(int64_t*) scratchpad);
}

void sum_int64_load(sql_txn* ctx, void* scratchpad, InputStream* is) {
  auto data = (int64_t*) scratchpad;
  *data = is->readVarUInt();
}

const SFunction sum_int64(
    { SType::INT64 },
    SType::INT64,
    sizeof(int64_t),
    &sum_int64_acc,
    &sum_int64_get,
    &sum_int64_reset,
    &sum_int64_reset,
    nullptr,
    &sum_int64_merge,
    &sum_int64_save,
    &sum_int64_load);

/**
* SUM(uint64) expression
*/
void sum_uint64_acc(sql_txn* ctx, void* scratchpad, int argc, void** argv) {
  *static_cast<uint64_t*>(scratchpad) += *static_cast<uint64_t*>(argv[0]);
}

void sum_uint64_get(sql_txn* ctx, void* scratchpad, VMRegister* out) {
 *((uint64_t*) out->data) = *((uint64_t*) scratchpad);
}

void sum_uint64_reset(sql_txn* ctx, void* scratchpad) {
 memset(scratchpad, 0, sizeof(uint64_t));
}

void sum_uint64_merge(sql_txn* ctx, void* scratchpad, const void* other) {
 auto this_data = (uint64_t*) scratchpad;
 auto other_data = (const uint64_t*) other;
 *this_data += *other_data;
}

void sum_uint64_save(sql_txn* ctx, void* scratchpad, OutputStream* os) {
 os->appendVarUInt(*(uint64_t*) scratchpad);
}

void sum_uint64_load(sql_txn* ctx, void* scratchpad, InputStream* is) {
 auto data = (uint64_t*) scratchpad;
 *data = is->readVarUInt();
}

const SFunction sum_uint64(
   { SType::UINT64 },
   SType::UINT64,
   sizeof(uint64_t),
   &sum_uint64_acc,
   &sum_uint64_get,
   &sum_uint64_reset,
   &sum_uint64_reset,
   nullptr,
   &sum_uint64_merge,
   &sum_uint64_save,
   &sum_uint64_load);


/**
 * MEAN() expression
 */
//struct mean_expr_scratchpad {
//  double sum;
//  int count;
//};
//
//void meanExprAcc(sql_txn* ctx, void* scratchpad, int argc, SValue* argv) {
//  SValue* val = argv;
//  auto data = (mean_expr_scratchpad*) scratchpad;
//
//  if (argc != 1) {
//    RAISE(
//        kRuntimeError,
//        "wrong number of arguments for mean(). expected: 1, got: %i\n",
//        argc);
//  }
//
//  switch(val->getType()) {
//    case SType::NIL:
//      return;
//
//    default:
//      data->sum += val->getFloat();
//      data->count += 1;
//      return;
//  }
//}
//
//void meanExprGet(sql_txn* ctx, void* scratchpad, SValue* out) {
//  auto data = (mean_expr_scratchpad*) scratchpad;
//  *out = SValue(data->sum / data->count);
//}
//
//void meanExprReset(sql_txn* ctx, void* scratchpad) {
//  memset(scratchpad, 0, sizeof(mean_expr_scratchpad));
//}
//
//void meanExprFree(sql_txn* ctx, void* scratchpad) {
//  /* noop */
//}
//
//size_t meanExprScratchpadSize() {
//  return sizeof(mean_expr_scratchpad);
//}
//
//void meanExprMerge(sql_txn* ctx, void* scratchpad, const void* other) {
//  auto this_data = (mean_expr_scratchpad*) scratchpad;
//  auto other_data = (const mean_expr_scratchpad*) other;
//
//  this_data->sum += other_data->sum;
//  this_data->count += other_data->count;
//}
//
//void meanExprSave(sql_txn* ctx, void* scratchpad, OutputStream* os) {
//  auto data = (mean_expr_scratchpad*) scratchpad;
//  os->appendVarUInt(data->count);
//  os->appendDouble(data->sum);
//}
//
//void meanExprLoad(sql_txn* ctx, void* scratchpad, InputStream* is) {
//  auto data = (mean_expr_scratchpad*) scratchpad;
//  data->count = is->readVarUInt();
//  data->sum = is->readDouble();
//}
//
//const AggregateFunction kMeanExpr {
//  .scratch_size = sizeof(mean_expr_scratchpad),
//  .accumulate = &meanExprAcc,
//  .get = &meanExprGet,
//  .reset = &meanExprReset,
//  .init = &meanExprReset,
//  .free = nullptr,
//  .merge = &meanExprMerge,
//  .savestate = &meanExprSave,
//  .loadstate = &meanExprLoad
//};

/**
 * MAX() expression
 */
//struct max_expr_scratchpad {
//  double max;
//  int count;
//};
//
//void maxExprAcc(sql_txn* ctx, void* scratchpad, int argc, SValue* argv) {
//  auto data = (max_expr_scratchpad*) scratchpad;
//
//  switch(argv->getType()) {
//    case SType::NIL:
//      return;
//
//    default: {
//      auto fval = argv->getFloat();
//      if (data->count == 0 || fval > data->max) {
//        data->max = fval;
//      }
//
//      data->count = 1;
//      return;
//    }
//
//  }
//}
//
//void maxExprGet(sql_txn* ctx, void* scratchpad, SValue* out) {
//  *out = SValue::newFloat(((max_expr_scratchpad*) scratchpad)->max);
//}
//
//void maxExprReset(sql_txn* ctx, void* scratchpad) {
//  memset(scratchpad, 0, sizeof(max_expr_scratchpad));
//}
//
//void maxExprMerge(sql_txn* ctx, void* scratchpad, const void* other) {
//  auto data = (max_expr_scratchpad*) scratchpad;
//  auto other_data = (max_expr_scratchpad*) other;
//
//  if (other_data->count == 0) {
//    return;
//  }
//
//  if (data->count == 0 || other_data->max > data->max) {
//    data->max = other_data->max;
//  }
//
//  data->count = 1;
//}
//
//void maxExprSave(sql_txn* ctx, void* scratchpad, OutputStream* os) {
//  os->write((char*) scratchpad, sizeof(max_expr_scratchpad));
//}
//
//void maxExprLoad(sql_txn* ctx, void* scratchpad, InputStream* is) {
//  is->readNextBytes(scratchpad, sizeof(max_expr_scratchpad));
//}
//
//const AggregateFunction kMaxExpr {
//  .scratch_size = sizeof(max_expr_scratchpad),
//  .accumulate = &maxExprAcc,
//  .get = &maxExprGet,
//  .reset = &maxExprReset,
//  .init = &maxExprReset,
//  .free = nullptr,
//  .merge = &maxExprMerge,
//  .savestate = &maxExprSave,
//  .loadstate = &maxExprLoad
//};
//
///**
// * MIN() expression
// */
//struct min_expr_scratchpad {
//  double min;
//  int count;
//};
//
//void minExprAcc(sql_txn* ctx, void* scratchpad, int argc, SValue* argv) {
//  auto data = (min_expr_scratchpad*) scratchpad;
//
//  switch(argv->getType()) {
//    case SType::NIL:
//      return;
//
//    default: {
//      auto fval = argv->getFloat();
//      if (data->count == 0 || fval < data->min) {
//        data->min = fval;
//      }
//
//      data->count = 1;
//      return;
//    }
//
//  }
//}
//
//void minExprGet(sql_txn* ctx, void* scratchpad, SValue* out) {
//  *out = SValue::newFloat(((min_expr_scratchpad*) scratchpad)->min);
//}
//
//void minExprReset(sql_txn* ctx, void* scratchpad) {
//  memset(scratchpad, 0, sizeof(min_expr_scratchpad));
//}
//
//void minExprMerge(sql_txn* ctx, void* scratchpad, const void* other) {
//  auto data = (min_expr_scratchpad*) scratchpad;
//  auto other_data = (min_expr_scratchpad*) other;
//
//  if (other_data->count == 0) {
//    return;
//  }
//
//  if (data->count == 0 || other_data->min < data->min) {
//    data->min = other_data->min;
//  }
//
//  data->count = 1;
//}
//
//void minExprSave(sql_txn* ctx, void* scratchpad, OutputStream* os) {
//  os->write((char*) scratchpad, sizeof(min_expr_scratchpad));
//}
//
//void minExprLoad(sql_txn* ctx, void* scratchpad, InputStream* is) {
//  is->readNextBytes(scratchpad, sizeof(min_expr_scratchpad));
//}
//
//const AggregateFunction kMinExpr {
//  .scratch_size = sizeof(min_expr_scratchpad),
//  .accumulate = &minExprAcc,
//  .get = &minExprGet,
//  .reset = &minExprReset,
//  .init = &minExprReset,
//  .free = nullptr,
//  .merge = &minExprMerge,
//  .savestate = &minExprSave,
//  .loadstate = &minExprLoad
//};

}
}
