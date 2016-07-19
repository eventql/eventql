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
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/logging.h>
#include <eventql/util/application.h>
#include "eventql/eventql.h"
#include "eventql/db/garbage_collector.h"
#include <assert.h>

namespace eventql {

String garbageCollectorModeToString(GarbageCollectorMode mode) {
  switch (mode) {
    case GarbageCollectorMode::DISABLED: return "DISABLED";
    case GarbageCollectorMode::MANUAL: return "MANUAL";
    case GarbageCollectorMode::AUTOMATIC: return "AUTOMATIC";
  }
}

GarbageCollectorMode garbageCollectorModeFromString(const String& str) {
  auto str_upper = StringUtil::toUpper(str);

  if (str_upper == "DISABLED") {
    return GarbageCollectorMode::DISABLED;
  }

  if (str_upper == "MANUAL") {
    return GarbageCollectorMode::MANUAL;
  }

  if (str_upper == "AUTOMATIC") {
    return GarbageCollectorMode::AUTOMATIC;
  }

  RAISEF(kRuntimeError, "invalid garbage collector mode: $0", str);
}


GarbageCollector::GarbageCollector(
    GarbageCollectorMode mode,
    const String& base_dir,
    const String& trash_dir,
    const String& cache_dir,
    size_t gc_interval /* = kDefaultGCInterval */) :
    mode_(mode),
    base_dir_(base_dir),
    trash_dir_(trash_dir),
    cache_dir_(cache_dir),
    gc_interval_(kDefaultGCInterval) {}

GarbageCollector::~GarbageCollector() {
  assert(thread_running_ == false);
}

void GarbageCollector::runGC() {
  logDebug("evqld", "Running garbage collector...");
}

void GarbageCollector::startGCThread() {
  thread_running_ = true;

  thread_ = std::thread([this] {
    Application::setCurrentThreadName("evqld-gc");

    std::unique_lock<std::mutex> lk(mutex_);
    while (thread_running_) {
      lk.unlock();

      try {
        runGC();
      } catch (const std::exception& e) {
        logError("evqld", e, "Error in GarbgageCollection thread");
      }

      lk.lock();
      cv_.wait_for(
          lk,
          std::chrono::microseconds(gc_interval_),
          [this] () { return !thread_running_; });
    }
  });
}

void GarbageCollector::stopGCThread() {
  std::unique_lock<std::mutex> lk(mutex_);
  if (!thread_running_) {
    return;
  }

  thread_running_ = false;
  cv_.notify_all();
}

void GarbageCollector::emptyTrash() {

}

void GarbageCollector::freeCache() {

}

} // namespace eventql

