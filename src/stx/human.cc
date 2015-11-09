/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Laura Schlimmer, Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/stdtypes.h>
#include "stx/exception.h"
#include "stx/UTF8.h"
#include <stx/human.h>
#include <stx/ISO8601.h>

namespace stx {

Option<UnixTime> Human::parseTime(
    const String& str,
    UnixTime now /* = WallClock::now() */) {
  /* now */
  if (str == "now") {
    return Some(now);
  }

  /* -<duration> */
  if (StringUtil::beginsWith(str, "-") && str.size() > 1 && isdigit(str[1])) {
    auto offset = parseDuration(str.substr(1));
    if (!offset.isEmpty()) {
      return Some(UnixTime(now.unixMicros() - offset.get().microseconds()));
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

  auto civil = ISO8601::parse(str);
  if (!civil.isEmpty()) {
    return Some(UnixTime(civil.get()));
  }

  auto civil_time = CivilTime::parseString(str);
  if (!civil_time.isEmpty()) {
    return Some(UnixTime(civil_time.get()));
  }

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

Option<bool> Human::parseBoolean(const String& value) {
  if (value == "true" ||
      value == "TRUE" ||
      value == "yes" ||
      value == "YES") {
    return Some(true);
  }

  if (value == "false" ||
      value == "FALSE" ||
      value == "no" ||
      value == "NO") {
    return Some(false);
  }

  return None<bool>();
}

HumanDataType Human::detectDataType(const String& value) {
  /* number like types */
  if (StringUtil::isNumber(value)) {
    if (StringUtil::includes(value, ".") || StringUtil::includes(value, ",")) {
      return HumanDataType::FLOAT;
    }

    if (StringUtil::beginsWith(value, "-")) {
      return HumanDataType::SIGNED_INTEGER;
    } else {
      return HumanDataType::UNSIGNED_INTEGER;
    }
  }

  /* null */
  if (isNullOrEmpty(value)) {
    return HumanDataType::NULL_OR_EMPTY;
  }

  /* boolean */
  if (value == "true" ||
      value == "TRUE" ||
      value == "false" ||
      value == "FALSE" ||
      value == "yes" ||
      value == "YES" ||
      value == "no" ||
      value == "NO") {
    return HumanDataType::BOOLEAN;
  }

  auto time = parseTime(value);
  if (!time.isEmpty()) {
    return HumanDataType::DATETIME;
  }

  if (UTF8::isValidUTF8(value)) {
    return HumanDataType::TEXT;
  }

  return HumanDataType::BINARY;
}

HumanDataType Human::detectDataTypeSeries(
    const String& value,
    HumanDataType prev /* = HumanDataType::UNKNOWN */) {
  if (prev == HumanDataType::BINARY) {
    return prev;
  }

  auto type = detectDataType(value);

  if (type == prev || prev == HumanDataType::UNKNOWN) {
    return type; // fastpath
  }

  switch (type) {

    case HumanDataType::UNKNOWN:
      return HumanDataType::UNKNOWN;

    case HumanDataType::TEXT:
      return HumanDataType::TEXT;

    case HumanDataType::BINARY:
      return HumanDataType::BINARY;

    case HumanDataType::NULL_OR_EMPTY:
      switch (prev) {
        case HumanDataType::DATETIME:
        case HumanDataType::DATETIME_NULLABLE:
          return HumanDataType::DATETIME_NULLABLE;

        case HumanDataType::URL:
        case HumanDataType::URL_NULLABLE:
          return HumanDataType::URL_NULLABLE;

        case HumanDataType::CURRENCY:
        case HumanDataType::CURRENCY_NULLABLE:
          return HumanDataType::CURRENCY_NULLABLE;

        case HumanDataType::UNSIGNED_INTEGER:
        case HumanDataType::UNSIGNED_INTEGER_NULLABLE:
          return HumanDataType::UNSIGNED_INTEGER_NULLABLE;

        case HumanDataType::SIGNED_INTEGER:
        case HumanDataType::SIGNED_INTEGER_NULLABLE:
          return HumanDataType::SIGNED_INTEGER_NULLABLE;

        case HumanDataType::FLOAT:
        case HumanDataType::FLOAT_NULLABLE:
          return HumanDataType::FLOAT_NULLABLE;

        case HumanDataType::BOOLEAN:
        case HumanDataType::BOOLEAN_NULLABLE:
          return HumanDataType::BOOLEAN_NULLABLE;

        case HumanDataType::TEXT:
          return HumanDataType::TEXT;

        case HumanDataType::BINARY:
          return HumanDataType::BINARY;

        case HumanDataType::UNKNOWN:
        case HumanDataType::NULL_OR_EMPTY:
          return HumanDataType::NULL_OR_EMPTY;

      }
      break;

    case HumanDataType::DATETIME:
    case HumanDataType::DATETIME_NULLABLE:
      switch (prev) {
        case HumanDataType::NULL_OR_EMPTY:
        case HumanDataType::DATETIME_NULLABLE:
           return HumanDataType::DATETIME_NULLABLE;
        default:
          break;
      }
      break;

    case HumanDataType::URL:
    case HumanDataType::URL_NULLABLE:
      switch (prev) {
        case HumanDataType::NULL_OR_EMPTY:
        case HumanDataType::URL_NULLABLE:
           return HumanDataType::URL_NULLABLE;
        default:
          break;
      }
      break;

    case HumanDataType::CURRENCY:
    case HumanDataType::CURRENCY_NULLABLE:
      switch (prev) {
        case HumanDataType::NULL_OR_EMPTY:
        case HumanDataType::CURRENCY_NULLABLE:
           return HumanDataType::CURRENCY_NULLABLE;
        default:
          break;
      }
      break;

    case HumanDataType::UNSIGNED_INTEGER:
      switch (prev) {
        case HumanDataType::FLOAT:
           return HumanDataType::FLOAT;
        case HumanDataType::SIGNED_INTEGER:
           return HumanDataType::SIGNED_INTEGER;
        default:
          { /* fallthrough */ }
      }
      /* fallthrough */

    case HumanDataType::UNSIGNED_INTEGER_NULLABLE:
      switch (prev) {
        case HumanDataType::FLOAT:
        case HumanDataType::FLOAT_NULLABLE:
           return HumanDataType::FLOAT_NULLABLE;

        case HumanDataType::SIGNED_INTEGER:
        case HumanDataType::SIGNED_INTEGER_NULLABLE:
           return HumanDataType::SIGNED_INTEGER_NULLABLE;

        case HumanDataType::NULL_OR_EMPTY:
        case HumanDataType::UNSIGNED_INTEGER:
        case HumanDataType::UNSIGNED_INTEGER_NULLABLE:
           return HumanDataType::UNSIGNED_INTEGER_NULLABLE;

        default:
          break;
      }
      break;

    case HumanDataType::SIGNED_INTEGER:
      switch (prev) {
        case HumanDataType::FLOAT:
           return HumanDataType::FLOAT;
        case HumanDataType::UNSIGNED_INTEGER:
           return HumanDataType::SIGNED_INTEGER;
        default:
          { /* fallthrough */ }
      }
      /* fallthrough */

    case HumanDataType::SIGNED_INTEGER_NULLABLE:
      switch (prev) {
        case HumanDataType::FLOAT:
        case HumanDataType::FLOAT_NULLABLE:
           return HumanDataType::FLOAT_NULLABLE;

        case HumanDataType::NULL_OR_EMPTY:
        case HumanDataType::UNSIGNED_INTEGER:
        case HumanDataType::UNSIGNED_INTEGER_NULLABLE:
        case HumanDataType::SIGNED_INTEGER:
        case HumanDataType::SIGNED_INTEGER_NULLABLE:
           return HumanDataType::SIGNED_INTEGER_NULLABLE;
        default:
          break;
      }
      break;

    case HumanDataType::FLOAT:
      switch (prev) {
        case HumanDataType::UNSIGNED_INTEGER:
        case HumanDataType::SIGNED_INTEGER:
           return HumanDataType::FLOAT;
        default:
          { /* fallthrough */ }
      }
      /* fallthrough */

    case HumanDataType::FLOAT_NULLABLE:
      switch (prev) {
        case HumanDataType::NULL_OR_EMPTY:
        case HumanDataType::FLOAT_NULLABLE:
        case HumanDataType::UNSIGNED_INTEGER:
        case HumanDataType::UNSIGNED_INTEGER_NULLABLE:
        case HumanDataType::SIGNED_INTEGER:
        case HumanDataType::SIGNED_INTEGER_NULLABLE:
           return HumanDataType::FLOAT_NULLABLE;
        default:
          break;
      }
      break;

    case HumanDataType::BOOLEAN:
    case HumanDataType::BOOLEAN_NULLABLE:
      switch (prev) {
        case HumanDataType::NULL_OR_EMPTY:
        case HumanDataType::BOOLEAN_NULLABLE:
           return HumanDataType::BOOLEAN_NULLABLE;
        default:
          break;
      }
      break;

  }

  return HumanDataType::TEXT;
}

bool Human::isNullOrEmpty(const String& value) {
  return
      value.empty() ||
      value == "null" ||
      value == "NULL";
}

} // namespace stx

