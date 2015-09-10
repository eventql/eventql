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

namespace stx {

inline constexpr Duration::Duration(uint64_t microseconds)
    : micros_(microseconds) {}

inline constexpr uint64_t Duration::microseconds() const noexcept {
  return micros_;
}

inline constexpr uint64_t Duration::seconds() const noexcept {
  return micros_ / kMicrosPerSecond;
}

inline constexpr Duration Duration::operator+(const Duration& other) const {
  return Duration(micros_ + other.micros_);
}

inline constexpr uint64_t Duration::minutes() const noexcept {
  return seconds() / kSecondsPerMinute;
}

inline constexpr uint64_t Duration::hours() const noexcept {
  return minutes() / kMinutesPerHour;
}

inline constexpr uint64_t Duration::days() const noexcept {
  return hours() / kHoursPerDay;
}

inline constexpr Duration Duration::fromDays(uint64_t v) {
  return Duration(v * kMicrosPerSecond * kSecondsPerDay);
}

inline constexpr Duration Duration::fromHours(uint64_t v) {
  return Duration(v * kMicrosPerSecond * kSecondsPerHour);
}

inline constexpr Duration Duration::fromMinutes(uint64_t v) {
  return Duration(v * kMicrosPerSecond * kSecondsPerMinute);
}

inline constexpr Duration Duration::fromSeconds(uint64_t v) {
  return Duration(v * kMicrosPerSecond);
}

inline constexpr Duration Duration::fromMilliseconds(uint64_t v) {
  return Duration(v * 1000);
}

inline constexpr Duration Duration::fromMicroseconds(uint64_t v) {
  return Duration(v);
}

inline constexpr Duration Duration::fromNanoseconds(uint64_t v) {
  return Duration(v / 1000);
}

} // namespace stx
