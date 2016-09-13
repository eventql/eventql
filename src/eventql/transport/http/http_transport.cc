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

HTTPTransport::HTTPTransport(
    Database* database) :
    database_(database),
    status_servlet_(database),
    api_servlet_(database),
    rpc_servlet_(database) {}

void HTTPTransport::handleConnection(int fd, std::string prelude_bytes) {
  database_->startThread([this, fd, prelude_bytes] (Session* session) {
    logDebug("eventql", "Opening new http connection; fd=$0", fd);

    auto dbctx = session->getDatabaseContext();

    http::HTTPServerConnection conn(
        fd,
        dbctx->config->getInt("server.http_io_timeout").get(),
        prelude_bytes);

    http::HTTPRequest request;
    auto rc = conn.recvRequest(&request);
    if (!rc.isSuccess()) {
      logError("evqld", "HTTP Error: $0", rc.getMessage());
      return;
    }

    logInfo("evqld", "HTTP Request: $0", request.uri());

    auto req_stream = mkRef(new http::HTTPRequestStream(&request));
    auto res_stream = mkRef(new http::HTTPResponseStream(&conn));

    try {
      auto servlet = getServlet(request);
      servlet->handleHTTPRequest(req_stream, res_stream);
    } catch (const std::exception& e) {
      logError("evqld", "HTTP Error: $0", e.what());
    }
  });
}

http::StreamingHTTPService* HTTPTransport::getServlet(
    const http::HTTPRequest& request) {
  auto path = request.uri();

  if (StringUtil::beginsWith(path, "/api/")) {
    return &api_servlet_;
  }

  if (StringUtil::beginsWith(path, "/rpc/") ||
      StringUtil::beginsWith(path, "/tsdb/")) {
    return &rpc_servlet_;
  }

  if (StringUtil::beginsWith(path, "/eventql/")) {
    return &status_servlet_;
  }

  return &default_servlet_;
}

} // namespace eventql

