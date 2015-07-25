/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Laura Schlimmer, Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "stx/stdtypes.h"
#include "stx/UnixTime.h"
#include "stx/duration.h"
#include "stx/wallclock.h"
#include "stx/option.h"

namespace stx {

enum class HumanDataType {
  UNKNOWN,
  DATETIME,
  DATETIME_NULLABLE,
  URL,
  URL_NULLABLE,
  CURRENCY,
  CURRENCY_NULLABLE,
  UNSIGNED_INTEGER,
  UNSIGNED_INTEGER_NULLABLE,
  SIGNED_INTEGER,
  SIGNED_INTEGER_NULLABLE,
  FLOAT,
  FLOAT_NULLABLE,
  BOOLEAN,
  BOOLEAN_NULLABLE,
  NULL_OR_EMPTY,
  TEXT,
  BINARY
};

class Human {
public:

  static Option<Duration> parseDuration(const String& str);

  static Option<UnixTime> parseTime(
      const String& str,
      UnixTime now = WallClock::now());

  static Option<bool> parseBoolean(const String& value);

  static HumanDataType detectDataType(const String& value);

  static HumanDataType detectDataTypeSeries(
      const String& value,
      HumanDataType prev = HumanDataType::UNKNOWN);

  static bool isNullOrEmpty(const String& value);

};

}
