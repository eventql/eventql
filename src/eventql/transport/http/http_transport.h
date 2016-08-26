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
#pragma once
#include "eventql/eventql.h"
#include "eventql/db/database.h"
#include "eventql/util/http/httpserver.h"
#include "eventql/util/http/httprouter.h"
#include "eventql/util/thread/eventloop.h"
#include "eventql/transport/http/default_servlet.h"
#include "eventql/transport/http/status_servlet.h"
#include "eventql/transport/http/api_servlet.h"
#include "eventql/transport/http/rpc_servlet.h"

namespace eventql {

class HTTPTransport {
public:

  HTTPTransport(Database* database);

  void handleConnection(int fd, std::string prelude_bytes);

  void startIOThread();
  void stopIOThread();

protected:
  Database* database_;
  thread::EventLoop ev_;
  http::HTTPRouter http_router_;
  http::HTTPServerStats http_stats_;
  DefaultServlet default_servlet_;
  StatusServlet status_servlet_;
  APIServlet api_servlet_;
  RPCServlet rpc_servlet_;
  std::thread io_thread_;
};

} // namespace eventql
