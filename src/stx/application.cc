/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/application.h"
#include "stx/logging.h"
#include "stx/logging/logoutputstream.h"
#include "stx/exceptionhandler.h"
#include "stx/thread/signalhandler.h"

namespace stx {

void Application::init() {
  stx::thread::SignalHandler::ignoreSIGHUP();
  stx::thread::SignalHandler::ignoreSIGPIPE();

  auto ehandler = new stx::CatchAndAbortExceptionHandler();
  ehandler->installGlobalHandlers();
}

void Application::logToStderr(LogLevel min_log_level /* = LogLevel::kInfo */) {
  auto logger = new LogOutputStream(OutputStream::getStderr());
  Logger::get()->setMinimumLogLevel(min_log_level);
  Logger::get()->addTarget(logger);
}

}
