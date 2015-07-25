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
#include <cortex-base/logging/LogAggregator.h>
#include <cortex-base/logging/LogLevel.h>
#include <cortex-base/logging/LogSource.h>
#include <string>

namespace cortex {

template<typename... Args>
CORTEX_API void logError(const char* component, const char* fmt, Args... args) {
  LogAggregator::get().getSource(component)->error(fmt, args...);
}

template<typename... Args>
CORTEX_API void logWarning(const char* component, const char* fmt, Args... args) {
  LogAggregator::get().getSource(component)->warn(fmt, args...);
}

template<typename... Args>
CORTEX_API void logNotice(const char* component, const char* fmt, Args... args) {
  LogAggregator::get().getSource(component)->notice(fmt, args...);
}

template<typename... Args>
CORTEX_API void logInfo(const char* component, const char* fmt, Args... args) {
  LogAggregator::get().getSource(component)->info(fmt, args...);
}

template<typename... Args>
CORTEX_API void logDebug(const char* component, const char* fmt, Args... args) {
  LogAggregator::get().getSource(component)->debug(fmt, args...);
}

template<typename... Args>
CORTEX_API void logTrace(const char* component, const char* fmt, Args... args) {
  LogAggregator::get().getSource(component)->trace(fmt, args...);
}

CORTEX_API void logError(const char* component, const std::exception& e);

}  // namespace cortex
