/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2015 Laura Schlimmer, Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord/stdtypes.h>
#include "fnord/exception.h"
#include "fnord/wallclock.h"
#include <fnord/human.h>

namespace fnord {

Option<UnixTime> Human::parseTime(const String& str) {
  /* now */
  if (str == "now") {
    return Some(UnixTime(WallClock::unixMicros()));
  }

  /* -<duration> */
  if (StringUtil::beginsWith(str, "-") && str.size() > 1 && isdigit(str[1])) {
    auto offset = parseDuration(str.substr(1));
    if (!offset.isEmpty()) {
      return Some(
          UnixTime(
              WallClock::unixMicros() - offset.get().microseconds()));
    }
  }

  /* raw timestamps */
  if (StringUtil::isDigitString(str)) {
    static const uint64_t bound_milli = 30000000000;
    static const uint64_t bound_micro = 30000000000000;
    try {
      uint64_t ts = std::stoull(str);

      if (ts < bound_milli) {
        return Some(UnixTime(ts * kMicrosPerSecond));
      }

      if (ts > bound_milli && ts < bound_micro) {
        return Some(UnixTime(ts * kMillisPerSecond));
      }

      if (ts > bound_micro) {
        return Some(UnixTime(ts));
      }
    } catch (...) {
      /* fallthrough */
    }
  }

  //auto isodate = IsoDate::toUnixTime(str);
  //if (!isodate.isEmpty()) {
  //  return isodate.get();
  //}

  return None<UnixTime> ();
}

Option<Duration> Human::parseDuration(const String& str) {
  size_t sz;
  auto value = std::stoull(str, &sz);
  const auto& time_suffix = str.substr(sz);

  if (time_suffix == "s" ||
      time_suffix == "sec" ||
      time_suffix == "secs" ||
      time_suffix == "second" ||
      time_suffix == "seconds") {
    return Some(Duration((value * kMicrosPerSecond)));
  }

  if (time_suffix == "m" ||
      time_suffix == "min" ||
      time_suffix == "mins" ||
      time_suffix == "minute" ||
      time_suffix == "minutes") {
    return Some(Duration(value * kMicrosPerMinute));
  }

  if (time_suffix == "h" ||
      time_suffix == "hour" ||
      time_suffix == "hours") {
    return Some(Duration(value * kMicrosPerHour));
  }

  if (time_suffix == "d" ||
      time_suffix == "day" ||
      time_suffix == "days") {
    return Some(Duration(value * kMicrosPerDay));
  }

  if (time_suffix == "w" ||
      time_suffix == "week" ||
      time_suffix == "weeks") {
    return Some(Duration(value * kMicrosPerWeek));
  }

  if (time_suffix == "y" ||
      time_suffix == "year" ||
      time_suffix == "years") {
    return Some(Duration(value * kMicrosPerYear));
  }

  return None<Duration>();
}


} // namespace fnord

