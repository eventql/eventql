// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/DateTime.h>
#include <pthread.h>

namespace xzero {

DateTime::DateTime() : value_(std::time(0)), http_(), htlog_() {}

DateTime::DateTime(const BufferRef& v)
    : value_(mktime(v.data())), http_(v), htlog_() {}

DateTime::DateTime(const std::string& v)
    : value_(mktime(v.c_str())), http_(v), htlog_(v) {}

DateTime::DateTime(ev::tstamp v) : value_(v), http_(), htlog_() {}

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

}  // namespace xzero
