/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _libstx_UTIL_LOGTARGET_H
#define _libstx_UTIL_LOGTARGET_H
#include "stx/logging/loglevel.h"

namespace stx {

class LogTarget {
public:
  virtual ~LogTarget() {}

  virtual void log(
      LogLevel level,
      const String& component,
      const String& message) = 0;

};

}

#endif
