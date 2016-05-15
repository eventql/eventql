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
#ifndef _FNORD_TSDB_TSDBSERVLET_H
#define _FNORD_TSDB_TSDBSERVLET_H
#include "eventql/util/http/httpservice.h"
#include <eventql/util/random.h>
#include <eventql/core/TSDBService.h>
#include <eventql/util/http/HTTPSSEStream.h>

using namespace util;

namespace eventql {

class TSDBServlet : public util::http::StreamingHTTPService {
public:

  TSDBServlet(
      TSDBService* node,
      const String& tmpdir);

  void handleHTTPRequest(
      RefPtr<http::HTTPRequestStream> req_stream,
      RefPtr<http::HTTPResponseStream> res_stream);

protected:

  void insertRecords(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  void replicateRecords(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  void compactPartition(
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
