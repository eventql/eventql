// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/support/libev/LibevClock.h>

namespace xzero {
namespace support {

LibevClock::LibevClock(ev::loop_ref loop) :
  loop_(loop) {
}

DateTime LibevClock::get() const {
  return DateTime(loop_.now());
}

} // namespace support
} // namespace xzero
