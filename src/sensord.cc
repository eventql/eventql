/**
 * This file is part of the "sensord" project
 *   Copyright (c) 2015 Finn Zirngibl
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <unistd.h>
#include "fnord-base/application.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-base/inspect.h"
#include "fnord-base/exception.h"
#include "SensorRepository.h"
#include "HostStatsSensor.h"

using namespace sensord;
using namespace fnord;

int main(int argc, const char** argv) {
  Application::init();
  Application::logToStderr();

  cli::FlagParser flags;

  flags.defineFlag(
      "loglevel",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  SensorRepository sensors;
  sensors.addSensor(new HostStatsSensor());

  iputs(
      "host sensor: $0",
      sensors.fetchSensorDataAs<HostStats>("host").DebugString());

  return 0;
}
