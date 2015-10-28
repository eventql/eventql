/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *   Copyright (c) 2015 Laura Schlimmer
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <csql/expressions/datetime.h>
#include <csql/svalue.h>

namespace csql {
namespace expressions {

static void checkArgs(const char* symbol, int argc, int argc_expected) {
  if (argc != argc_expected) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for %s. expected: %i, got: %i",
        symbol,
        argc_expected,
        argc);
  }
}

void fromTimestamp(int argc, SValue* argv, SValue* out) {
  checkArgs("FROM_TIMESTAMP", argc, 1);

  SValue tmp = *argv;
  tmp.tryTimeConversion();
  *out = SValue(tmp.getTimestamp());
}

void dateTruncExpr(int argc, SValue* argv, SValue* out) {
  checkArgs("DATE_TRUNC", argc, 2);

  SValue val = argv[1];
  val.tryTimeConversion();

  auto tmp = val.getTimestamp();
  auto time_suffix = argv[0].toString();
  unsigned long long dur = 1;

  if (StringUtil::isNumber(time_suffix.substr(0, 1))) {
    size_t sz;
    dur = std::stoull(time_suffix, &sz);
    time_suffix = time_suffix.substr(sz);
  }

  if (time_suffix == "ms" ||
      time_suffix == "msec" ||
      time_suffix == "msecs" ||
      time_suffix == "millisecond" ||
      time_suffix == "milliseconds") {
    *out = SValue(SValue::TimeType(
        (uint64_t(tmp) / (kMicrosPerMilli * dur)) * kMicrosPerMilli * dur));
    return;
  }

  if (time_suffix == "s" ||
      time_suffix == "sec" ||
      time_suffix == "secs" ||
      time_suffix == "second" ||
      time_suffix == "seconds") {
    *out = SValue(SValue::TimeType(
        (uint64_t(tmp) / (kMicrosPerSecond * dur)) * kMicrosPerSecond * dur));
    return;
  }

  if (time_suffix == "m" ||
      time_suffix == "min" ||
      time_suffix == "mins" ||
      time_suffix == "minute" ||
      time_suffix == "minutes") {
    *out = SValue(SValue::TimeType(
        (uint64_t(tmp) / (kMicrosPerMinute * dur)) * kMicrosPerMinute * dur));
    return;
  }

  if (time_suffix == "h" ||
      time_suffix == "hour" ||
      time_suffix == "hours") {
    *out = SValue(SValue::TimeType(
        (uint64_t(tmp) / (kMicrosPerHour * dur)) * kMicrosPerHour * dur));
    return;
  }

  if (time_suffix == "d" ||
      time_suffix == "day" ||
      time_suffix == "days") {
    *out = SValue(SValue::TimeType(
        (uint64_t(tmp) / (kMicrosPerDay * dur)) * kMicrosPerDay * dur));
    return;
  }

  if (time_suffix == "w" ||
      time_suffix == "week" ||
      time_suffix == "weeks") {
    *out = SValue(SValue::TimeType(
        (uint64_t(tmp) / (kMicrosPerWeek * dur)) * kMicrosPerWeek * dur));
    return;
  }

  if (time_suffix == "m" ||
      time_suffix == "month" ||
      time_suffix == "months") {
    *out = SValue(SValue::TimeType(
        (uint64_t(tmp) / (kMicrosPerDay * 31 * dur)) * kMicrosPerDay * 31 * dur));
    return;
  }

  if (time_suffix == "y" ||
      time_suffix == "year" ||
      time_suffix == "years") {
    *out = SValue(SValue::TimeType(
        (uint64_t(tmp) / (kMicrosPerYear * dur)) * kMicrosPerYear * dur));
    return;
  }

  RAISE(
      kRuntimeError,
      "unknown time precision %s",
      time_suffix.c_str());
}


}
}
