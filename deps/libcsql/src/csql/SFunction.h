/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/ieee754.h>
#include <stx/util/binarymessagereader.h>
#include <stx/util/binarymessagewriter.h>
#include <csql/svalue.h>
#include <csql/Transaction.h>

using namespace stx;

namespace csql {

enum kFunctionType {
  FN_PURE,
  FN_AGGREGATE
};

/**
 * A pure/stateless expression that returns a single return value
 */
struct PureFunction {
  PureFunction();
  PureFunction(void (*_call)(sql_txn* ctx, int argc, SValue* in, SValue* out));
  void (*call)(sql_txn* ctx, int argc, SValue* in, SValue* out);
};

/**
 * An aggregate expression that returns a single return value
 */
struct AggregateFunction {
  size_t scratch_size;
  void (*accumulate)(sql_txn*, void* scratch, int argc, SValue* in);
  void (*get)(sql_txn*, void* scratch, SValue* out);
  void (*reset)(sql_txn*, void* scratch);
  void (*init)(sql_txn*, void* scratch);
  void (*free)(sql_txn*, void* scratch);
  void (*merge)(sql_txn*, void* scratch, const void* other);
  void (*savestate)(sql_txn*, void* scratch, OutputStream* os);
  void (*loadstate)(sql_txn*, void* scratch, InputStream* is);
};

struct SFunction {
  SFunction();
  SFunction(PureFunction fn);
  SFunction(AggregateFunction fn);

  bool isAggregate() const;

  kFunctionType type;
  union {
    PureFunction t_pure;
    AggregateFunction t_aggregate;
  } vtable;
};

} // namespace csql
