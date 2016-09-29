/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <sstream>
#include <stdarg.h>
#include <eventql/util/exception.h>
#include <eventql/util/inspect.h>
#include <eventql/util/wallclock.h>
#include <eventql/util/logging.h>

const char* logLevelToStr(LogLevel log_level) {
  switch (log_level) {
    case LogLevel::kEmergency: return "EMERGENCY";
    case LogLevel::kAlert: return "ALERT";
    case LogLevel::kCritical: return "CRITICAL";
    case LogLevel::kError: return "ERROR";
    case LogLevel::kWarning: return "WARNING";
    case LogLevel::kNotice: return "NOTICE";
    case LogLevel::kInfo: return "INFO";
    case LogLevel::kDebug: return "DEBUG";
    case LogLevel::kTrace: return "TRACE";
    default: return "CUSTOM"; // FIXPAUL
  }
}

LogLevel strToLogLevel(const String& log_level) {
  if (log_level == "EMERGENCY") return LogLevel::kEmergency;
  if (log_level == "ALERT") return LogLevel::kAlert;
  if (log_level == "CRITICAL") return LogLevel::kCritical;
  if (log_level == "ERROR") return LogLevel::kError;
  if (log_level == "WARNING") return LogLevel::kWarning;
  if (log_level == "NOTICE") return LogLevel::kNotice;
  if (log_level == "INFO") return LogLevel::kInfo;
  if (log_level == "DEBUG") return LogLevel::kDebug;
  if (log_level == "TRACE") return LogLevel::kTrace;
  RAISEF(kIllegalArgumentError, "unknown log level: $0", log_level);
}

Logger* Logger::get() {
  static Logger singleton;
  return &singleton;
}

Logger::Logger() :
    min_level_(LogLevel::kNotice),
    max_listener_index_(0) {
  for (int i = 0; i < STX_LOGGER_MAX_LISTENERS; ++i) {
    listeners_[i] = nullptr;
  }
}

void Logger::logException(
    LogLevel log_level,
    const String& component,
    const std::exception& exception,
    const String& message) {
  if (log_level < min_level_) {
    return;
  }

  try {
    auto rte = dynamic_cast<const Exception&>(exception);
    log(
        log_level,
        component,
        "$0: $1: $2\n    in $3\n    in $4:$5",
        message,
        rte.getTypeName(),
        rte.getMessage(),
        rte.method(),
        rte.file(),
        rte.line());
  } catch (const std::exception& bcee) {
    log(
        log_level,
        component,
        "$0: std::exception: <foreign exception> $1",
        message,
        exception.what());
  }
}

void Logger::log(
      LogLevel log_level,
      const String& component,
      const String& message) {
  if (log_level < min_level_) {
    return;
  }

  const auto max_idx = max_listener_index_.load();
  for (int i = 0; i < max_idx; ++i) {
    auto listener = listeners_[i].load();

    if (listener != nullptr) {
      listener->log(log_level, component, message);
    }
  }
}

void Logger::addTarget(LogTarget* target) {
  auto listener_id = max_listener_index_.fetch_add(1);
  listeners_[listener_id] = target;
}

void Logger::setMinimumLogLevel(LogLevel min_level) {
  min_level_ = min_level;
}
