// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#ifndef sw_x0_TimeSpan_h
#define sw_x0_TimeSpan_h

#include <cortex-base/Api.h>
#include <cortex-base/Buffer.h>
#include <limits>
#include <cstdio>

namespace cortex {

/**
 * @brief High resolution time span.
 */
class CORTEX_API TimeSpan {
 public:
  static constexpr const uint64_t MicrosPerSecond = 1000000;
  static constexpr const uint64_t MillisPerSecond = 1000;
  static constexpr const uint64_t SecondsPerMinute = 60;
  static constexpr const uint64_t MinutesPerHour = 60;
  static constexpr const uint64_t SecondsPerHour = SecondsPerMinute * MinutesPerHour;
  static constexpr const uint64_t MillisPerHour = SecondsPerHour * MillisPerSecond;
  static constexpr const uint64_t MicrosPerHour = SecondsPerHour * MicrosPerSecond;
  static constexpr const uint64_t HoursPerDay = 24;
  static constexpr const uint64_t SecondsPerDay = SecondsPerHour * HoursPerDay;
  static constexpr const uint64_t MillisPerDay = SecondsPerDay * MillisPerSecond;
  static constexpr const uint64_t MicrosPerDay = SecondsPerDay * MicrosPerSecond;

 private:
  double value_;

 public:
  TimeSpan() : value_(0) {}
  TimeSpan(double v) : value_(v) {}
  TimeSpan(std::size_t v) : value_(v) {}
  TimeSpan(const TimeSpan& v) : value_(v.value_) {}

  double value() const { return value_; }
  double operator()() const { return value_; }

  int days() const { return ((int)value_ / ticksPerDay()); }
  int hours() const {
    return ((int)value_ / ticksPerHour()) - 24 * ((int)value_ / ticksPerDay());
  }
  int minutes() const { return ((int)value_ / 60) - 60 * ((int)value_ / 3600); }
  int seconds() const { return static_cast<int>(value_) % 60; }
  int milliseconds() const { return static_cast<int>(value_ * 1000) % 1000; }
  int microseconds() const { return static_cast<int>(value_ * 1000000) % 1000000; }
  long nanoseconds() const { return static_cast<long>(value_ * 1000000000) % 1000000000; }

  static inline int ticksPerDay() { return 86400; }
  static inline int ticksPerHour() { return 3600; }
  static inline int ticksPerMinute() { return 60; }
  static inline int ticksPerSecond() { return 1; }

  static TimeSpan fromDays(std::size_t v) {
    return TimeSpan(ticksPerDay() * v);
  }
  static TimeSpan fromHours(std::size_t v) {
    return TimeSpan(ticksPerHour() * v);
  }
  static TimeSpan fromMinutes(std::size_t v) {
    return TimeSpan(ticksPerMinute() * v);
  }
  static TimeSpan fromSeconds(std::size_t v) {
    return TimeSpan(ticksPerSecond() * v);
  }
  static TimeSpan fromMilliseconds(std::size_t v) {
    return TimeSpan(double(v / 1000 + (unsigned(v) % 1000) / 1000.0));
  }
  static TimeSpan fromMicroseconds(long v) {
    return TimeSpan(double(
        v / 1000000 + (uint64_t(v) % 1000000) / 1000000.0));
  }
  static TimeSpan fromNanoseconds(long v) {
    return TimeSpan(double(
        v / 1000000000 + (uint64_t(v) % 1000000000) / 1000000000.0));
  }

  std::size_t totalSeconds() const { return value_; }
  std::size_t totalMilliseconds() const { return value_ * 1000; }
  std::size_t totalMicroseconds() const { return value_ * 1000000; }
  unsigned long long totalNanoseconds() const { return value_ * 1000000000; }

  bool operator!() const { return value_ == 0; }
  operator bool() const { return value_ != 0; }

  std::string str() const;

  static const TimeSpan Zero;
};

inline bool operator==(const TimeSpan& a, const TimeSpan& b) {
  return a() == b();
}

inline bool operator!=(const TimeSpan& a, const TimeSpan& b) {
  return a() != b();
}

inline bool operator<(const TimeSpan& a, const TimeSpan& b) {
  return a() < b();
}

inline bool operator<=(const TimeSpan& a, const TimeSpan& b) {
  return a() <= b();
}

inline bool operator>(const TimeSpan& a, const TimeSpan& b) {
  return a() > b();
}

inline bool operator>=(const TimeSpan& a, const TimeSpan& b) {
  return a() >= b();
}

inline TimeSpan operator+(const TimeSpan& a, const TimeSpan& b) {
  return TimeSpan(a() + b());
}

inline TimeSpan operator-(const TimeSpan& a, const TimeSpan& b) {
  return TimeSpan(a() - b());
}

CORTEX_API Buffer& operator<<(Buffer& buf, const TimeSpan& ts);

}  // namespace cortex

namespace std {
template <> class numeric_limits<cortex::TimeSpan> {
public:
  static CORTEX_API cortex::TimeSpan max();
  static CORTEX_API cortex::TimeSpan min();
};
}

#endif
