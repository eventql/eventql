/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <string>
#include <ctime>
#include "stx/UnixTime.h"
#include "stx/inspect.h"
#include "stx/wallclock.h"
#include "stx/stringutil.h"
#include "stx/ISO8601.h"

namespace stx {

UnixTime::UnixTime() :
    utc_micros_(WallClock::unixMicros()) {}

UnixTime::UnixTime(const CivilTime& civil) {
  uint64_t days = civil.day() - 1;

  for (auto i = 1970; i < civil.year(); ++i) {
    days += 365 + ISO8601::isLeapYear(i);
  }

  for (auto i = 1; i < civil.month(); ++i) {
    days += ISO8601::daysInMonth(civil.year(), i);
  }

  utc_micros_ =
      days * kMicrosPerDay +
      civil.hour() * kMicrosPerHour +
      civil.minute() * kMicrosPerMinute +
      civil.second() * kMicrosPerSecond +
      civil.millisecond() * 1000 +
      civil.offset() * kMicrosPerSecond * -1;
}

UnixTime& UnixTime::operator=(const UnixTime& other) {
  utc_micros_ = other.utc_micros_;
  return *this;
}

UnixTime UnixTime::now() {
  return UnixTime(WallClock::unixMicros());
}

UnixTime UnixTime::daysFromNow(double days) {
  return UnixTime(WallClock::unixMicros() + (days * kMicrosPerDay));
}

std::string UnixTime::toString(const char* fmt) const {
  struct tm tm;
  time_t tt = utc_micros_ / 1000000;
  gmtime_r(&tt, &tm); // FIXPAUL

  char buf[256]; // FIXPAUL
  buf[0] = 0;
  strftime(buf, sizeof(buf), fmt, &tm);

  return std::string(buf);
}

Option<UnixTime> UnixTime::parseString(
    const String& str,
    const char* fmt /* = "%Y-%m-%d %H:%M:%S" */) {
  return UnixTime::parseString(str.data(), str.size(), fmt);
}

Option<UnixTime> UnixTime::parseString(
    const char* str,
    size_t strlen,
    const char* fmt /* = "%Y-%m-%d %H:%M:%S" */) {
  auto ct = CivilTime::parseString(str, strlen, fmt);
  if (ct.isEmpty()) {
    return None<UnixTime>();
  } else {
    return Some(UnixTime(ct.get()));
  }
}

template <>
std::string StringUtil::toString(UnixTime value) {
  return value.toString();
}

template <>
std::string inspect(const UnixTime& value) {
  return value.toString();
}

}

stx::UnixTime
    std::numeric_limits<stx::UnixTime>::min() {
  return stx::UnixTime::epoch();
}

stx::UnixTime
    std::numeric_limits<stx::UnixTime>::max() {
  return stx::UnixTime(std::numeric_limits<uint64_t>::max());
}
