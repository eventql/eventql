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

CivilTime::CivilTime() :
    year_(0),
    month_(0),
    day_(0),
    hour_(0),
    minute_(0),
    second_(0),
    millisecond_(0),
    offset_(0) {}


CivilTime::CivilTime(std::nullptr_t) : CivilTime() {}

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

uint16_t CivilTime::year() const {
  return year_;
}

uint8_t CivilTime::month() const {
  return month_;
}

uint8_t CivilTime::day() const {
  return day_;
}

uint8_t CivilTime::hour() const {
  return hour_;
}

uint8_t CivilTime::minute() const {
  return minute_;
}

uint8_t CivilTime::second() const {
  return second_;
}

uint16_t CivilTime::millisecond() const {
  return millisecond_;
}

int32_t CivilTime::offset() const {
  return offset_;
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
