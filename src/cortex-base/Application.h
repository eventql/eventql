// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/Api.h>
#include <cortex-base/logging/LogLevel.h>

namespace cortex {

class CORTEX_API Application {
 public:
  static void init();

  static void logToStderr(LogLevel loglevel = LogLevel::Info);

  static void installGlobalExceptionHandler();

  /**
   * Retrieves the user-name this application is running under.
   */
  static std::string userName();

  /**
   * Retrieves the group-name this application is running under.
   */
  static std::string groupName();

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

} // namespace cortex
