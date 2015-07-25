/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/CivilTime.h"

namespace stx {

CivilTime::CivilTime(
    std::nullptr_t) :
    year_(0),
    month_(0),
    day_(0),
    hour_(0),
    minute_(0),
    second_(0),
    millisecond_(0),
    offset_(0) {}

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
