/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "stx/http/httpservice.h"
#include <stx/random.h>
#include <stx/http/HTTPSSEStream.h>

using namespace fnord;

namespace tsdb {

class SQLServlet : public fnord::http::StreamingHTTPService {
public:

  SQLServlet(TSDBNode* node);

  void handleHTTPRequest(
      RefPtr<http::HTTPRequestStream> req_stream,
      RefPtr<http::HTTPResponseStream> res_stream);

protected:


  void executeSQL(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  void executeSQLStream(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream,
      URI* uri);

};

}
#endif
