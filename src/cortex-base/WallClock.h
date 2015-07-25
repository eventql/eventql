// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <cortex-base/DateTime.h>

namespace cortex {

class TimeSpan;

/**
 * Abstract API for retrieving the current system time.
 */
class CORTEX_API WallClock {
 public:
  virtual ~WallClock() {}

  virtual DateTime get() const = 0;

  /**
   * Retrieves a global unique system clock using the standard C runtime.
   */
  static WallClock* system();

  /**
   * Retrieves a process-global monotonic clock that never jumps back in time.
   */
  static WallClock* monotonic();

  static void sleep(TimeSpan ts);
};

} // namespace cortex
