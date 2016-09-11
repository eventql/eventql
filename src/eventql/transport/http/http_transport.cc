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

SessionTaskScheduler::SessionTaskScheduler(Database* db) : db_(db) {}

void SessionTaskScheduler::run(std::function<void()> task) {
  db_->startThread([task] (Session* session) {
    task();
  });
}

void SessionTaskScheduler::runOnReadable(std::function<void()> task, int fd) {
  abort();
}

void SessionTaskScheduler::runOnWritable(std::function<void()> task, int fd) {
  abort();
}

void SessionTaskScheduler::runOnWakeup(
    std::function<void()> task,
    Wakeup* wakeup,
    long generation) {
  abort();
}

HTTPTransport::HTTPTransport(
    Database* database) :
    database_(database),
    status_servlet_(database),
    api_servlet_(database),
    rpc_servlet_(database),
    session_scheduler_(database) {
  http_router_.addRouteByPrefixMatch(
      "/rpc/",
      &rpc_servlet_,
      &session_scheduler_);

  http_router_.addRouteByPrefixMatch(
      "/tsdb/",
      &rpc_servlet_,
      &session_scheduler_);

  http_router_.addRouteByPrefixMatch(
      "/api/",
      &api_servlet_,
      &session_scheduler_);

  http_router_.addRouteByPrefixMatch(
      "/eventql",
      &status_servlet_,
      &session_scheduler_);

  http_router_.addRouteByPrefixMatch("/", &default_servlet_);
}

void HTTPTransport::handleConnection(int fd, std::string prelude_bytes) {
  logDebug("eventql", "Opening new http connection; fd=$0", fd);

  try {
        http::HTTPServerConnection::start(
            &http_router_,
            ScopedPtr<net::TCPConnection>(new net::TCPConnection(fd)),
            &ev_,
            &http_stats_,
            prelude_bytes);

    //http_conn->start(prelude_bytes);
  } catch (const std::exception& e){
    logError(
        "eventql",
        "HTTP connection error: $0",
        e.what());
  }
}

void HTTPTransport::startIOThread() {
  io_thread_ = std::thread([this] () {
    ev_.run();
  });
}

void HTTPTransport::stopIOThread() {
  ev_.shutdown();
  io_thread_.join();
}

} // namespace eventql

