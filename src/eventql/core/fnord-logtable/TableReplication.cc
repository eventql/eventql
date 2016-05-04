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
#include <eventql/util/logging.h>
#include <eventql/util/wallclock.h>
#include <fnord-logtable/TableReplication.h>

namespace stx {
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
  stx::logDebug(
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
        stx::logError("fnord.evdb", e, "TableReplication error");
      }
    }

    auto elapsed = WallClock::unixMicros() - begin;
    if (elapsed < interval_) {
      usleep(interval_ - elapsed);
    }
  }
}

} // namespace logtable
} // namespace stx

