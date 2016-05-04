/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _libstx_UTIL_LOGOUTPUTSTREAM_H
#define _libstx_UTIL_LOGOUTPUTSTREAM_H

#include "stx/io/outputstream.h"
#include "stx/logging/loglevel.h"
#include "stx/logging/logtarget.h"
#include "stx/stdtypes.h"

namespace stx {

class LogOutputStream : public LogTarget {
public:
  LogOutputStream(std::unique_ptr<OutputStream> target);

  void log(
      LogLevel level,
      const String& component,
      const String& message) override;

protected:
  ScopedPtr<OutputStream> target_;
};

}
#endif
