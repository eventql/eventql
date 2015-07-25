// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/logging/LogLevel.h>
#include <cortex-base/RuntimeError.h>
#include <stdexcept>
#include <string>

namespace cortex {

std::string to_string(LogLevel value) {
  switch (value) {
    case LogLevel::None:
      return "none";
    case LogLevel::Error:
      return "error";
    case LogLevel::Warning:
      return "warning";
    case LogLevel::Notice:
      return "notice";
    case LogLevel::Info:
      return "info";
    case LogLevel::Debug:
      return "debug";
    case LogLevel::Trace:
      return "trace";
    default:
      RAISE(IllegalStateError, "Invalid State. Unknown LogLevel.");
  }
}

LogLevel to_loglevel(const std::string& value) {
  if (value == "none")
    return LogLevel::None;

  if (value == "error")
    return LogLevel::Error;

  if (value == "warning")
    return LogLevel::Warning;

  if (value == "notice")
    return LogLevel::Notice;

  if (value == "info")
    return LogLevel::Info;

  if (value == "debug")
    return LogLevel::Debug;

  if (value == "trace")
    return LogLevel::Trace;

  RAISE(IllegalStateError, "Invalid State. Unknown LogLevel.");
}

} // namespace cortex
