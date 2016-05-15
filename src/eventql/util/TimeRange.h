/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Christian Parpart <trapni@dawanda.com>
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

#ifndef sw_x0_TimeRange_h
#define sw_x0_TimeRange_h

#include <eventql/util/UnixTime.h>
#include <eventql/util/duration.h>
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

#include <eventql/util/TimeRange_impl.h>
#endif
