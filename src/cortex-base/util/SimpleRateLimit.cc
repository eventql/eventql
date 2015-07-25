// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/util/SimpleRateLimit.h>

namespace cortex {
namespace util {

SimpleRateLimit::SimpleRateLimit(
    const TimeSpan& period) :
    period_micros_(period.microseconds()),
    last_micros_(0) {}

bool SimpleRateLimit::check() {
  auto now = WallClock::monotonic()->get().unixMicroseconds();

  if (now - last_micros_ >= period_micros_) {
    last_micros_ = now;
    return true;
  } else {
    return false;
  }
}

SimpleRateLimitedFn::SimpleRateLimitedFn(
    const TimeSpan& period,
    std::function<void ()> fn) :
    limit_(period),
    fn_(fn) {}

void SimpleRateLimitedFn::runMaybe() {
  if (limit_.check()) {
    fn_();
  }
}

void SimpleRateLimitedFn::runForce() {
  fn_();
}

}
}

