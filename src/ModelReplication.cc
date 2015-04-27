/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "unistd.h"
#include <fnord-base/wallclock.h>
#include <fnord-base/logging.h>
#include "ModelReplication.h"

using namespace fnord;

namespace cm {

ModelReplication::ModelReplication() :
    interval_(kMicrosPerSecond),
    running_(true) {}

void ModelReplication::addJob(const String& name, Function<void()> fn) {
  jobs_.emplace_back(name, fn);
}

void ModelReplication::start() {
  running_ = true;
  thread_ = std::thread(std::bind(&ModelReplication::run, this));
}

void ModelReplication::stop() {
  if (!running_) {
    return;
  }

  running_ = false;
  thread_.join();
}

void ModelReplication::run() {
  while (running_) {
    auto begin = WallClock::unixMicros();

    for (const auto& job : jobs_) {
      try {
        job.second();
      } catch (const Exception& e) {
        fnord::logError(
            "cm.chunkserver",
            e,
            "ModelReplication error for model '$0'",
            job.first);
      }
    }

    auto elapsed = WallClock::unixMicros() - begin;
    if (elapsed < interval_.microseconds()) {
      usleep(interval_.microseconds() - elapsed);
    }
  }
}


} // namespace cm

