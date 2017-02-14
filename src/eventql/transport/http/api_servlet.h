/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include "eventql/util/VFS.h"
#include "eventql/util/http/httpservice.h"
#include "eventql/util/http/HTTPSSEStream.h"
#include "eventql/util/json/json.h"
#include "eventql/util/web/SecureCookie.h"
#include "eventql/sql/runtime/runtime.h"
#include "eventql/auth/internal_auth.h"
#include "eventql/config/config_directory.h"
#include "eventql/db/table_service.h"
#include "eventql/transport/http/mapreduce_servlet.h"
#include "eventql/auth/client_auth.h"
#include "eventql/auth/internal_auth.h"
#include "eventql/server/sql_service.h"
#include "eventql/db/database.h"

namespace eventql {

class APIServlet : public http::StreamingHTTPService {
public:

  APIServlet(Database* db);

  void handleHTTPRequest(
      RefPtr<http::HTTPRequestStream> req_stream,
      RefPtr<http::HTTPResponseStream> res_stream);

protected:

  void handle(
      RefPtr<http::HTTPRequestStream> req_stream,
      RefPtr<http::HTTPResponseStream> res_stream);

  void getAuthInfo(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void listTables(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void fetchTableDefinition(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void createTable(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void addTableField(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void removeTableField(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void dropTable(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void insertIntoTable(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void executeSQL(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream);

  void executeSQL_ASCII(
      const std::string& query,
      const std::string& database,
      Session* session,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream);

  void executeSQL_JSON(
      const std::string& query,
      const std::string& database,
      Session* session,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream);

  void executeSQL_JSONSSE(
      const std::string& query,
      const std::string& database,
      Session* session,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream);

  Database* db_;
  MapReduceAPIServlet mapreduce_api_;
};

}
