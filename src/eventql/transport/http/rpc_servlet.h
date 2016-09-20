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
#include "eventql/util/http/httpservice.h"
#include <eventql/util/random.h>
#include <eventql/db/table_service.h>
#include <eventql/util/http/HTTPSSEStream.h>
#include "eventql/eventql.h"
#include "eventql/db/metadata_service.h"
#include "eventql/db/database.h"

namespace eventql {

class RPCServlet : public http::StreamingHTTPService {
public:

  RPCServlet(Database* database);

  void handleHTTPRequest(
      RefPtr<http::HTTPRequestStream> req_stream,
      RefPtr<http::HTTPResponseStream> res_stream);

protected:

  void replicateRecords(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  void commitPartition(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  void compactPartition(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  void createMetadataFile(
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void performMetadataOperation(
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void discoverPartitionMetadata(
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void fetchMetadataFile(
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void fetchLatestMetadataFile(
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void listPartitions(
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void findPartition(
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  Database* db_;
};

}

