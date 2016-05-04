/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/exception.h>
#include <stx/inspect.h>
#include <stx/logging.h>
#include <stx/wallclock.h>
#include "stx/http/httpserverconnection.h"
#include <stx/http/httpserver.h>

/*
TODO:
  - timeouts
  - httpconnection -> httpserverconnection
  - eventloop
  - 100 continue
  - chunked encoding
  - https
*/

namespace stx {
namespace http {

HTTPServer::HTTPServer(
    HTTPHandlerFactory* handler_factory,
    TaskScheduler* scheduler) :
    handler_factory_(handler_factory),
    scheduler_(scheduler),
    ssock_(scheduler) {
  ssock_.onConnection([this] (std::unique_ptr<net::TCPConnection> conn) {
    HTTPServerConnection::start(
        handler_factory_,
        std::move(conn),
        scheduler_,
        &stats_);
  });
}

void HTTPServer::listen(int port) {
  logNotice("http.server", "Starting HTTP server on port $0", port);
  ssock_.listen(port);
}

HTTPServerStats* HTTPServer::stats() {
  return &stats_;
}

}
}
