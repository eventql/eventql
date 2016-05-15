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
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <pthread.h>
#include "eventql/util/application.h"
#include "eventql/util/logging.h"
#include "eventql/util/logging/logoutputstream.h"
#include "eventql/util/logging/syslog.h"
#include "eventql/util/exceptionhandler.h"
#include "eventql/util/thread/signalhandler.h"

namespace util {

void Application::init() {
  util::thread::SignalHandler::ignoreSIGHUP();
  util::thread::SignalHandler::ignoreSIGPIPE();

  auto ehandler = new util::CatchAndAbortExceptionHandler();
  ehandler->installGlobalHandlers();
}

void Application::logToStderr(LogLevel min_log_level /* = LogLevel::kInfo */) {
  auto logger = new LogOutputStream(OutputStream::getStderr());
  Logger::get()->setMinimumLogLevel(min_log_level);
  Logger::get()->addTarget(logger);
}

void Application::logToSyslog(
    const String& name,
    LogLevel min_log_level /* = LogLevel::kInfo */) {
  auto logger = new SyslogTarget(name);
  Logger::get()->addTarget(logger);
}

std::string Application::userName() {
  if (struct passwd* pw = getpwuid(getuid()))
    return pw->pw_name;

  RAISE_ERRNO("getpwuid() failed");
}

std::string Application::groupName() {
  if (struct group* gr = getgrgid(getgid()))
    return gr->gr_name;

  RAISE_ERRNO("getgrgid() failed");
}

/*
 * getCurrentMemoryUsage and getPeakMemoryUsage based on
 *   http://nadeausoftware.com/articles/2012/07/c_c_tip_how_get_process_resident_set_size_physical_memory_use
 *
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */
#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>
#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>
#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>
#endif
#endif

size_t Application::getCurrentMemoryUsage() {
#if defined(_WIN32)
  PROCESS_MEMORY_COUNTERS info;
  GetProcessMemoryInfo(GetCurrentProcess( ), &info, sizeof(info));
  return info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
  struct mach_task_basic_info info;
  mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
  auto ret = task_info(
        mach_task_self(),
        MACH_TASK_BASIC_INFO,
        (task_info_t) &info,
        &infoCount);

  if (ret != KERN_SUCCESS) {
    RAISE(kRuntimeError, "task_info() failed");
  }

  return info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
  size_t rss = 0;
  FILE* fp = NULL;
  if ((fp = fopen("/proc/self/statm", "r" )) == NULL) {
    RAISE_ERRNO(kRuntimeError, "can't open /proc/self/statm");
  }

  if (fscanf(fp, "%*s%ld", &rss) != 1) {
    fclose(fp);
    RAISE_ERRNO(kRuntimeError, "error reading /proc/self/statm");
  }

  fclose(fp);
  return (size_t) rss * (size_t) sysconf(_SC_PAGESIZE);
#else
#error Unsupported OS
#endif
}

size_t Application::getPeakMemoryUsage() {
#if defined(_WIN32)
  PROCESS_MEMORY_COUNTERS info;
  GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
  return info.PeakWorkingSetSize;

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
  struct psinfo psinfo;
  int fd = -1;
  if ((fd = open("/proc/self/psinfo", O_RDONLY )) == -1) {
    RAISE_ERRNO(kRuntimeError, "can't open /proc/self/psinfo");
  }

  if (read(fd, &psinfo, sizeof(psinfo)) != sizeof(psinfo)) {
    close(fd);
    RAISE_ERRNO(kRuntimeError, "error reading /proc/self/psinfo");
  }

  close(fd);
  return psinfo.pr_rssize * 1024L;

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
  struct rusage rusage;
  getrusage(RUSAGE_SELF, &rusage);
#if defined(__APPLE__) && defined(__MACH__)
  return rusage.ru_maxrss;
#else
  return rusage.ru_maxrss * 1024L;
#endif
#else
#error Unsupported OS
#endif
}

void Application::dropPrivileges(const std::string& username,
                                 const std::string& groupname) {
  if (username == Application::userName() && groupname == Application::groupName())
    return;

  logDebug("application", "dropping privileges to %s:%s",
      username.c_str(), groupname.c_str());

  if (!groupname.empty() && !getgid()) {
    struct group* gr = getgrnam(groupname.c_str());
    if (!gr) {
      logError("application", "Could not find group: %s", groupname.c_str());
      return;
    }

    if (setgid(gr->gr_gid) != 0) {
      logError("application",
          "could not setgid to %s: %s", groupname.c_str(), strerror(errno));
      return;
    }

    setgroups(0, nullptr);

    if (!username.empty()) {
      initgroups(username.c_str(), gr->gr_gid);
    }
    logTrace("application", "Dropped group privileges to '%s'.", groupname.c_str());
  }

  if (!username.empty() && !getuid()) {
    struct passwd* pw = getpwnam(username.c_str());
    if (!pw)
      RAISE(kRuntimeError, "Could not find group: %s", groupname.c_str());

    if (setuid(pw->pw_uid) != 0)
      RAISE(kRuntimeError, "Could not setgid to %s: %s",
            username.c_str(), strerror(errno));

    logInfo("application", "Dropped privileges to user %s", username.c_str());

    if (chdir(pw->pw_dir) < 0)
      RAISE(kRuntimeError, "could not chdir to %s: %s",
            pw->pw_dir, strerror(errno));

    logTrace("application", "Dropped user privileges to '%s'.", username.c_str());
  }
}

void Application::daemonize() {
#if defined(_WIN32)
#error "Application::daemonize() not yet implemented for windows"

#elif defined(__APPLE__) && defined(__MACH__)
  // FIXME: deprecated on OSX
  if (::daemon(true /*no chdir*/, true /*no close*/) < 0) {
    RAISE_ERRNO("daemon() failed");
  }

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
  if (::daemon(true /*no chdir*/, true /*no close*/) < 0) {
    RAISE_ERRNO("daemon() failed");
  }

#else
#error Unsupported OS
#endif
}

void Application::setCurrentThreadName(const String& name) {
#if defined(_WIN32)
#error "Application::setCurrentThreadName() not yet implemented for windows"

#elif defined(__APPLE__) && defined(__MACH__)
  pthread_setname_np(name.c_str());

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
  pthread_setname_np(pthread_self(), name.c_str());

#else
#error Unsupported OS
#endif
}

}
