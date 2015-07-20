/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "fnord/stdtypes.h"
#include "fnord/UnixTime.h"

namespace fnord {

class CivilTime {
public:

  /**
   * Create a new CivilTime instance from the provided ISO8601 string
   */
  static CivilTime parseISO861(const String& str);

  /**
   * Create a new CivilTime instance with time = now
   */
  CivilTime();

  /**
   * Create a new CivilTime instance with all fields set to zero
   */
  CivilTime(std::nullptr_t);

  uint16_t year() const;
  uint8_t month() const;
  uint8_t day() const;
  uint8_t hour() const;
  uint8_t minute() const;
  uint8_t second() const;
  uint16_t millisecond() const;
  uint32_t offset() const;

  void setYear(uint16_t value);
  void setMonth(uint8_t value);
  void setDay(uint8_t value);
  void setHour(uint8_t value);
  void setMinute(uint8_t value);
  void setSecond(uint8_t value);
  void setMillisecond(uint16_t value);
  void setOffset(uint32_t value);

  UnixTime toUnixTime();

protected:
  uint16_t year_;
  uint8_t month_;
  uint8_t day_;
  uint8_t hour_;
  uint8_t minute_;
  uint8_t second_;
  uint16_t millisecond_;
  uint32_t offset_;
};

}
