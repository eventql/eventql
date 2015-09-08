/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Christian Parpart
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _STX_DURATION_H
#define _STX_DURATION_H

#include <ctime>
#include <inttypes.h>
#include <limits>
#include <string>
#include <stx/time_constants.h>

namespace stx {

class Duration {
public:
  /**
   * Create a new Duration
   *
   * @param microseconds the duration in microseconds
   */
  constexpr Duration(uint64_t microseconds);

  constexpr bool operator==(const Duration& other) const;
  constexpr bool operator<(const Duration& other) const;
  constexpr bool operator>(const Duration& other) const;
  constexpr bool operator<=(const Duration& other) const;
  constexpr bool operator>=(const Duration& other) const;

  constexpr Duration operator+(const Duration& other) const;

  /**
   * Return the represented duration in microseconds
   */
  constexpr explicit operator uint64_t() const;

  /**
   * Return the represented duration in microseconds
   */
  constexpr explicit operator double() const;

  /**
   * Return the represented duration in microseconds
   */
  constexpr uint64_t microseconds() const noexcept;

  /**
   * Return the represented duration in seconds
   */
  constexpr uint64_t seconds() const noexcept;

  constexpr uint64_t minutes() const noexcept;
  constexpr uint64_t hours() const noexcept;
  constexpr uint64_t days() const noexcept;

  static constexpr Duration fromDays(uint64_t v);
  static constexpr Duration fromHours(uint64_t v);
  static constexpr Duration fromMinutes(uint64_t v);
  static constexpr Duration fromSeconds(uint64_t v);
  static constexpr Duration fromMilliseconds(uint64_t v);
  static constexpr Duration fromMicroseconds(uint64_t v);
  static constexpr Duration fromNanoseconds(uint64_t v);

protected:
  const uint64_t micros_;
};

}

#include <stx/Duration_impl.h>
#endif
