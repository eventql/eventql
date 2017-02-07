/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <eventql/util/inspect.h>
#include <eventql/util/human.h>
#include <eventql/util/wallclock.h>
#include <eventql/sql/expressions/datetime.h>
#include <eventql/sql/svalue.h>

namespace csql {
namespace expressions {

void now_call(sql_txn* ctx, VMStack* stack) {
  pushTimestamp64(stack, WallClock::unixMicros());
}

const SFunction now(
    {  },
    SType::TIMESTAMP64,
    &now_call);

void to_timestamp_int64_call(sql_txn* ctx, VMStack* stack) {
  auto value = popInt64(stack);
  pushTimestamp64(stack, value * kMicrosPerSecond);
}

const SFunction to_timestamp_int64(
    { SType::INT64 },
    SType::TIMESTAMP64,
    &to_timestamp_int64_call);

void to_timestamp_float64_call(sql_txn* ctx, VMStack* stack) {
  auto value = popFloat64(stack);
  pushTimestamp64(stack, value * kMicrosPerSecond);
}

const SFunction to_timestamp_float64(
    { SType::FLOAT64 },
    SType::TIMESTAMP64,
    &to_timestamp_float64_call);

static Option<uint64_t> parseInterval(String time_interval) {
  uint64_t num;
  String unit;

  try {
    size_t sz;
    num = std::stoull(time_interval, &sz);
    unit = time_interval.substr(sz);
    StringUtil::toLower(&unit);

  } catch (std::invalid_argument e) {
    RAISEF(
      kRuntimeError,
      "TIME_AT: invalid argument $0",
      time_interval);
  }

  if (unit == "sec" ||
      unit == "secs" ||
      unit == "second" ||
      unit == "seconds") {
    return Some(num * kMicrosPerSecond);
  }

  if (unit == "min" ||
      unit == "mins" ||
      unit == "minute" ||
      unit == "minutes") {
    return Some(num * kMicrosPerMinute);
  }

  if (unit == "h" ||
      unit == "hour" ||
      unit == "hours") {
    return Some(num * kMicrosPerHour);
  }

  if (unit == "d" ||
      unit == "day" ||
      unit == "days") {
    return Some(num * kMicrosPerDay);
  }

  if (unit == "w" ||
      unit == "week" ||
      unit == "weeks") {
    return Some(num * kMicrosPerWeek);
  }

  if (unit == "month" ||
      unit == "months") {
    return Some(num * kMicrosPerDay * 31);
  }

  if (unit == "y" ||
      unit == "year" ||
      unit == "years") {
    return Some(num * kMicrosPerYear);
  }

  return None<uint64_t>();
}

//void fromTimestamp(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
//  checkArgs("FROM_TIMESTAMP", argc, 1);
//
//  switch (argv->getType()) {
//    case SType::TIMESTAMP64:
//      *out = *argv;
//      break;
//    default:
//      *out = SValue(SValue::TimeType(parseTimestamp(argv)));
//      break;
//  }
//}

//
//void dateTruncExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
//  checkArgs("DATE_TRUNC", argc, 2);
//
//  auto time_suffix = argv[0].getString();
//  uint64_t val = argv[1].getTimestamp().unixMicros();
//  unsigned long long dur = 1;
//
//  if (StringUtil::isNumber(time_suffix.substr(0, 1))) {
//    size_t sz;
//    dur = std::stoull(time_suffix, &sz);
//    time_suffix = time_suffix.substr(sz);
//  }
//
//  if (time_suffix == "ms" ||
//      time_suffix == "msec" ||
//      time_suffix == "msecs" ||
//      time_suffix == "millisecond" ||
//      time_suffix == "milliseconds") {
//    *out = SValue(SValue::TimeType(
//        (uint64_t(val) / (kMicrosPerMilli * dur)) * kMicrosPerMilli * dur));
//    return;
//  }
//
//  if (time_suffix == "s" ||
//      time_suffix == "sec" ||
//      time_suffix == "secs" ||
//      time_suffix == "second" ||
//      time_suffix == "seconds") {
//    *out = SValue(SValue::TimeType(
//        (uint64_t(val) / (kMicrosPerSecond * dur)) * kMicrosPerSecond * dur));
//    return;
//  }
//
//  if (time_suffix == "m" ||
//      time_suffix == "min" ||
//      time_suffix == "mins" ||
//      time_suffix == "minute" ||
//      time_suffix == "minutes") {
//    *out = SValue(SValue::TimeType(
//        (uint64_t(val) / (kMicrosPerMinute * dur)) * kMicrosPerMinute * dur));
//    return;
//  }
//
//  if (time_suffix == "h" ||
//      time_suffix == "hour" ||
//      time_suffix == "hours") {
//    *out = SValue(SValue::TimeType(
//        (uint64_t(val) / (kMicrosPerHour * dur)) * kMicrosPerHour * dur));
//    return;
//  }
//
//  if (time_suffix == "d" ||
//      time_suffix == "day" ||
//      time_suffix == "days") {
//    *out = SValue(SValue::TimeType(
//        (uint64_t(val) / (kMicrosPerDay * dur)) * kMicrosPerDay * dur));
//    return;
//  }
//
//  if (time_suffix == "w" ||
//      time_suffix == "week" ||
//      time_suffix == "weeks") {
//    *out = SValue(SValue::TimeType(
//        (uint64_t(val) / (kMicrosPerWeek * dur)) * kMicrosPerWeek * dur));
//    return;
//  }
//
//  if (time_suffix == "m" ||
//      time_suffix == "month" ||
//      time_suffix == "months") {
//    *out = SValue(SValue::TimeType(
//        (uint64_t(val) / (kMicrosPerDay * 31 * dur)) * kMicrosPerDay * 31 * dur));
//    return;
//  }
//
//  if (time_suffix == "y" ||
//      time_suffix == "year" ||
//      time_suffix == "years") {
//    *out = SValue(SValue::TimeType(
//        (uint64_t(val) / (kMicrosPerYear * dur)) * kMicrosPerYear * dur));
//    return;
//  }
//
//  RAISE(
//      kRuntimeError,
//      "unknown time precision %s",
//      time_suffix.c_str());
//}
//
//void dateAddExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
//  checkArgs("DATE_ADD", argc, 3);
//
//  SValue val = argv[0];
//  auto date = val.getTimestamp();
//  auto unit = argv[2].getString();
//  StringUtil::toLower(&unit);
//
//  if (unit == "second") {
//    if (argv[1].isConvertibleToNumeric()) {
//      *out = SValue(SValue::TimeType(
//          uint64_t(date) + (argv[1].getFloat() * kMicrosPerSecond)));
//      return;
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_ADD: invalid expression $0 for unit $1",
//        argv[1].getString(),
//        argv[2].getString());
//  }
//
//  if (unit == "minute") {
//    if (argv[1].isConvertibleToNumeric()) {
//      *out = SValue(SValue::TimeType(
//          uint64_t(date) + (argv[1].getFloat() * kMicrosPerMinute)));
//        return;
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_ADD: invalid expression $0 for unit $1",
//        argv[1].getString(),
//        argv[2].getString());
//  }
//
//  if (unit == "hour") {
//    if (argv[1].isConvertibleToNumeric()) {
//      *out = SValue(SValue::TimeType(
//          uint64_t(date) + (argv[1].getFloat() * kMicrosPerHour)));
//      return;
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_ADD: invalid expression $0 for unit $1",
//        argv[1].getString(),
//        argv[2].getString());
//  }
//
//  if (unit == "day") {
//    if (argv[1].isConvertibleToNumeric()) {
//      *out = SValue(SValue::TimeType(
//          uint64_t(date) + (argv[1].getFloat() * kMicrosPerDay)));
//      return;
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_ADD: invalid expression $0 for unit $1",
//        argv[1].getString(),
//        argv[2].getString());
//  }
//
//  if (unit == "week") {
//    if (argv[1].isConvertibleToNumeric()) {
//      *out = SValue(SValue::TimeType(
//          uint64_t(date) + (argv[1].getFloat() * kMicrosPerWeek)));
//      return;
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_ADD: invalid expression $0 for unit $1",
//        argv[1].getString(),
//        argv[2].getString());
//  }
//
//  if (unit == "month") {
//    if (argv[1].isConvertibleToNumeric()) {
//      *out = SValue(SValue::TimeType(
//          uint64_t(date) + (argv[1].getFloat() * kMicrosPerDay * 31)));
//      return;
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_ADD: invalid expression $0 for unit $1",
//        argv[1].getString(),
//        argv[2].getString());
//  }
//
//  if (unit == "year") {
//    if (argv[1].isConvertibleToNumeric()) {
//      *out = SValue(SValue::TimeType(
//          uint64_t(date) + (argv[1].getFloat() * kMicrosPerYear)));
//      return;
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_ADD: invalid expression $0 for unit $1",
//        argv[1].getString(),
//        argv[2].getString());
//  }
//
//  auto expr = argv[1].getString();
//  if (unit == "minute_second") {
//    auto values = StringUtil::split(expr, ":");
//    if (values.size() == 2 &&
//        StringUtil::isNumber(values[0]) &&
//        StringUtil::isNumber(values[1])) {
//
//      try {
//        *out = SValue(SValue::TimeType(
//            uint64_t(date) +
//            (std::stoull(values[0]) * kMicrosPerMinute) +
//            (std::stoull(values[1]) * kMicrosPerSecond)));
//        return;
//      } catch (std::invalid_argument e) {
//        /* fallthrough */
//      }
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_ADD: invalid expression $0 for unit $1",
//        expr,
//        argv[2].getString());
//  }
//
//  if (unit == "hour_second") {
//    auto values = StringUtil::split(expr, ":");
//    if (values.size() == 3 &&
//        StringUtil::isNumber(values[0]) &&
//        StringUtil::isNumber(values[1]) &&
//        StringUtil::isNumber(values[2])) {
//
//      try {
//        *out = SValue(SValue::TimeType(
//            uint64_t(date) +
//            (std::stoull(values[0]) * kMicrosPerHour) +
//            (std::stoull(values[1]) * kMicrosPerMinute) +
//            (std::stoull(values[2]) * kMicrosPerSecond)));
//        return;
//      } catch (std::invalid_argument e) {
//        /* fallthrough */
//      }
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_ADD: invalid expression $0 for unit $1",
//        expr,
//        argv[2].getString());
//  }
//
//  if (unit == "hour_minute") {
//    auto values = StringUtil::split(expr, ":");
//    if (values.size() == 2 &&
//        StringUtil::isNumber(values[0]) &&
//        StringUtil::isNumber(values[1])) {
//
//      try {
//        *out = SValue(SValue::TimeType(
//            uint64_t(date) +
//            (std::stoull(values[0]) * kMicrosPerHour) +
//            (std::stoull(values[1]) * kMicrosPerMinute)));
//        return;
//      } catch (std::invalid_argument e) {
//        /* fallthrough */
//      }
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_ADD: invalid expression $0 for unit $1",
//        expr,
//        argv[2].getString());
//  }
//
//  if (unit == "day_second") {
//    auto values = StringUtil::split(expr, " ");
//    if (values.size() == 2 && StringUtil::isNumber(values[0])) {
//
//      auto time_values = StringUtil::split(values[1], ":");
//      if (time_values.size() == 3 &&
//          StringUtil::isNumber(time_values[0]) &&
//          StringUtil::isNumber(time_values[1]) &&
//          StringUtil::isNumber(time_values[2])) {
//
//        try {
//          *out = SValue(SValue::TimeType(
//              uint64_t(date) +
//              (std::stoull(values[0]) * kMicrosPerDay) +
//              (std::stoull(time_values[0]) * kMicrosPerHour) +
//              (std::stoull(time_values[1]) * kMicrosPerMinute) +
//              (std::stoull(time_values[2]) * kMicrosPerSecond)));
//          return;
//        } catch (std::invalid_argument e) {
//          /* fallthrough */
//        }
//      }
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_ADD: invalid expression $0 for unit $1",
//        expr,
//        argv[2].getString());
//  }
//
//  if (unit == "day_minute") {
//    auto values = StringUtil::split(expr, " ");
//    if (values.size() == 2 && StringUtil::isNumber(values[0])) {
//
//      auto time_values = StringUtil::split(values[1], ":");
//      if (time_values.size() == 2 &&
//          StringUtil::isNumber(time_values[0]) &&
//          StringUtil::isNumber(time_values[1])) {
//
//        try {
//          *out = SValue(SValue::TimeType(
//              uint64_t(date) +
//              (std::stoull(values[0]) * kMicrosPerDay) +
//              (std::stoull(time_values[0]) * kMicrosPerHour) +
//              (std::stoull(time_values[1]) * kMicrosPerMinute)));
//          return;
//        } catch (std::invalid_argument e) {
//          /* fallthrough */
//        }
//      }
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_ADD: invalid expression $0 for unit $1",
//        expr,
//        argv[2].getString());
//  }
//
//  if (unit == "day_hour") {
//    auto values = StringUtil::split(expr, " ");
//    if (values.size() == 2 &&
//        StringUtil::isNumber(values[0]) &&
//        StringUtil::isNumber(values[1])) {
//
//      try {
//        *out = SValue(SValue::TimeType(
//            uint64_t(date) +
//            (std::stoull(values[0]) * kMicrosPerDay) +
//            (std::stoull(values[1]) * kMicrosPerHour)));
//        return;
//      } catch (std::invalid_argument e) {
//        /* fallthrough */
//      }
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_ADD: invalid expression $0 for unit $1",
//        expr,
//        argv[2].getString());
//  }
//
//  if (unit == "year_month") {
//    auto values = StringUtil::split(expr, "-");
//    if (values.size() == 2 &&
//        StringUtil::isNumber(values[0]) &&
//        StringUtil::isNumber(values[1])) {
//
//      try {
//        *out = SValue(SValue::TimeType(
//            uint64_t(date) +
//            (std::stoull(values[0]) * kMicrosPerYear) +
//            (std::stoull(values[1]) * kMicrosPerDay * 31)));
//        return;
//      } catch (std::invalid_argument e) {
//        /* fallthrough */
//      }
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_ADD: invalid expression $0 for unit $1",
//        expr,
//        argv[2].getString());
//  }
//
//  RAISEF(
//      kRuntimeError,
//      "DATE_ADD: invalid unit $0",
//      argv[2].getString());
//}
//
//void dateSubExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
//  checkArgs("DATE_SUB", argc, 3);
//
//  SValue val = argv[0];
//  auto date = val.getTimestamp();
//  auto unit = argv[2].getString();
//  StringUtil::toLower(&unit);
//
//  if (unit == "second") {
//    if (argv[1].isConvertibleToNumeric()) {
//      *out = SValue(SValue::TimeType(
//          uint64_t(date) - (argv[1].getFloat() * kMicrosPerSecond)));
//      return;
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_SUB: invalid expression $0 for unit $1",
//        argv[1].getString(),
//        argv[2].getString());
//  }
//
//  if (unit == "minute") {
//    if (argv[1].isConvertibleToNumeric()) {
//      *out = SValue(SValue::TimeType(
//          uint64_t(date) - (argv[1].getFloat() * kMicrosPerMinute)));
//      return;
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_SUB: invalid expression $0 for unit $1",
//        argv[1].getString(),
//        argv[2].getString());
//  }
//
//  if (unit == "hour") {
//    if (argv[1].isConvertibleToNumeric()) {
//      *out = SValue(SValue::TimeType(
//          uint64_t(date) - (argv[1].getFloat() * kMicrosPerHour)));
//      return;
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_SUB: invalid expression $0 for unit $1",
//        argv[1].getString(),
//        argv[2].getString());
//  }
//
//  if (unit == "day") {
//    if (argv[1].isConvertibleToNumeric()) {
//      *out = SValue(SValue::TimeType(
//          uint64_t(date) - (argv[1].getFloat() * kMicrosPerDay)));
//      return;
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_SUB: invalid expression $0 for unit $1",
//        argv[1].getString(),
//        argv[2].getString());
//  }
//
//  if (unit == "week") {
//    if (argv[1].isConvertibleToNumeric()) {
//      *out = SValue(SValue::TimeType(
//          uint64_t(date) - (argv[1].getFloat() * kMicrosPerWeek)));
//      return;
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_SUB: invalid expression $0 for unit $1",
//        argv[1].getString(),
//        argv[2].getString());
//  }
//
//  if (unit == "month") {
//    if (argv[1].isConvertibleToNumeric()) {
//      *out = SValue(SValue::TimeType(
//          uint64_t(date) - (argv[1].getFloat() * kMicrosPerDay * 31)));
//      return;
//    }
//    RAISEF(
//        kRuntimeError,
//        "DATE_SUB: invalid expression $0 for unit $1",
//        argv[1].getString(),
//        argv[2].getString());
//  }
//
//  if (unit == "year") {
//    if (argv[1].isConvertibleToNumeric()) {
//      *out = SValue(SValue::TimeType(
//          uint64_t(date) - (argv[1].getFloat() * kMicrosPerYear)));
//      return;
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_SUB: invalid expression $0 for unit $1",
//        argv[1].getString(),
//        argv[2].getString());
//  }
//
//  auto expr = argv[1].getString();
//  if (unit == "minute_second") {
//    auto values = StringUtil::split(expr, ":");
//    if (values.size() == 2 &&
//        StringUtil::isNumber(values[0]) &&
//        StringUtil::isNumber(values[1])) {
//
//      try {
//        *out = SValue(SValue::TimeType(
//            uint64_t(date) -
//            (std::stoull(values[0]) * kMicrosPerMinute) +
//            (std::stoull(values[1]) * kMicrosPerSecond)));
//        return;
//      } catch (std::invalid_argument e) {
//        /* fallthrough */
//      }
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_SUB: invalid expression $0 for unit $1",
//        expr,
//        argv[2].getString());
//  }
//
//  if (unit == "hour_second") {
//    auto values = StringUtil::split(expr, ":");
//    if (values.size() == 3 &&
//        StringUtil::isNumber(values[0]) &&
//        StringUtil::isNumber(values[1]) &&
//        StringUtil::isNumber(values[2])) {
//
//      try {
//        *out = SValue(SValue::TimeType(
//            uint64_t(date) -
//            (std::stoull(values[0]) * kMicrosPerHour) +
//            (std::stoull(values[1]) * kMicrosPerMinute) +
//            (std::stoull(values[2]) * kMicrosPerSecond)));
//        return;
//      } catch (std::invalid_argument e) {
//        /* fallthrough */
//      }
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_SUB: invalid expression $0 for unit $1",
//        expr,
//        argv[2].getString());
//  }
//
//  if (unit == "hour_minute") {
//    auto values = StringUtil::split(expr, ":");
//    if (values.size() == 2 &&
//        StringUtil::isNumber(values[0]) &&
//        StringUtil::isNumber(values[1])) {
//
//      try {
//        *out = SValue(SValue::TimeType(
//            uint64_t(date) -
//            (std::stoull(values[0]) * kMicrosPerHour) +
//            (std::stoull(values[1]) * kMicrosPerMinute)));
//        return;
//      } catch (std::invalid_argument e) {
//        /* fallthrough */
//      }
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_SUB: invalid expression $0 for unit $1",
//        expr,
//        argv[2].getString());
//  }
//
//  if (unit == "day_second") {
//    auto values = StringUtil::split(expr, " ");
//    if (values.size() == 2 && StringUtil::isNumber(values[0])) {
//
//      auto time_values = StringUtil::split(values[1], ":");
//      if (time_values.size() == 3 &&
//          StringUtil::isNumber(time_values[0]) &&
//          StringUtil::isNumber(time_values[1]) &&
//          StringUtil::isNumber(time_values[2])) {
//
//        try {
//          *out = SValue(SValue::TimeType(
//              uint64_t(date) -
//              (std::stoull(values[0]) * kMicrosPerDay) +
//              (std::stoull(time_values[0]) * kMicrosPerHour) +
//              (std::stoull(time_values[1]) * kMicrosPerMinute) +
//              (std::stoull(time_values[2]) * kMicrosPerSecond)));
//          return;
//        } catch (std::invalid_argument e) {
//          /* fallthrough */
//        }
//      }
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_SUB: invalid expression $0 for unit $1",
//        expr,
//        argv[2].getString());
//  }
//
//  if (unit == "day_minute") {
//    auto values = StringUtil::split(expr, " ");
//    if (values.size() == 2 && StringUtil::isNumber(values[0])) {
//
//      auto time_values = StringUtil::split(values[1], ":");
//      if (time_values.size() == 2 &&
//          StringUtil::isNumber(time_values[0]) &&
//          StringUtil::isNumber(time_values[1])) {
//
//        try {
//          *out = SValue(SValue::TimeType(
//              uint64_t(date) -
//              (std::stoull(values[0]) * kMicrosPerDay) +
//              (std::stoull(time_values[0]) * kMicrosPerHour) +
//              (std::stoull(time_values[1]) * kMicrosPerMinute)));
//          return;
//        } catch (std::invalid_argument e) {
//          /* fallthrough */
//        }
//      }
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_SUB: invalid expression $0 for unit $1",
//        expr,
//        argv[2].getString());
//  }
//
//  if (unit == "day_hour") {
//    auto values = StringUtil::split(expr, " ");
//    if (values.size() == 2 &&
//        StringUtil::isNumber(values[0]) &&
//        StringUtil::isNumber(values[1])) {
//
//      try {
//        *out = SValue(SValue::TimeType(
//            uint64_t(date) -
//            (std::stoull(values[0]) * kMicrosPerDay) +
//            (std::stoull(values[1]) * kMicrosPerHour)));
//        return;
//      } catch (std::invalid_argument e) {
//        /* fallthrough */
//      }
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_SUB: invalid expression $0 for unit $1",
//        expr,
//        argv[2].getString());
//  }
//
//  if (unit == "year_month") {
//    auto values = StringUtil::split(expr, "-");
//    if (values.size() == 2 &&
//        StringUtil::isNumber(values[0]) &&
//        StringUtil::isNumber(values[1])) {
//
//      try {
//        *out = SValue(SValue::TimeType(
//            uint64_t(date) -
//            (std::stoull(values[0]) * kMicrosPerYear) +
//            (std::stoull(values[1]) * kMicrosPerDay * 31)));
//        return;
//      } catch (std::invalid_argument e) {
//        /* fallthrough */
//      }
//    }
//
//    RAISEF(
//        kRuntimeError,
//        "DATE_SUB: invalid expression $0 for unit $1",
//        expr,
//        argv[2].getString());
//  }
//
//  RAISEF(
//      kRuntimeError,
//      "DATE_SUB: invalid unit $0",
//      argv[2].getString());
//}

void time_at_call(sql_txn* ctx, VMStack* stack) {
  auto time_str = popString(stack);

  StringUtil::toLower(&time_str);
  if (time_str == "now") {
    pushTimestamp64(stack, WallClock::unixMicros());
    return;
  }

  if (StringUtil::beginsWith(time_str, "-")) {
    try {
      auto now = uint64_t(WallClock::now());
      Option<uint64_t> time_val = parseInterval(time_str.substr(1));
      if (!time_val.isEmpty()) {
        pushTimestamp64(stack, now - time_val.get());
        return;
      }
    } catch (...) {
      RAISEF(
        kRuntimeError,
        "TIME_AT: invalid argument $0",
        time_str);
    }
  }

  if (StringUtil::endsWith(time_str, "ago")) {
    try {
      auto now = uint64_t(WallClock::now());
      Option<uint64_t> time_val = parseInterval(
          time_str.substr(0, time_str.length() - 4));
      if (!time_val.isEmpty()) {
        pushTimestamp64(stack, now - time_val.get());
        return;
      }
    } catch (...) {
      RAISEF(
        kRuntimeError,
        "TIME_AT: invalid argument $0",
        time_str);
    }
  }

  auto time_opt = Human::parseTime(time_str);
  if (time_opt.isEmpty()) {
    RAISEF(
       kTypeError,
        "can't convert '$0' to TIMESTAMP",
        time_str);
  }

  pushTimestamp64(stack, time_opt.get().unixMicros());
}

const SFunction time_at(
    { SType::STRING },
    SType::TIMESTAMP64,
    &time_at_call);

}
}
