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
 * count(nil)
 */
void count_acc(sql_txn* ctx, void* self, VMStack* stack) {
  popNil(stack);
  ++(*static_cast<uint64_t*>(self));
}

void count_get(sql_txn* ctx, void* self, VMStack* stack) {
  pushUInt64(stack, *static_cast<uint64_t*>(self));
}

void count_reset(sql_txn* ctx, void* self) {
  memset(self, 0, sizeof(uint64_t));
}

void count_merge(sql_txn* ctx, void* self, const void* other) {
  *static_cast<uint64_t*>(self) += *static_cast<const uint64_t*>(other);
}

void count_save(sql_txn* ctx, const void* self, OutputStream* os) {
  os->appendVarUInt(*static_cast<const uint64_t*>(self));
}

void count_load(sql_txn* ctx, void* self, InputStream* is) {
  *static_cast<uint64_t*>(self) = is->readVarUInt();
}

const SFunction count(
    { SType::NIL },
    SType::UINT64,
    sizeof(uint64_t),
    &count_acc,
    &count_get,
    &count_reset,
    &count_reset,
    nullptr,
    &count_merge,
    &count_save,
    &count_load);


/**
 * count_distinct(uint64)
 */
template <typename T>
using DistinctSetType = std::set<T>;

void count_distinct_uint64_acc(sql_txn* ctx, void* self, VMStack* stack) {
  auto val = popUInt64(stack);
  static_cast<DistinctSetType<uint64_t>*>(self)->insert(val);
}

void count_distinct_uint64_get(sql_txn* ctx, void* self, VMStack* stack) {
  auto cnt = static_cast<DistinctSetType<uint64_t>*>(self)->size();
  pushUInt64(stack, cnt);
}

void count_distinct_uint64_init(sql_txn* ctx, void* self) {
  new (self) DistinctSetType<uint64_t>();
}

void count_distinct_uint64_free(sql_txn* ctx, void* self) {
  static_cast<DistinctSetType<uint64_t>*>(self)->~DistinctSetType<uint64_t>();
}

void count_distinct_uint64_reset(sql_txn* ctx, void* self) {
  static_cast<DistinctSetType<uint64_t>*>(self)->clear();
}

void count_distinct_uint64_merge(sql_txn* ctx, void* self, const void* other) {
  auto self_set = static_cast<DistinctSetType<uint64_t>*>(self);
  auto other_set = static_cast<const DistinctSetType<uint64_t>*>(other);
  for (const auto& v : *other_set) {
    self_set->insert(v);
  }
}

void count_distinct_uint64_save(sql_txn* ctx, const void* self, OutputStream* os) {
  auto self_set = static_cast<const DistinctSetType<uint64_t>*>(self);
  os->appendVarUInt(self_set->size());
  for (const auto& v : *self_set) {
    os->appendVarUInt(v);
  }
}

void count_distinct_uint64_load(sql_txn* ctx, void* self, InputStream* is) {
  auto self_set = static_cast<DistinctSetType<uint64_t>*>(self);
  auto n = is->readVarUInt();
  for (size_t i = 0; i < n; ++i) {
    self_set->insert(is->readVarUInt());
  }
}

const SFunction count_distinct_uint64(
    { SType::UINT64 },
    SType::UINT64,
    sizeof(DistinctSetType<uint64_t>),
    &count_distinct_uint64_acc,
    &count_distinct_uint64_get,
    &count_distinct_uint64_reset,
    &count_distinct_uint64_init,
    &count_distinct_uint64_free,
    &count_distinct_uint64_merge,
    &count_distinct_uint64_save,
    &count_distinct_uint64_load);


/**
 * SUM(int64) expression
 */
void sum_int64_acc(sql_txn* ctx, void* self, VMStack* stack) {
  *static_cast<int64_t*>(self) += popInt64(stack);
}

void sum_int64_get(sql_txn* ctx, void* self, VMStack* stack) {
  pushInt64(stack, *static_cast<int64_t*>(self));
}

void sum_int64_reset(sql_txn* ctx, void* self) {
  memset(self, 0, sizeof(int64_t));
}

void sum_int64_merge(sql_txn* ctx, void* self, const void* other) {
  *static_cast<int64_t*>(self) += *static_cast<const int64_t*>(other);
}

void sum_int64_save(sql_txn* ctx, const void* self, OutputStream* os) {
  os->appendVarUInt(*(const int64_t*) self); // FIXME
}

void sum_int64_load(sql_txn* ctx, void* self, InputStream* is) {
  auto data = (int64_t*) self;
  *data = is->readVarUInt(); // FIXME
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
void sum_uint64_acc(sql_txn* ctx, void* self, VMStack* stack) {
  *static_cast<uint64_t*>(self) += popUInt64(stack);
}

void sum_uint64_get(sql_txn* ctx, void* self, VMStack* stack) {
  pushUInt64(stack, *static_cast<uint64_t*>(self));
}

void sum_uint64_reset(sql_txn* ctx, void* self) {
 memset(self, 0, sizeof(uint64_t));
}

void sum_uint64_merge(sql_txn* ctx, void* self, const void* other) {
  *static_cast<uint64_t*>(self) += *static_cast<const uint64_t*>(other);
}

void sum_uint64_save(sql_txn* ctx, const void* self, OutputStream* os) {
  os->appendVarUInt(*static_cast<const uint64_t*>(self));
}

void sum_uint64_load(sql_txn* ctx, void* self, InputStream* is) {
  *static_cast<uint64_t*>(self) = is->readVarUInt();
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
//struct mean_expr_self {
//  double sum;
//  int count;
//};
//
//void meanExprAcc(sql_txn* ctx, void* self, int argc, SValue* argv) {
//  SValue* val = argv;
//  auto data = (mean_expr_self*) self;
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
//void meanExprGet(sql_txn* ctx, void* self, SValue* out) {
//  auto data = (mean_expr_self*) self;
//  *out = SValue(data->sum / data->count);
//}
//
//void meanExprReset(sql_txn* ctx, void* self) {
//  memset(self, 0, sizeof(mean_expr_self));
//}
//
//void meanExprFree(sql_txn* ctx, void* self) {
//  /* noop */
//}
//
//size_t meanExprScratchpadSize() {
//  return sizeof(mean_expr_self);
//}
//
//void meanExprMerge(sql_txn* ctx, void* self, const void* other) {
//  auto this_data = (mean_expr_self*) self;
//  auto other_data = (const mean_expr_self*) other;
//
//  this_data->sum += other_data->sum;
//  this_data->count += other_data->count;
//}
//
//void meanExprSave(sql_txn* ctx, void* self, OutputStream* os) {
//  auto data = (mean_expr_self*) self;
//  os->appendVarUInt(data->count);
//  os->appendDouble(data->sum);
//}
//
//void meanExprLoad(sql_txn* ctx, void* self, InputStream* is) {
//  auto data = (mean_expr_self*) self;
//  data->count = is->readVarUInt();
//  data->sum = is->readDouble();
//}
//
//const AggregateFunction kMeanExpr {
//  .scratch_size = sizeof(mean_expr_self),
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
//struct max_expr_self {
//  double max;
//  int count;
//};
//
//void maxExprAcc(sql_txn* ctx, void* self, int argc, SValue* argv) {
//  auto data = (max_expr_self*) self;
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
//void maxExprGet(sql_txn* ctx, void* self, SValue* out) {
//  *out = SValue::newFloat(((max_expr_self*) self)->max);
//}
//
//void maxExprReset(sql_txn* ctx, void* self) {
//  memset(self, 0, sizeof(max_expr_self));
//}
//
//void maxExprMerge(sql_txn* ctx, void* self, const void* other) {
//  auto data = (max_expr_self*) self;
//  auto other_data = (max_expr_self*) other;
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
//void maxExprSave(sql_txn* ctx, void* self, OutputStream* os) {
//  os->write((char*) self, sizeof(max_expr_self));
//}
//
//void maxExprLoad(sql_txn* ctx, void* self, InputStream* is) {
//  is->readNextBytes(self, sizeof(max_expr_self));
//}
//
//const AggregateFunction kMaxExpr {
//  .scratch_size = sizeof(max_expr_self),
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
//struct min_expr_self {
//  double min;
//  int count;
//};
//
//void minExprAcc(sql_txn* ctx, void* self, int argc, SValue* argv) {
//  auto data = (min_expr_self*) self;
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
//void minExprGet(sql_txn* ctx, void* self, SValue* out) {
//  *out = SValue::newFloat(((min_expr_self*) self)->min);
//}
//
//void minExprReset(sql_txn* ctx, void* self) {
//  memset(self, 0, sizeof(min_expr_self));
//}
//
//void minExprMerge(sql_txn* ctx, void* self, const void* other) {
//  auto data = (min_expr_self*) self;
//  auto other_data = (min_expr_self*) other;
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
//void minExprSave(sql_txn* ctx, void* self, OutputStream* os) {
//  os->write((char*) self, sizeof(min_expr_self));
//}
//
//void minExprLoad(sql_txn* ctx, void* self, InputStream* is) {
//  is->readNextBytes(self, sizeof(min_expr_self));
//}
//
//const AggregateFunction kMinExpr {
//  .scratch_size = sizeof(min_expr_self),
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
