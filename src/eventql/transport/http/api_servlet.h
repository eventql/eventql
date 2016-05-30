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
#pragma once
#include "eventql/util/VFS.h"
#include "eventql/util/http/httpservice.h"
#include "eventql/util/http/HTTPSSEStream.h"
#include "eventql/util/json/json.h"
#include "eventql/util/web/SecureCookie.h"
#include "eventql/AnalyticsSession.pb.h"
#include "eventql/sql/runtime/runtime.h"
#include "eventql/auth/internal_auth.h"
#include "eventql/config/config_directory.h"
#include "eventql/db/table_service.h"
#include "eventql/transport/http/LogfileAPIServlet.h"
#include "eventql/transport/http/MapReduceAPIServlet.h"
#include "eventql/RemoteTSDBScanParams.pb.h"
#include "eventql/auth/client_auth.h"
#include "eventql/auth/internal_auth.h"
#include "eventql/server/sql_service.h"

#include "eventql/eventql.h"

namespace eventql {

struct AnalyticsQuery;

class AnalyticsServlet : public http::StreamingHTTPService {
public:

  AnalyticsServlet(
      const String& cachedir,
      InternalAuth* auth,
      ClientAuth* client_auth,
      InternalAuth* internal_auth,
      csql::Runtime* sql,
      eventql::TableService* tsdb,
      ConfigDirectory* customer_dir,
      PartitionMap* pmap,
      SQLService* sql_service,
      LogfileService* logfile_service,
      MapReduceService* mapreduce_service,
      TableService* table_service);

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

  void pushEvents(
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void insertIntoMetric(
      const URI& uri,
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void listTables(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void fetchTableDefinition(
      Session* session,
      const String& table_name,
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

  void addTableTag(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void removeTableTag(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void insertIntoTable(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void uploadTable(
      const URI& uri,
      Session* session,
      http::HTTPRequestStream* req_stream,
      http::HTTPResponse* res);

  void executeSQL(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream);

  void executeSQL_ASCII(
      const URI::ParamList& params,
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream);

  void executeSQL_BINARY(
      const URI::ParamList& params,
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream);

  void executeSQL_JSON(
      const URI::ParamList& params,
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream);

  void executeSQL_JSONSSE(
      const URI::ParamList& params,
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream);

  void executeQTree(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream);

  void executeSQLAggregatePartition(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void executeSQLScanPartition(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream);

  void executeDrilldownQuery(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream);

  void pipelineInfo(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void sessionTrackingListEvents(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void sessionTrackingEventInfo(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void sessionTrackingEventAdd(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void sessionTrackingEventRemove(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void sessionTrackingEventAddField(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void sessionTrackingEventRemoveField(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void sessionTrackingListAttributes(
      Session* session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void performLogin(
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void performLogout(
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  inline void expectHTTPPost(const http::HTTPRequest& req) {
    if (req.method() != http::HTTPMessage::M_POST) {
      RAISE(kIllegalArgumentError, "expected HTTP POST request");
    }
  }

  String getCookieDomain(const http::HTTPRequest& req) {
    auto domain = req.getHeader("Host");
    auto ppos = domain.find(":");
    if (ppos != String::npos) {
      domain.erase(domain.begin() + ppos, domain.end());
    }

    auto parts = StringUtil::split(domain, ".");
    if (parts.size() > 2) {
      parts.erase(parts.begin(), parts.end() - 2);
    }

    if (parts.size() == 2) {
      return "." + StringUtil::join(parts, ".");
    } else {
      return "";
    }
  }

  void catchAndReturnErrors(
      http::HTTPResponse* resp,
      Function<void ()> fn) const {
    try {
      fn();
    } catch (const StandardException& e) {
      resp->setStatus(http::kStatusInternalServerError);
      resp->addBody(e.what());
    }
  }

  String cachedir_;
  InternalAuth* auth_;
  ClientAuth* client_auth_;
  InternalAuth* internal_auth_;
  csql::Runtime* sql_;
  eventql::TableService* tsdb_;
  ConfigDirectory* customer_dir_;

  PartitionMap* pmap_;

  SQLService* sql_service_;
  LogfileService* logfile_service_;
  MapReduceService* mapreduce_service_;
  TableService* table_service_;

  LogfileAPIServlet logfile_api_;
  MapReduceAPIServlet mapreduce_api_;
};

}
