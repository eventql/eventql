/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _libstx_UTIL_LOGGER_H
#define _libstx_UTIL_LOGGER_H
#include <atomic>
#include "stx/UnixTime.h"
#include "stx/stdtypes.h"
#include "stx/logging/loglevel.h"
#include "stx/logging/logtarget.h"

#ifndef STX_LOGGER_MAX_LISTENERS
#define STX_LOGGER_MAX_LISTENERS 64
#endif

namespace stx {

class Logger {
public:
  Logger();
  static Logger* get();

  void log(
      LogLevel log_level,
      const String& component,
      const String& message);

  template <typename... T>
  void log(
      LogLevel log_level,
      const String& component,
      const String& message,
      T... args);

  void logException(
      LogLevel log_level,
      const String& component,
      const std::exception& exception,
      const String& message);

  template <typename... T>
  void logException(
      LogLevel log_level,
      const String& component,
      const std::exception& exception,
      const String& message,
      T... args);

  void addTarget(LogTarget* target);
  void setMinimumLogLevel(LogLevel min_level);

protected:
  std::atomic<LogLevel> min_level_;
  std::atomic<size_t> max_listener_index_;
  std::atomic<LogTarget*> listeners_[STX_LOGGER_MAX_LISTENERS];
};

} // namespace stx

#include "stx/logging/logger_impl.h"
#endif
