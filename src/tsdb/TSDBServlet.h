/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_TSDB_TSDBSERVLET_H
#define _FNORD_TSDB_TSDBSERVLET_H
#include "stx/http/httpservice.h"
#include <stx/random.h>
#include <tsdb/TSDBService.h>
#include <stx/http/HTTPSSEStream.h>

using namespace stx;

namespace tsdb {

class TSDBServlet : public stx::http::StreamingHTTPService {
public:

  TSDBServlet(TSDBService* node);

  void handleHTTPRequest(
      RefPtr<http::HTTPRequestStream> req_stream,
      RefPtr<http::HTTPResponseStream> res_stream);

protected:

  void insertRecords(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  void streamPartition(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream,
      URI* uri);

  void fetchPartitionInfo(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  void executeSQL(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  void executeSQLStream(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream,
      URI* uri);

  void updateCSTable(
      const URI& uri,
      http::HTTPRequestStream* req_stream,
      http::HTTPResponse* res);

  TSDBService* node_;
  String tmpdir_;
  Random rnd_;
};

}
#endif
