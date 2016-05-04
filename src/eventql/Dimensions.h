/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/duration.h>
#include <eventql/util/autoref.h>
#include <eventql/sql/svalue.h>
#include <eventql/sql/SFunction.h>

using namespace stx;

namespace zbase {

struct TimeDimension : public csql::PureFunction {
  TimeDimension(const Duration& w) : window(w) {};
  void compute(int argc, const csql::SValue* argv, csql::SValue* out) override;
  Duration window;
};

struct TotalGMVMetric : public csql::SumExpression {
  void compute(int argc, const csql::SValue* argv) {
    //
  }
};

}
