#include <xzero/logging/LogSource.h>
#include <xzero/logging/LogTarget.h>
#include <xzero/logging/LogAggregator.h>
#include <stdio.h>
#include <stdarg.h>

namespace xzero {

LogSource::LogSource(const std::string& className)
    : className_(className) {
  LogAggregator::get().registerSource(this);
}

LogSource::~LogSource() {
  LogAggregator::get().unregisterSource(this);
}

#define LOG_SOURCE_MSG(level, fmt) {                                 \
  if (LogTarget* target = LogAggregator::get().logTarget()) {        \
    char msg[512];                                                   \
    int n = snprintf(msg, sizeof(msg), "[%s] ", className_.c_str()); \
    va_list va;                                                      \
    va_start(va, fmt);                                               \
    vsnprintf(msg + n, sizeof(msg) - n, fmt, va);                    \
    va_end(va);                                                      \
    target->level(msg);                                              \
  }                                                                  \
}

void LogSource::debug(const char* fmt, ...) {
  LOG_SOURCE_MSG(debug, fmt);
}

void LogSource::info(const char* fmt, ...) {
  LOG_SOURCE_MSG(info, fmt);
}

void LogSource::warn(const char* fmt, ...) {
  LOG_SOURCE_MSG(warn, fmt);
}

void LogSource::error(const char* fmt, ...) {
  LOG_SOURCE_MSG(error, fmt);
}

void LogSource::enable() {
  enabled_ = true;
}

bool LogSource::isEnabled() const noexcept {
  return enabled_;
}

void LogSource::disable() {
  enabled_ = false;
}

} // namespace xzero
