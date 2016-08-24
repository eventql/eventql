/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include "eventql/eventql.h"
#include "eventql/server/session.h"
#include "eventql/util/logging.h"
#include "eventql/transport/http/http_transport.h"

namespace eventql {

namespace {

class LocalTaskScheduler : public TaskScheduler {
public:

  void run(std::function<void()> task) override {
    task();
  }

  void runOnReadable(std::function<void()> task, int fd) override {
    fd_set op_read, op_write;
    FD_ZERO(&op_read);
    FD_ZERO(&op_write);
    FD_SET(fd, &op_read);

    auto res = select(fd + 1, &op_read, &op_write, &op_read, NULL);

    if (res == 0) {
      RAISE(kIOError, "unexpected timeout while select()ing");
    }

    if (res == -1) {
      RAISE_ERRNO(kIOError, "select() failed");
    }

    task();
  }

  void runOnWritable(std::function<void()> task, int fd) override{
    run([task, fd] () {
      fd_set op_read, op_write;
      FD_ZERO(&op_read);
      FD_ZERO(&op_write);
      FD_SET(fd, &op_write);

      auto res = select(fd + 1, &op_read, &op_write, &op_write, NULL);

      if (res == 0) {
        RAISE(kIOError, "unexpected timeout while select()ing");
      }

      if (res == -1) {
        RAISE_ERRNO(kIOError, "select() failed");
      }

      task();
    });
  }

  void runOnWakeup(
      std::function<void()> task,
      Wakeup* wakeup,
      long generation) override {
    run([task, wakeup, generation] () {
      wakeup->waitForWakeup(generation);
      task();
    });
  }

};

static LocalTaskScheduler local_scheduler;

} // namespace

HTTPTransport::HTTPTransport(Database* database) : database_(database) {
  //eventql::AnalyticsServlet analytics_servlet(
  //    cache_dir,
  //    internal_auth.get(),
  //    client_auth.get(),
  //    internal_auth.get(),
  //    sql.get(),
  //    &table_service,
  //    config_dir.get(),
  //    &partition_map,
  //    &sql_service,
  //    &mapreduce_service,
  //    &table_service);

  //eventql::StatusServlet status_servlet(
  //    &cfg,
  //    &partition_map,
  //    config_dir.get(),
  //    http_server.stats(),
  //    &evqld_stats()->http_client_stats,
  //    &tsdb_replication);

  //http_router.addRouteByPrefixMatch("/api/", &analytics_servlet);
  //http_router.addRouteByPrefixMatch("/eventql", &status_servlet);
  http_router_.addRouteByPrefixMatch("/", &default_servlet_);
}

void HTTPTransport::handleConnection(int fd, std::string prelude_bytes) {
  logDebug("eventql", "Opening new http connection; fd=$0", fd);

  database_->startThread([this, fd, prelude_bytes] (Session* session) {
    try {
      auto http_conn = mkRef(
          new http::HTTPServerConnection(
              &http_router_,
              ScopedPtr<net::TCPConnection>(new net::TCPConnection(fd)),
              &local_scheduler,
              &http_stats_));

      http_conn->start(prelude_bytes);
    } catch (const std::exception& e){
      logError(
          "eventql",
          "HTTP connection error: $0",
          e.what());
    }
  });
}

} // namespace eventql

