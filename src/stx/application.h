/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Christian Parpart
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
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
#include "stx/logging.h"

namespace stx {

class Application {
public:

  static void init();

  static void logToStderr(LogLevel min_log_level = LogLevel::kInfo);

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

};

}
#endif
