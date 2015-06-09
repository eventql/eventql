/**
 * This file is part of the "sensord" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Finn Zirngibl
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/wallclock.h>
#include "Sampler.h"

using namespace fnord;

namespace sensord {

Sampler::Sampler(
    SamplerConfig config,
    SensorRepository* sensors) :
    config_(config),
    sensors_(sensors),
    running_(true) {}

void Sampler::run() {
  auto now = WallClock::unixMicros();

  for (auto& rule : *config_.mutable_rules()) {
    queue_.insert(&rule, now);
  }

  while (running_) {
    auto job = queue_.interruptiblePop();
    if (job.isEmpty()) {
      continue;
    }

    executeRule(job.get());
  }
}

void Sampler::executeRule(SampleRule* rule) {
  fnord::iputs("execute rule... $0", rule);

  auto now = WallClock::unixMicros();
  auto next = now + rule->sample_interval();
  queue_.insert(rule, next);
}

};

