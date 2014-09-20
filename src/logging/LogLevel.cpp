#include <xzero/logging/LogLevel.h>
#include <stdexcept>
#include <string>

namespace xzero {

std::string to_string(LogLevel value) {
  switch (value) {
    case LogLevel::None:
      return "none";
    case LogLevel::Error:
      return "error";
    case LogLevel::Warning:
      return "warning";
    case LogLevel::Info:
      return "info";
    case LogLevel::Debug:
      return "debug";
    case LogLevel::Trace:
      return "trace";
    default:
      throw std::runtime_error("Invalid State. Unknown LogLevel.");
  }
}

LogLevel to_loglevel(const std::string& value) {
  if (value == "none")
    return LogLevel::None;

  if (value == "error")
    return LogLevel::Error;

  if (value == "warning")
    return LogLevel::Warning;

  if (value == "info")
    return LogLevel::Info;

  if (value == "debug")
    return LogLevel::Debug;

  if (value == "trace")
    return LogLevel::Trace;

  throw std::runtime_error("Invalid State. Unknown LogLevel.");
}

} // namespace xzero
