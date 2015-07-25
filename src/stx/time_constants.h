/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_TIME_CONSTANTS_H
#define _STX_TIME_CONSTANTS_H
#include <ctime>
#include <inttypes.h>
#include <limits>
#include <string>

namespace stx {

constexpr const uint64_t kMicrosPerSecond = 1000000;
constexpr const uint64_t kMillisPerSecond = 1000;
constexpr const uint64_t kSecondsPerMinute = 60;
constexpr const uint64_t kMiillisPerMinute = kSecondsPerMinute * kMillisPerSecond;
constexpr const uint64_t kMicrosPerMinute = kSecondsPerMinute * kMicrosPerSecond;
constexpr const uint64_t kMinutesPerHour = 60;
constexpr const uint64_t kSecondsPerHour = kSecondsPerMinute * kMinutesPerHour;
constexpr const uint64_t kMillisPerHour = kSecondsPerHour * kMillisPerSecond;
constexpr const uint64_t kMicrosPerHour = kSecondsPerHour * kMicrosPerSecond;
constexpr const uint64_t kHoursPerDay = 24;
constexpr const uint64_t kSecondsPerDay = kSecondsPerHour * kHoursPerDay;
constexpr const uint64_t kMillisPerDay = kSecondsPerDay * kMillisPerSecond;
constexpr const uint64_t kMicrosPerDay = kSecondsPerDay * kMicrosPerSecond;
constexpr const uint64_t kDaysPerWeek = 7;
constexpr const uint64_t kSecondsPerWeek = kSecondsPerDay * kDaysPerWeek;
constexpr const uint64_t kMillisPerWeek = kSecondsPerWeek * kMicrosPerSecond;
constexpr const uint64_t kMicrosPerWeek = kSecondsPerWeek * kMicrosPerSecond;
constexpr const uint64_t kDaysPerYear = 365;
constexpr const uint64_t kSecondsPerYear = kDaysPerYear * kSecondsPerDay;
constexpr const uint64_t kMillisPerYear = kDaysPerYear * kMillisPerDay;
constexpr const uint64_t kMicrosPerYear = kDaysPerYear * kMicrosPerDay;

} // namespace stx
#endif
