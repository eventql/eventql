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
#include <fnord-logtable/TableReplication.h>

namespace util {
namespace logtable {

TableReplication::TableReplication(
    http::HTTPConnectionPool* http) :
    http_(http),
    interval_(1 * kMicrosPerSecond),
    running_(true) {}

void TableReplication::replicateTableFrom(
    RefPtr<TableWriter> table,
    const URI& source_uri) {
  targets_.emplace_back(table, source_uri);
}

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

void TableReplication::pull(
      RefPtr<TableWriter> table,
      const URI& uri) {
  logDebug(
      "fn.evdb",
      "Replicating table '$0' from '$1'...",
      table->name(),
      uri.toString());

  auto snapshot_path = StringUtil::format(
      "$0/$1?table=$2",
      uri.path(),
      "snapshot",
      table->name());

  http::HTTPRequest req(http::HTTPMessage::M_GET, snapshot_path);
  req.addHeader("Host", uri.hostAndPort());

  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 200) {
    RAISEF(
        kRuntimeError,
        "received non-200 response: $0 for $1 ($2)",
        r.body().toString(),
        snapshot_path,
        uri.hostAndPort());
  }

  TableGeneration gen;
  gen.decode(r.body());
  table->replicateFrom(gen);
}

void TableReplication::run() {
  while (running_.load()) {
    auto begin = WallClock::unixMicros();

    for (auto& t : targets_) {
      try {
        pull(t.first, t.second);
      } catch (const Exception& e) {
        logError("fnord.evdb", e, "TableReplication error");
      }
    }

    auto elapsed = WallClock::unixMicros() - begin;
    if (elapsed < interval_) {
      usleep(interval_ - elapsed);
    }
  }
}

} // namespace logtable
} // namespace util

