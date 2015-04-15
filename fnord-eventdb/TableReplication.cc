/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "unistd.h"
#include <fnord-base/logging.h>
#include <fnord-base/wallclock.h>
#include <fnord-eventdb/TableReplication.h>

namespace fnord {
namespace eventdb {

TableReplication::TableReplication() :
    interval_(10 * kMicrosPerSecond),
    running_(true) {}

void TableReplication::start() {
  running_ = true;
  thread_ = std::thread(std::bind(&TableReplication::run, this));
}

void TableReplication::stop() {
  if (!running_) {
    return;
  }

  running_ = false;
  thread_.join();
}

void TableReplication::pullAll() {
  fnord::logDebug("fnord.evdb", "Running TableReplication...");

}

void TableReplication::run() {
  while (running_.load()) {
    auto begin = WallClock::unixMicros();

    try {
      pullAll();
    } catch (const Exception& e) {
      fnord::logError("fnord.evdb", e, "TableReplication error");
    }

    auto elapsed = WallClock::unixMicros() - begin;
    if (elapsed < interval_) {
      usleep(interval_ - elapsed);
    }
  }
}

} // namespace eventdb
} // namespace fnord

