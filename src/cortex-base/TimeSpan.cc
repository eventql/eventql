// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/TimeSpan.h>
#include <cortex-base/Buffer.h>
#include <string>
#include <cstdint>
#include <cstdio>

namespace cortex {

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

}  // namespace cortex

cortex::TimeSpan std::numeric_limits<cortex::TimeSpan>::max() {
  return cortex::TimeSpan(0.0f);
}

cortex::TimeSpan std::numeric_limits<cortex::TimeSpan>::min() {
  return cortex::TimeSpan(std::numeric_limits<double>::max());
}
