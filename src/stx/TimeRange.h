// This file is part of the "libstx" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libstx is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#ifndef sw_x0_TimeRange_h
#define sw_x0_TimeRange_h

#include <stx/UnixTime.h>
#include <stx/duration.h>
#include <cstdio>

namespace stx {

/**
 * @brief High resolution time span.
 *
 * A timespan is a tuple of a fixed begin and a duration.
 */
class TimeRange {
public:
  constexpr TimeRange(UnixTime start, Duration duration);
  constexpr TimeRange(UnixTime start, UnixTime end);
  constexpr TimeRange(const TimeRange& other);
  constexpr TimeRange();

  constexpr UnixTime start() const;
  constexpr Duration duration() const;
  constexpr UnixTime end() const;

  constexpr TimeRange backward(Duration by) const;
  constexpr TimeRange forward(Duration by) const;

  constexpr bool operator==(const TimeRange& other) const;
  constexpr bool operator!=(const TimeRange& other) const;

private:
  const UnixTime start_;
  const Duration duration_;
};

}  // namespace stx

#include <stx/TimeRange_impl.h>
#endif
