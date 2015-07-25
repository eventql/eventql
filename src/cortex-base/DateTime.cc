// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/DateTime.h>
#include <cortex-base/RuntimeError.h>
#include <pthread.h>

namespace cortex {

DateTime::DateTime() : value_(std::time(0)), http_(), htlog_() {}

DateTime::DateTime(const BufferRef& v)
    : value_(mktime(v.data())), http_(v), htlog_() {}

DateTime::DateTime(const std::string& v)
    : value_(mktime(v.c_str())), http_(v), htlog_(v) {}

DateTime::DateTime(double v) : value_(v), http_(), htlog_() {}

DateTime::~DateTime() {}

const Buffer& DateTime::http_str() const {
  if (http_.empty()) {
    std::time_t ts = unixtime();
    if (struct tm* tm = gmtime(&ts)) {
      char buf[256];

      if (strftime(buf, sizeof(buf), "%a, %d %b %Y %T GMT", tm) != 0) {
        http_ = buf;
      }
    }
  }

  return http_;
}

const Buffer& DateTime::htlog_str() const {
  if (htlog_.empty()) {
    std::time_t ts = unixtime();
    if (struct tm* tm = localtime(&ts)) {
      char buf[256];

      if (strftime(buf, sizeof(buf), "%d/%b/%Y:%T %z", tm) != 0) {
        htlog_ = buf;
      } else {
        htlog_ = "-";
      }
    } else {
      htlog_ = "-";
    }
  }

  return htlog_;
}

std::string DateTime::to_s() const {
  std::time_t ts = unixtime();
  struct tm* tm = gmtime(&ts);
  if (!tm)
    throw 0;

  char buf[256];
  if (strftime(buf, sizeof(buf), "%F %T GMT", tm) <= 0)
    throw 0;

  return buf;
}

std::string DateTime::format(const char* fmt) const {
  std::time_t ts = unixtime();
  struct tm* tm = gmtime(&ts);
  if (!tm)
    RAISE(RuntimeError, "DateTime.format: gmtime() failed");

  char buf[256];
  size_t n = strftime(buf, sizeof(buf), fmt, tm);
  if (n == 0)
    RAISE(RuntimeError, "DateTime.format: strftime() failed");

  return buf;
}

DateTime DateTime::epoch() {
  return DateTime(static_cast<double>(0.0));
}

}  // namespace cortex

cortex::DateTime std::numeric_limits<cortex::DateTime>::max() {
  return cortex::DateTime(0.0f);
}

cortex::DateTime std::numeric_limits<cortex::DateTime>::min() {
  return cortex::DateTime(std::numeric_limits<double>::max());
}
