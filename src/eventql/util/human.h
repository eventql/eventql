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
#pragma once
#include "eventql/util/stdtypes.h"
#include "eventql/util/UnixTime.h"
#include "eventql/util/duration.h"
#include "eventql/util/wallclock.h"
#include "eventql/util/option.h"

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

