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

#include <xzero/TimeSpan.h>
#include <xzero/Buffer.h>
#include <string>
#include <cstdint>
#include <cstdio>

namespace xzero {

const TimeSpan TimeSpan::Zero(static_cast<size_t>(0));

std::string TimeSpan::str() const {
  Buffer b(64);
  b << *this;
  return b.str();
}

Buffer& operator<<(Buffer& buf, const TimeSpan& ts) {
  bool green = false;

  // "5 days 5h 20m 33s"
  //        "5h 20m 33s"
  //           "20m 33s"
  //               "33s"

  if (ts.days()) {
    buf << ts.days() << "days";
    green = true;
  }

  if (green || ts.hours()) {
    if (green) buf << ' ';
    buf << ts.hours() << "h";
    green = true;
  }

  if (green || ts.minutes()) {
    if (green) buf << ' ';
    buf << ts.minutes() << "m";
    green = true;
  }

  if (green || ts.seconds()) {
    if (green) buf << ' ';
    buf << ts.seconds() << "s";
  }

  return buf;
}

}  // namespace xzero
