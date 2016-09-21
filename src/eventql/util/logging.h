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
#ifndef _libstx_UTIL_LOGGING_H
#define _libstx_UTIL_LOGGING_H

#include "eventql/util/logging/loglevel.h"
#include "eventql/util/logging/logger.h"

/**
 * EMERGENCY: Something very bad happened
 */
template <typename... T>
void logEmergency(const String& component, const String& msg, T... args) {
  Logger::get()->log(LogLevel::kEmergency, component, msg, args...);
}

template <typename... T>
void logEmergency(
    const String& component,
    const std::exception& e,
    const String& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kEmergency, component, e, msg, args...);
}

/**
 * ALERT: Action must be taken immediately
 */
template <typename... T>
void logAlert(const String& component, const String& msg, T... args) {
  Logger::get()->log(LogLevel::kAlert, component, msg, args...);
}

template <typename... T>
void logAlert(
    const String& component,
    const std::exception& e,
    const String& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kAlert, component, e, msg, args...);
}

/**
 * CRITICAL: Action should be taken as soon as possible
 */
template <typename... T>
void logCritical(const String& component, const String& msg, T... args) {
  Logger::get()->log(LogLevel::kCritical, component, msg, args...);
}

template <typename... T>
void logCritical(
    const String& component,
    const std::exception& e,
    const String& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kCritical, component, e, msg, args...);
}

/**
 * ERROR: User-visible Runtime Errors
 */
template <typename... T>
void logError(const String& component, const String& msg, T... args) {
  Logger::get()->log(LogLevel::kError, component, msg, args...);
}

template <typename... T>
void logError(
    const String& component,
    const std::exception& e,
    const String& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kError, component, e, msg, args...);
}

/**
 * WARNING: Something unexpected happened that should not have happened
 */
template <typename... T>
void logWarning(const String& component, const String& msg, T... args) {
  Logger::get()->log(LogLevel::kWarning, component, msg, args...);
}

template <typename... T>
void logWarning(
    const String& component,
    const std::exception& e,
    const String& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kWarning, component, e, msg, args...);
}

/**
 * NOTICE: Normal but significant condition.
 */
template <typename... T>
void logNotice(const String& component, const String& msg, T... args) {
  Logger::get()->log(LogLevel::kNotice, component, msg, args...);
}

template <typename... T>
void logNotice(
    const String& component,
    const std::exception& e,
    const String& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kNotice, component, e, msg, args...);
}

/**
 * INFO: Informational messages
 */
template <typename... T>
void logInfo(const String& component, const String& msg, T... args) {
  Logger::get()->log(LogLevel::kInfo, component, msg, args...);
}

template <typename... T>
void logInfo(
    const String& component,
    const std::exception& e,
    const String& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kInfo, component, e, msg, args...);
}

/**
 * DEBUG: Debug messages
 */
template <typename... T>
void logDebug(const String& component, const String& msg, T... args) {
  Logger::get()->log(LogLevel::kDebug, component, msg, args...);
}

template <typename... T>
void logDebug(
    const String& component,
    const std::exception& e,
    const String& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kDebug, component, e, msg, args...);
}

/**
 * TRACE: Trace messages
 */
template <typename... T>
void logTrace(const String& component, const String& msg, T... args) {
  Logger::get()->log(LogLevel::kTrace, component, msg, args...);
}

template <typename... T>
void logTrace(
    const String& component,
    const std::exception& e,
    const String& msg,
    T... args) {
  Logger::get()->logException(LogLevel::kTrace, component, e, msg, args...);
}

/**
 * Return the human readable string representation of the provided log leval
 */
const char* logLevelToStr(LogLevel log_level);

/**
 * Return the log level from the human readable string representation. This
 * will raise an exception if no such log level is known
 */
LogLevel strToLogLevel(const String& log_level);

#endif
