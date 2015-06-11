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
#include "fnord-base/logging.h"
#include "fnord-http/httpclient.h"
#include "SensorRepository.h"
#include "Sampler.h"
#include "HostStatsSensor.h"

using namespace sensord;
using namespace fnord;

int main(int argc, const char** argv) {
  Application::init();
  Application::logToStderr();

  cli::FlagParser flags;

  flags.defineFlag(
      "target-http",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "url",
      "<url>");

  flags.defineFlag(
      "namespace",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "",
      "namespace",
      "<namespace>");

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

  /* set up sensors */
  SensorRepository sensors;
  sensors.addSensor(new HostStatsSensor());

  /* set up sampler config */
  SamplerConfig config;
  {
    auto rule = config.add_rules();
    rule->set_sensor_key("host");
    rule->set_sample_interval(kMicrosPerSecond);
  }

  fnord::logInfo(
      "sensord",
      "Starting sensord; sensors=$0 rules=$1",
      sensors.numSensors(),
      config.rules().size());

  /* set up sampler */
  Sampler sampler(config, &sensors);

  /* set up http upload targets (--target-http) */
  http::HTTPClient http;
  auto sample_namespace = flags.getString("namespace");
  for (const auto& trgt : flags.getStrings("target-http")) {
    fnord::logInfo("sensord", "Uploading samples to '$0'", trgt);

    sampler.onSample([&sample_namespace, &http, trgt] (const SampleEnvelope& s) {
      fnord::logDebug("sensord", "Uploading sample to '$0'", trgt);

      URI target(trgt);
      SampleList list;

      auto sample = list.add_samples();
      *sample = s;
      sample->set_sample_namespace(sample_namespace);

      /* N.B. naive request per sample for now, batch/optimize later ~paul */
      http::HTTPRequest req(http::HTTPMessage::M_POST, target.pathAndQuery());
      req.addHeader("Host", target.hostAndPort());
      req.addBody(*msg::encode(list));

      auto res = http.executeRequest(req);
      if (res.statusCode() != 201) {
        RAISEF(kRuntimeError, "got non-201 response: $0", res.body().toString());
      }
    });
  }

  /* go! ;) */
  sampler.run();

  return 0;
}
