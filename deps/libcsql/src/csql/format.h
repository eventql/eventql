/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <string.h>
#include <csql/svalue.h>

namespace csql {

std::string numberToHuman(double value);
std::string svalueToHuman(const SValue& value);
std::string formatTime(SValue::TimeType time, const char* fmt = nullptr);
std::string formatTimeWithRange(SValue::TimeType time, int range);

// FIXPAUL clean up...
template <typename T>
std::string toHuman(T value) {
  SValue sval(value);
  return sval.getString();
}

}
