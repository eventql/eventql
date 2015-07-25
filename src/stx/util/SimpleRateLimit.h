/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_UTIL_SIMPLERATELIMIT_H
#define _STX_UTIL_SIMPLERATELIMIT_H
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include "stx/stdtypes.h"
#include "stx/wallclock.h"
#include "stx/duration.h"

namespace stx {
namespace util {

class SimpleRateLimit {
public:
  SimpleRateLimit(const Duration& period);
  bool check();
protected:
  uint64_t period_micros_;
  uint64_t last_micros_;
};

class SimpleRateLimitedFn {
public:
  SimpleRateLimitedFn(const Duration& period, Function<void ()> fn);
  void runMaybe();
  void runForce();
protected:
  SimpleRateLimit limit_;
  Function<void ()> fn_;
};



}
}

#endif
