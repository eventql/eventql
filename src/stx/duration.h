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
#include <sys/time.h>                   // struct timeval; struct timespec;
#include <stx/time_constants.h>

namespace stx {

class Duration {
private:
  enum class ZeroType { Zero };

public:
  constexpr static ZeroType Zero = ZeroType::Zero;

  /**
   * Creates a new Duration of zero microseconds.
   */
  constexpr Duration(ZeroType);

  /**
   * Create a new Duration
   *
   * @param microseconds the duration in microseconds
   */
  constexpr Duration(uint64_t microseconds);

  /**
   * Creates a new Duration out of a @c timeval struct.
   *
   * @param value duration as @c timeval.
   */
  Duration(const struct ::timeval& value);

  /**
   * Creates a new Duration out of a @c timespec struct.
   *
   * @param value duration as @c timespec.
   */
  Duration(const struct ::timespec& value);

  constexpr bool operator==(const Duration& other) const;
  constexpr bool operator!=(const Duration& other) const;
  constexpr bool operator<(const Duration& other) const;
  constexpr bool operator>(const Duration& other) const;
  constexpr bool operator<=(const Duration& other) const;
  constexpr bool operator>=(const Duration& other) const;
  constexpr bool operator!() const;

  constexpr Duration operator+(const Duration& other) const;

  constexpr operator struct timeval() const;
  constexpr operator struct timespec() const;

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
   * Return the represented duration in milliseconds
   */
  constexpr uint64_t milliseconds() const noexcept;

  /**
   * Return the represented duration in seconds
   */
  constexpr uint64_t seconds() const noexcept;

  constexpr uint64_t minutes() const noexcept;
  constexpr uint64_t hours() const noexcept;
  constexpr uint64_t days() const noexcept;

  static inline Duration fromDays(uint64_t v);
  static inline Duration fromHours(uint64_t v);
  static inline Duration fromMinutes(uint64_t v);
  static inline Duration fromSeconds(uint64_t v);
  static inline Duration fromMilliseconds(uint64_t v);
  static inline Duration fromMicroseconds(uint64_t v);
  static inline Duration fromNanoseconds(uint64_t v);

protected:
  uint64_t micros_;
};

std::string inspect(const Duration& value);

}

#include <stx/duration_impl.h>
#endif
