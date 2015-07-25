// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#ifndef sw_x0_datetime_hpp
#define sw_x0_datetime_hpp 1

#include <cortex-base/Api.h>
#include <cortex-base/Buffer.h>
#include <cortex-base/TimeSpan.h>
#include <limits>
#include <string>
#include <ctime>
#include <pthread.h>

namespace cortex {

//! \addtogroup base
//@{

/**
 *
 * @brief Date/Time object.
 *
 * ... that understands unix timestamps as well as HTTP conform dates
 * as used in Date/Last-Modified and other headers.
 */
class CORTEX_API DateTime {
 private:
  double value_;
  mutable Buffer http_;
  mutable Buffer htlog_;

  static time_t mktime(const char* v);

 public:
  /**
   * Initializes the date/time object with the current timestamp.
   */
  DateTime();

  /** Initializes this object with an HTTP conform input date-time. */
  explicit DateTime(const BufferRef& http_v);

  /** Initializes this object with an HTTP conform input date-time. */
  explicit DateTime(const std::string& http_v);

  /** Initializes this object with the given timestamp. */
  explicit DateTime(double v);

  ~DateTime();

  /** Retrieves the timestamp as type @c double, as used by libev. */
  double value() const;

  /** Retrieves the UNIX timestamp as @c time_t. */
  std::time_t unixtime() const;

  /** Retrieves the UNIX timestamp in total milliseconds. */
  uint64_t unixMilliseconds() const;

  /** Retrieves the UNIX timestamp in total microseconds. */
  uint64_t unixMicroseconds() const;

  /**
   * @brief  Retrieve this dateime object as a HTTP/1.1 conform string.
   *
   * @return HTTP/1.1 conform string value.
   */
  const Buffer& http_str() const;
  const Buffer& htlog_str() const;
  std::string to_s() const;

  std::string format(const char* fmt = "%Y-%m-%d %H:%M:%S") const;

  void update();
  void update(double vvalue);
  DateTime& operator=(double value);
  DateTime& operator=(const DateTime& value);

  bool valid() const;

  static int compare(const DateTime& a, const DateTime& b);

  /**
   * Return a new DateTime instance with time 00:00:00 UTC, 1 January 1970.
   */
  static DateTime epoch();
};

CORTEX_API bool operator==(const DateTime& a, const DateTime& b);
CORTEX_API bool operator!=(const DateTime& a, const DateTime& b);
CORTEX_API bool operator<=(const DateTime& a, const DateTime& b);
CORTEX_API bool operator>=(const DateTime& a, const DateTime& b);
CORTEX_API bool operator<(const DateTime& a, const DateTime& b);
CORTEX_API bool operator>(const DateTime& a, const DateTime& b);

CORTEX_API TimeSpan operator-(const DateTime& a, const DateTime& b);
CORTEX_API DateTime operator+(const DateTime& a, const TimeSpan& b);
CORTEX_API DateTime operator-(const DateTime& a, const TimeSpan& b);

// {{{ impl
inline time_t DateTime::mktime(const char* v) {
  struct tm tm;
  tm.tm_isdst = 0;

  if (strptime(v, "%a, %d %b %Y %H:%M:%S GMT", &tm))
    return ::mktime(&tm) - timezone;

  return 0;
}

inline double DateTime::value() const {
  return value_;
}

inline std::time_t DateTime::unixtime() const {
  return static_cast<std::time_t>(value_);
}

inline uint64_t DateTime::unixMilliseconds() const {
  return static_cast<uint64_t>(value_ * 1000llu);
}

inline uint64_t DateTime::unixMicroseconds() const {
  return static_cast<uint64_t>(value_ * 1000000llu);
}

inline void DateTime::update() {
  update(static_cast<double>(std::time(nullptr)));
}

inline void DateTime::update(double v) {
  if (value_ != v) {
    value_ = v;
    http_.clear();
    htlog_.clear();
  }
}

inline DateTime& DateTime::operator=(double value) {
  update(value);
  return *this;
}

inline DateTime& DateTime::operator=(const DateTime& dt) {
  update(dt.value());
  return *this;
}

inline bool DateTime::valid() const { return value_ != 0; }

inline int DateTime::compare(const DateTime& a, const DateTime& b) {
  return a.unixtime() - b.unixtime();
}

inline TimeSpan operator-(const DateTime& a, const DateTime& b) {
  double diff = a.value() - b.value();

  if (diff < 0) diff = -diff;

  return TimeSpan(diff);
}

inline DateTime operator+(const DateTime& a, const TimeSpan& b) {
  return DateTime(a.value() + b.value());
}

inline DateTime operator-(const DateTime& a, const TimeSpan& b) {
  return DateTime(a.value() - b.value());
}
// }}}

// {{{ compare operators
inline bool operator==(const DateTime& a, const DateTime& b) {
  return DateTime::compare(a, b) == 0;
}

inline bool operator!=(const DateTime& a, const DateTime& b) {
  return DateTime::compare(a, b) != 0;
}

inline bool operator<=(const DateTime& a, const DateTime& b) {
  return DateTime::compare(a, b) <= 0;
}

inline bool operator>=(const DateTime& a, const DateTime& b) {
  return DateTime::compare(a, b) >= 0;
}

inline bool operator<(const DateTime& a, const DateTime& b) {
  return DateTime::compare(a, b) < 0;
}

inline bool operator>(const DateTime& a, const DateTime& b) {
  return DateTime::compare(a, b) > 0;
}
// }}}

//@}

}  // namespace cortex

namespace std {
template <> class numeric_limits<cortex::DateTime> {
public:
  static CORTEX_API cortex::DateTime max();
  static CORTEX_API cortex::DateTime min();
};
}

#endif
