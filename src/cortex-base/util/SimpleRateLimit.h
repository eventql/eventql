// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <cortex-base/WallClock.h>
#include <cortex-base/TimeSpan.h>
#include <functional>
#include <string>
#include <stdlib.h>
#include <stdint.h>

namespace cortex {
namespace util {

class CORTEX_API SimpleRateLimit {
 public:
  SimpleRateLimit(const TimeSpan& period);

  bool check();

 protected:
  uint64_t period_micros_;
  uint64_t last_micros_;
};

class CORTEX_API SimpleRateLimitedFn {
 public:
  SimpleRateLimitedFn(const TimeSpan& period, std::function<void ()> fn);

  void runMaybe();
  void runForce();

 protected:
  SimpleRateLimit limit_;
  std::function<void ()> fn_;
};



} // namespace util
} // namespace cortex
