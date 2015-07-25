// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/Api.h>
#include <cortex-base/Buffer.h>
#include <cortex-base/DateTime.h>
#include <mutex>

namespace cortex {

class WallClock;

namespace http {

/**
 * API to generate an HTTP conform Date response header field value.
 */
class CORTEX_HTTP_API HttpDateGenerator {
 public:
  explicit HttpDateGenerator(WallClock* clock);

  WallClock* clock() const { return clock_; }
  void setClock(WallClock* clock) { clock_ = clock; }

  void update();
  void fill(Buffer* target);

 private:
  WallClock* clock_;
  DateTime current_;
  Buffer buffer_;
  std::mutex mutex_;
};

} // namespace http
} // namespace cortex
