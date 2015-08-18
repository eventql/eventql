/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/duration.h>
#include <stx/autoref.h>
#include <chartsql/svalue.h>
#include <chartsql/SFunction.h>

using namespace stx;

namespace cm {

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
