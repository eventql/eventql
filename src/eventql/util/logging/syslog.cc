/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "eventql/util/logging/syslog.h"
#include "eventql/util/sysconfig.h"
#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

namespace stx {

SyslogTarget::SyslogTarget(const String& name)  {
#ifdef HAVE_SYSLOG_H
  setlogmask(LOG_UPTO (LOG_DEBUG));


  ident_ = mkScoped((char*) malloc(name.length() + 1));
  if (ident_.get()) {
    memcpy(ident_.get(), name.c_str(), name.length() + 1);
    openlog(ident_.get(), LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
  } else {
    openlog(NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
  }
#else
  RAISE(kRuntimeError, "compiled without syslog support");
#endif
}

SyslogTarget::~SyslogTarget()  {
#ifdef HAVE_SYSLOG_H
  closelog();
#endif
}

void SyslogTarget::log(
    LogLevel level,
    const String& component,
    const String& message) {
#ifdef HAVE_SYSLOG_H
  switch (level) {
    case LogLevel::kEmergency:
      syslog(LOG_EMERG, "[%s] %s", component.c_str(), message.c_str());
      return;

    case LogLevel::kAlert:
      syslog(LOG_ALERT, "[%s] %s", component.c_str(), message.c_str());
      return;

    case LogLevel::kCritical:
      syslog(LOG_CRIT, "[%s] %s", component.c_str(), message.c_str());
      return;

    case LogLevel::kError:
      syslog(LOG_ERR, "[%s] %s", component.c_str(), message.c_str());
      return;

    case LogLevel::kWarning:
      syslog(LOG_WARNING, "[%s] %s", component.c_str(), message.c_str());
      return;

    case LogLevel::kNotice:
      syslog(LOG_NOTICE, "[%s] %s", component.c_str(), message.c_str());
      return;

    case LogLevel::kInfo:
      syslog(LOG_INFO, "[%s] %s", component.c_str(), message.c_str());
      return;

    case LogLevel::kDebug:
    case LogLevel::kTrace:
      syslog(LOG_DEBUG, "[%s] %s", component.c_str(), message.c_str());
      return;

  }
#endif
}


} // namespace stx
