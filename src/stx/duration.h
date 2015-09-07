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
#include "stx/time_constants.h"

namespace stx {

class Duration {
public:
  static constexpr const uint64_t MicrosPerSecond = 1000000;
  static constexpr const uint64_t MillisPerSecond = 1000;
  static constexpr const uint64_t SecondsPerMinute = 60;
  static constexpr const uint64_t MinutesPerHour = 60;
  static constexpr const uint64_t SecondsPerHour = SecondsPerMinute * MinutesPerHour;
  static constexpr const uint64_t MillisPerHour = SecondsPerHour * MillisPerSecond;
  static constexpr const uint64_t MicrosPerHour = SecondsPerHour * MicrosPerSecond;
  static constexpr const uint64_t HoursPerDay = 24;
  static constexpr const uint64_t SecondsPerDay = SecondsPerHour * HoursPerDay;
  static constexpr const uint64_t MillisPerDay = SecondsPerDay * MillisPerSecond;
  static constexpr const uint64_t MicrosPerDay = SecondsPerDay * MicrosPerSecond;

  /**
   * Create a new Duration
   *
   * @param microseconds the duration in microseconds
   */
  Duration(uint64_t microseconds);

  bool operator==(const Duration& other) const;
  bool operator<(const Duration& other) const;
  bool operator>(const Duration& other) const;
  bool operator<=(const Duration& other) const;
  bool operator>=(const Duration& other) const;

  Duration operator+(const Duration& other) const;

  /**
   * Return the represented duration in microseconds
   */
  explicit operator uint64_t() const;

  /**
   * Return the represented duration in microseconds
   */
  explicit operator double() const;

  /**
   * Return the represented duration in microseconds
   */
  uint64_t microseconds() const noexcept;

  /**
   * Return the represented duration in seconds
   */
  uint64_t seconds() const noexcept;

  uint64_t minutes() const noexcept { return seconds() / SecondsPerMinute; }
  uint64_t hours() const noexcept { return minutes() / MinutesPerHour; }
  uint64_t days() const noexcept { return hours() / HoursPerDay; }

  static Duration fromDays(uint64_t v) { return Duration(v * MicrosPerSecond * SecondsPerDay); }
  static Duration fromHours(uint64_t v) { return Duration(v * MicrosPerSecond * SecondsPerHour); }
  static Duration fromMinutes(uint64_t v) { return Duration(v * MicrosPerSecond * SecondsPerMinute); }
  static Duration fromSeconds(uint64_t v) { return Duration(v * MicrosPerSecond); }
  static Duration fromMilliseconds(uint64_t v) { return Duration(v * 1000); }
  static Duration fromMicroseconds(uint64_t v) { return Duration(v); }
  static Duration fromNanoseconds(uint64_t v) { return Duration(v / 1000); }

protected:
  uint64_t micros_;
};

}
#endif
