// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/logging/LogSource.h>
#include <cortex-base/logging/LogTarget.h>
#include <cortex-base/logging/LogAggregator.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/StackTrace.h>
#include <typeinfo>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdarg.h>

namespace cortex {

LogSource::LogSource(const std::string& componentName)
    : componentName_(componentName) {
}

LogSource::~LogSource() {
}

#define LOG_SOURCE_MSG(level, fmt) \
  if (static_cast<int>(LogLevel::level) <=                                     \
      static_cast<int>(LogAggregator::get().logLevel())) {                     \
    if (LogTarget* target = LogAggregator::get().logTarget()) {                \
      char msg[512];                                                           \
      int n = snprintf(msg, sizeof(msg), "[%s] ", componentName_.c_str());     \
      va_list va;                                                              \
      va_start(va, fmt);                                                       \
      vsnprintf(msg + n, sizeof(msg) - n, fmt, va);                            \
      va_end(va);                                                              \
      target->level(msg);                                                      \
    }                                                                          \
  }

void LogSource::trace(const char* fmt, ...) {
  LOG_SOURCE_MSG(trace, fmt);
}

void LogSource::error(const std::exception& e) {
  if (LogTarget* target = LogAggregator::get().logTarget()) {
    std::stringstream sstr;

    if (auto rt = dynamic_cast<const RuntimeError*>(&e)) {
      auto bt = rt->backtrace();

      sstr << "Exception of type " << typeid(*rt).name() << " caught from "
           << rt->sourceFile() << ":" << rt->sourceLine() << ". "
           << rt->what();

      for (size_t i = 0; i < bt.size(); ++i) {
        sstr << std::endl << "  [" << i << "] " << bt[i];
      }
    } else {
      sstr << "Exception of type "
           << StackTrace::demangleSymbol(typeid(e).name())
           << " caught. " << e.what();
    }

    target->error(sstr.str());
  }
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

void LogSource::notice(const char* fmt, ...) {
  LOG_SOURCE_MSG(notice, fmt);
}

void LogSource::error(const char* fmt, ...) {
  LOG_SOURCE_MSG(error, fmt);
}

void LogSource::enable() {
  enabled_ = true;
}

bool LogSource::isEnabled() const CORTEX_NOEXCEPT {
  return enabled_;
}

void LogSource::disable() {
  enabled_ = false;
}

}  // namespace cortex
