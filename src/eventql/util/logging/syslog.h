/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include "eventql/util/logging/logtarget.h"

namespace stx {

class SyslogTarget : public LogTarget {
public:

  SyslogTarget(const String& name);
  ~SyslogTarget();

  void log(
      LogLevel level,
      const String& component,
      const String& message) override;

protected:
  ScopedPtr<char> ident_;
};


} // namespace stx
