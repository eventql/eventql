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
#include <cortex-base/sysconfig.h>
#include <string>

namespace std {
  class exception;
}

namespace cortex {

/**
 * A logging source.
 *
 * Creates a log message such as "[$ClassName] $LogMessage"
 *
 * Your class that needs logging creates a static member of LogSource
 * for generating logging messages.
 */
class CORTEX_API LogSource {
 public:
  explicit LogSource(const std::string& component);
  ~LogSource();

  void trace(const char* fmt, ...);
  void debug(const char* fmt, ...);
  void info(const char* fmt, ...);
  void warn(const char* fmt, ...);
  void notice(const char* fmt, ...);
  void error(const char* fmt, ...);
  void error(const std::exception& e);

  void enable();
  bool isEnabled() const CORTEX_NOEXCEPT;
  void disable();

  const std::string& componentName() const CORTEX_NOEXCEPT { return componentName_; }

 private:
  std::string componentName_;
  bool enabled_;
};

}  // namespace cortex
