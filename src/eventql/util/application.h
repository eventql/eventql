/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Christian Parpart <trapni@dawanda.com>
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
#ifndef _STX_APPLICATION_H
#define _STX_APPLICATION_H
#include <functional>
#include <memory>
#include <mutex>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "eventql/util/logging.h"

// FIXME rename to procutil
class Application {
public:

  static void init();

  static void logToStderr(
      const String& program_name,
      LogLevel min_log_level = LogLevel::kInfo);

  static void logToSyslog(
      const String& name,
      LogLevel min_log_level = LogLevel::kInfo);

  /**
   * Retrieves the user-name this application is running under.
   */
  static std::string userName();

  /**
   * Retrieves the group-name this application is running under.
   */
  static std::string groupName();

  /**
   * Returns the current resident set size in bytes.
   */
  static size_t getCurrentMemoryUsage();

  /**
   * Returns the peak resident set size in bytes.
   */
  static size_t getPeakMemoryUsage();

  /**
   * Drops privileges to given @p user and @p group.
   *
   * Will only actually perform the drop if currently running as root
   * and the respective values are non-empty.
   */
  static void dropPrivileges(const std::string& user, const std::string& group);

  /**
   * Forks the application into background and become a daemon.
   */
  static void daemonize();

  /**
   * Set the current thread's name
   */
  static void setCurrentThreadName(const String& name);

};

#endif
