/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Christian Parpart
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/duration.h>
#include <stx/stringutil.h>
#include <sstream>

namespace stx {

std::string inspect(const Duration& value) {
  unsigned years = value.days() / kDaysPerYear;
  unsigned days = value.days() % kDaysPerYear;;
  unsigned hours = value.hours() % kHoursPerDay;
  unsigned minutes = value.minutes() % kMinutesPerHour;
  unsigned seconds = value.seconds() % kSecondsPerMinute;
  unsigned msecs = value.milliseconds () % kMillisPerSecond;

  std::stringstream sstr;
  int i = 0;

  if (years)
    sstr << years << " years";

  if (days) {
    if (i++) sstr << ' ';
    sstr << days << " days";
  }

  if (hours) {
    if (i++) sstr << ' ';
    sstr << hours << " hours";
  }

  if (minutes) {
    if (i++) sstr << ' ';
    sstr << minutes << " minutes";
  }

  if (seconds) {
    if (i++) sstr << ' ';
    sstr << seconds << " seconds";
  }

  if (msecs) {
    if (i) sstr << ' ';
    sstr << msecs << "ms";
  }

  return sstr.str();
}

template<>
std::string StringUtil::toString<Duration>(Duration duration) {
  return inspect(duration);
}

template<>
std::string StringUtil::toString<const Duration&>(const Duration& duration) {
  return inspect(duration);
}

}
