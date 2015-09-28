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
#include <stx/defines.h>

namespace stx {

inline constexpr Duration::Duration(ZeroType)
    : micros_(0) {}

inline constexpr Duration::Duration(uint64_t microseconds)
    : micros_(microseconds) {}

inline Duration::Duration(const struct ::timeval& value)
    : micros_(value.tv_sec + value.tv_usec * kMicrosPerSecond) {}

inline Duration::Duration(const struct ::timespec& value)
    : micros_(value.tv_sec + value.tv_nsec * kMicrosPerSecond / 1000) {}

constexpr bool Duration::operator==(const Duration& other) const {
  return micros_ == other.micros_;
}

constexpr bool Duration::operator!=(const Duration& other) const {
  return micros_ != other.micros_;
}

constexpr bool Duration::operator<(const Duration& other) const {
  return micros_ < other.micros_;
}

constexpr bool Duration::operator>(const Duration& other) const {
  return micros_ > other.micros_;
}

constexpr bool Duration::operator<=(const Duration& other) const {
  return micros_ <= other.micros_;
}

constexpr bool Duration::operator>=(const Duration& other) const {
  return micros_ >= other.micros_;
}

constexpr bool Duration::operator!() const {
  return micros_ == 0;
}

inline constexpr Duration::operator struct timeval() const {
#if defined(STX_OS_DARWIN)
  // OS/X plays in it's own universe. ;(
  return { static_cast<time_t>(micros_ / kMicrosPerSecond),
           static_cast<__darwin_suseconds_t>(micros_ % kMicrosPerSecond) };
#else
  return { static_cast<time_t>(micros_ / kMicrosPerSecond),
           static_cast<long>(micros_ % kMicrosPerSecond) };
#endif
}

inline constexpr Duration::operator struct timespec() const {
#if defined(STX_OS_DARWIN)
  // OS/X plays in it's own universe. ;(
  return { static_cast<time_t>(micros_ / kMicrosPerSecond),
           (static_cast<__darwin_suseconds_t>(micros_ % kMicrosPerSecond) * 1000) };
#else
  return { static_cast<time_t>(micros_ / kMicrosPerSecond),
           (static_cast<long>(micros_ % kMicrosPerSecond) * 1000) };
#endif
}

inline constexpr uint64_t Duration::microseconds() const noexcept {
  return micros_;
}

inline constexpr uint64_t Duration::milliseconds() const noexcept {
  return micros_ / kMillisPerSecond;
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

Duration Duration::fromDays(uint64_t v) {
  return Duration(v * kMicrosPerSecond * kSecondsPerDay);
}

Duration Duration::fromHours(uint64_t v) {
  return Duration(v * kMicrosPerSecond * kSecondsPerHour);
}

Duration Duration::fromMinutes(uint64_t v) {
  return Duration(v * kMicrosPerSecond * kSecondsPerMinute);
}

Duration Duration::fromSeconds(uint64_t v) {
  return Duration(v * kMicrosPerSecond);
}

Duration Duration::fromMilliseconds(uint64_t v) {
  return Duration(v * 1000);
}

Duration Duration::fromMicroseconds(uint64_t v) {
  return Duration(v);
}

Duration Duration::fromNanoseconds(uint64_t v) {
  return Duration(v / 1000);
}

} // namespace stx
