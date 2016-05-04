/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/logging.h"
#include "stx/logging/logoutputstream.h"
#include "stx/stringutil.h"
#include "stx/wallclock.h"

using stx::OutputStream;

namespace stx {

LogOutputStream::LogOutputStream(
    std::unique_ptr<OutputStream> target) :
    target_(std::move(target)) {}

void LogOutputStream::log(
    LogLevel level,
    const String& component,
    const String& message) {
  const auto prefix = StringUtil::format(
      "$0 $1 [$2] ",
      WallClock::now().toString("%Y-%m-%d %H:%M:%S"),
      logLevelToStr(level),
      component);

  std::string lines = prefix + message;
  StringUtil::replaceAll(&lines, "\n", "\n" + prefix);
  lines.append("\n");

  ScopedLock<std::mutex> lk(target_->mutex);
  target_->write(lines);
}

}
