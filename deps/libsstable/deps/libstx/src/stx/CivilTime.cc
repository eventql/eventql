/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/CivilTime.h"
#include <string>
#include <ctime>

namespace stx {

Option<CivilTime> CivilTime::parseString(
    const String& str,
    const char* fmt /* = "%Y-%m-%d %H:%M:%S" */) {
  return CivilTime::parseString(str.data(), str.size(), fmt);
}

Option<CivilTime> CivilTime::parseString(
    const char* str,
    size_t strlen,
    const char* fmt /* = "%Y-%m-%d %H:%M:%S" */) {
  struct tm t;

  // FIXME strptime doesn't handle time zone offsets
  if (strptime(str, fmt, &t) == NULL) {
    return None<CivilTime>();
  } else {
    CivilTime ct;
    ct.setSecond(t.tm_sec);
    ct.setMinute(t.tm_min);
    ct.setHour(t.tm_hour);
    ct.setDay(t.tm_mday);
    ct.setMonth(t.tm_mon + 1);
    ct.setYear(t.tm_year + 1900);
    return Some(ct);
  }
}

void CivilTime::setYear(uint16_t value) {
  year_ = value;
}

void CivilTime::setMonth(uint8_t value) {
  month_ = value;
}

void CivilTime::setDay(uint8_t value) {
  day_ = value;
}

void CivilTime::setHour(uint8_t value) {
  hour_ = value;
}

void CivilTime::setMinute(uint8_t value) {
  minute_ = value;
}

void CivilTime::setSecond(uint8_t value) {
  second_ = value;
}

void CivilTime::setMillisecond(uint16_t value) {
  millisecond_ = value;
}

void CivilTime::setOffset(int32_t value) {
  offset_ = value;
}

}
