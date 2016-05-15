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
#include "unistd.h"
#include <eventql/util/logging.h>
#include <eventql/util/wallclock.h>
#include <fnord-logtable/TableJanitor.h>

namespace util {
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
  util::logDebug("fnord.evdb", "Running TableJanitor...");

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
      util::logError("fnord.evdb", e, "TableJanitor error");
    }

    auto elapsed = WallClock::unixMicros() - begin;
    if (elapsed < interval_) {
      usleep(interval_ - elapsed);
    }
  }
}

} // namespace logtable
} // namespace util

