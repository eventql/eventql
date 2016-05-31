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
#include "eventql/util/logging/syslog.h"
#include "eventql/util/logging.h"
#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

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
  syslog(LOG_NOTICE, "[%s] %s", logLevelToStr(level), message.c_str());
#endif
}
