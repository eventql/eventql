// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/Application.h>
#include <cortex-base/thread/SignalHandler.h>
#include <cortex-base/logging/LogAggregator.h>
#include <cortex-base/logging/LogTarget.h>
#include <cortex-base/logging.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/sysconfig.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>

namespace cortex {

void Application::init() {
  Application::installGlobalExceptionHandler();

  thread::SignalHandler::ignore(SIGPIPE);

  // well, when you detach from the terminal, you're garanteed to not get one.
  // unless someone sends it explicitely (so why ignoring then)?
  thread::SignalHandler::ignore(SIGHUP);
}

void Application::logToStderr(LogLevel loglevel) {
  LogAggregator::get().setLogTarget(LogTarget::console());
  LogAggregator::get().setLogLevel(loglevel);
}

static void globalEH() {
  try {
    throw;
  } catch (const std::exception& e) {
    logAndAbort(e);
  } catch (...) {
    // d'oh
    fprintf(stderr, "Unhandled foreign exception caught.\n");
    abort();
  }
}

void Application::installGlobalExceptionHandler() {
  std::set_terminate(&globalEH);
  std::set_unexpected(&globalEH);
}

std::string Application::userName() {
  if (struct passwd* pw = getpwuid(getuid()))
    return pw->pw_name;

  RAISE_ERRNO(errno);
}

std::string Application::groupName() {
  if (struct group* gr = getgrgid(getgid()))
    return gr->gr_name;

  RAISE_ERRNO(errno);
}

void Application::dropPrivileges(const std::string& username,
                                 const std::string& groupname) {
  if (username == Application::userName() && groupname == Application::groupName())
    return;

  logDebug("application", "dropping privileges to %s:%s",
      username.c_str(), groupname.c_str());

  if (!groupname.empty() && !getgid()) {
    if (struct group* gr = getgrnam(groupname.c_str())) {
      if (setgid(gr->gr_gid) != 0) {
        logError("application",
            "could not setgid to %s: %s", groupname.c_str(), strerror(errno));
        return;
      }

      setgroups(0, nullptr);

      if (!username.empty()) {
        initgroups(username.c_str(), gr->gr_gid);
      }
    } else {
      logError("application", "Could not find group: %s", groupname.c_str());
      return;
    }
    logTrace("application", "Dropped group privileges to '%s'.", groupname.c_str());
  }

  if (!username.empty() && !getuid()) {
    if (struct passwd* pw = getpwnam(username.c_str())) {
      if (setuid(pw->pw_uid) != 0) {
        logError("application", "could not setgid to %s: %s", username.c_str(),
            strerror(errno));
        return;
      }
      logInfo("application", "Dropped privileges to user %s", username.c_str());

      if (chdir(pw->pw_dir) < 0) {
        logError("application", "could not chdir to %s: %s", pw->pw_dir,
            strerror(errno));
        return;
      }
    } else {
      logError("application", "Could not find group: %s", groupname.c_str());
      return;
    }

    logTrace("application", "Dropped user privileges to '%s'.", username.c_str());
  }

  if (!::getuid() || !::geteuid() || !::getgid() || !::getegid()) {
#if defined(X0_RELEASE)
    logError("application",
        "Service is not allowed to run with administrative permissionsService "
        "is still running with administrative permissions.");
#else
    logWarning("application",
        "Service is still running with administrative permissions.");
#endif
  }
}

void Application::daemonize() {
  // XXX raises a warning on OS/X, but heck, how do you do it then on OS/X?
  if (::daemon(true /*no chdir*/, true /*no close*/) < 0) {
    RAISE_ERRNO(errno);
  }
}

} // namespace cortex
