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
#include <stx/logging.h>
#include <stx/wallclock.h>
#include <fnord-logtable/TableJanitor.h>

namespace stx {
namespace logtable {

TableJanitor::TableJanitor(
    TableRepository* repo) :
    repo_(repo),
    interval_(10 * kMicrosPerSecond),
    running_(true) {}

void TableJanitor::start() {
  running_ = true;
  thread_ = std::thread(std::bind(&TableJanitor::run, this));
}

void TableJanitor::stop() {
  if (!running_) {
    return;
  }

  running_ = false;
  thread_.join();
}

void TableJanitor::check() {
  stx::logDebug("fnord.evdb", "Running TableJanitor...");

  auto tables = repo_->tables();

  for (const auto& table_name : tables) {
    auto tbl = repo_->findTableWriter(table_name);

    tbl->commit();
    tbl->merge();
    tbl->gc();
  }
}

void TableJanitor::run() {
  while (running_.load()) {
    auto begin = WallClock::unixMicros();

    try {
      check();
    } catch (const Exception& e) {
      stx::logError("fnord.evdb", e, "TableJanitor error");
    }

    auto elapsed = WallClock::unixMicros() - begin;
    if (elapsed < interval_) {
      usleep(interval_ - elapsed);
    }
  }
}

} // namespace logtable
} // namespace stx

