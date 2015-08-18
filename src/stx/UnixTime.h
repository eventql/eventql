/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _libstx_UTIL_DATETIME_H
#define _libstx_UTIL_DATETIME_H
#include <ctime>
#include <inttypes.h>
#include <limits>
#include <string>
#include "stx/time_constants.h"
#include "stx/CivilTime.h"
#include "stx/option.h"

namespace stx {

class UnixTime {
public:

  /**
   * Create a new UTC UnixTime instance with time = now
   */
  UnixTime();

  /**
   * Create a new UTC UnixTime instance
   *
   * @param timestamp the UTC microsecond timestamp
   */
  UnixTime(uint64_t utc_time);

  /**
   * Create a new UTC UnixTime instance from a civil time reference
   *
   * @param civil the civil time
   */
  UnixTime(const CivilTime& civil);

  /**
   * Parse time from the provided string
   *
   * @param str the string to parse
   * @param fmt the strftime format string (optional)
   */
  static Option<UnixTime> parseString(
      const String& str,
      const char* fmt = "%Y-%m-%d %H:%M:%S");

  /**
   * Parse time from the provided string
   *
   * @param str the string to parse
   * @param strlen the size of the string to parse
   * @param fmt the strftime format string (optional)
   */
  static Option<UnixTime> parseString(
      const char* str,
      size_t strlen,
      const char* fmt = "%Y-%m-%d %H:%M:%S");

  /**
   * Return a representation of the date as a string (strftime)
   *
   * @param fmt the strftime format string (optional)
   */
  std::string toString(const char* fmt = "%Y-%m-%d %H:%M:%S") const;

  UnixTime& operator=(const UnixTime& other);

  bool operator==(const UnixTime& other) const;
  bool operator<(const UnixTime& other) const;
  bool operator>(const UnixTime& other) const;
  bool operator<=(const UnixTime& other) const;
  bool operator>=(const UnixTime& other) const;

  /**
   * Cast the UnixTime object to a UTC unix microsecond timestamp represented as
   * an uint64_t
   */
  explicit operator uint64_t() const;

  /**
   * Cast the UnixTime object to a UTC unix microsecond timestamp represented as
   * a double
   */
  explicit operator double() const;

  /**
   * Return the represented date/time as a UTC unix microsecond timestamp
   */
  uint64_t unixMicros() const;

  /**
   * Return a new UnixTime instance with time 00:00:00 UTC, 1 Jan. 1970
   */
  static UnixTime epoch();

  /**
   * Return a new UnixTime instance with time = now
   */
  static UnixTime now();

  /**
   * Return a new UnixTime instance with time = now + days
   */
  static UnixTime daysFromNow(double days);

protected:

  /**
   * The utc microsecond timestamp of the represented moment in time
   */
  uint64_t utc_micros_;

  /**
   * The time zone offset to UTC in seconds
   */
  uint32_t tz_offset_;

};

}

namespace std {
template <> class numeric_limits<stx::UnixTime> {
public:
  static stx::UnixTime max();
  static stx::UnixTime min();
};
}

#endif
