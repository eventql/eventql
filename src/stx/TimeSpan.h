// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#ifndef sw_x0_TimeSpan_h
#define sw_x0_TimeSpan_h

#include <stx/UnixTime.h>
#include <stx/Duration.h>
#include <cstdio>

namespace stx {

/**
 * @brief High resolution time span.
 *
 * A timespan is a tuple of a fixed begin and a duration.
 */
class TimeSpan {
public:
  constexpr TimeSpan(UnixTime start, Duration duration);
  constexpr TimeSpan(UnixTime start, UnixTime end);
  constexpr TimeSpan(const TimeSpan& other);
  constexpr TimeSpan();

  constexpr UnixTime start() const;
  constexpr Duration duration() const;
  constexpr UnixTime end() const;

  constexpr TimeSpan backward(Duration by) const;
  constexpr TimeSpan forward(Duration by) const;

  constexpr bool operator==(const TimeSpan& other) const;
  constexpr bool operator!=(const TimeSpan& other) const;

private:
  const UnixTime start_;
  const Duration duration_;
};

}  // namespace cortex

#include <stx/TimeSpan_impl.h>
#endif
