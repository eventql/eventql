/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#ifndef _libstx_UTIL_LOGGER_H
#define _libstx_UTIL_LOGGER_H
#include <atomic>
#include "eventql/util/UnixTime.h"
#include "eventql/util/stdtypes.h"
#include "eventql/util/logging/loglevel.h"
#include "eventql/util/logging/logtarget.h"

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

#include "eventql/util/logging/logger_impl.h"
#endif
