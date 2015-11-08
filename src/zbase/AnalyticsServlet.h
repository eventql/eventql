/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/VFS.h"
#include "stx/http/httpservice.h"
#include "stx/http/HTTPSSEStream.h"
#include "stx/json/json.h"
#include "stx/web/SecureCookie.h"
#include "zbase/dproc/DispatchService.h"
#include "zbase/AnalyticsApp.h"
#include "zbase/ReportParams.pb.h"
#include "zbase/EventIngress.h"
#include "zbase/AnalyticsSession.pb.h"
#include "csql/runtime/runtime.h"
#include "zbase/AnalyticsAuth.h"
#include "zbase/ConfigDirectory.h"
#include "zbase/core/TSDBService.h"
#include "zbase/api/LogfileAPIServlet.h"
#include "zbase/api/EventsAPIServlet.h"
#include "zbase/api/MapReduceAPIServlet.h"
#include "zbase/docdb/DocumentDB.h"
#include "zbase/docdb/DocumentDBServlet.h"
#include "zbase/RemoteTSDBScanParams.pb.h"

using namespace stx;

namespace zbase {

struct AnalyticsQuery;

class AnalyticsServlet : public stx::http::StreamingHTTPService {
public:

  AnalyticsServlet(
      RefPtr<AnalyticsApp> app,
      dproc::DispatchService* dproc,
      EventIngress* ingress,
      const String& cachedir,
      AnalyticsAuth* auth,
      csql::Runtime* sql,
      zbase::TSDBService* tsdb,
      ConfigDirectory* customer_dir,
      DocumentDB* docdb);

  void handleHTTPRequest(
      RefPtr<stx::http::HTTPRequestStream> req_stream,
      RefPtr<stx::http::HTTPResponseStream> res_stream);

protected:

  void handle(
      RefPtr<stx::http::HTTPRequestStream> req_stream,
      RefPtr<stx::http::HTTPResponseStream> res_stream);

  void getAuthInfo(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void getPrivateAPIToken(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void pushEvents(
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void insertIntoMetric(
      const URI& uri,
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void listTables(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void fetchTableDefinition(
      const AnalyticsSession& session,
      const String& table_name,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void createTable(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void insertIntoTable(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void uploadTable(
      const URI& uri,
      const AnalyticsSession& session,
      http::HTTPRequestStream* req_stream,
      http::HTTPResponse* res);

  void executeSQL(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void executeSQLStream(
      const URI& uri,
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream);

  void executeSQLAggregatePartition(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void executeSQLScanPartition(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream);

  void executeSQLScanPartition(
      const AnalyticsSession& session,
      const RemoteTSDBScanParams& query,
      Function<bool (int argc, const csql::SValue* argv)> fn);

  void pipelineInfo(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void sessionTrackingListEvents(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void sessionTrackingEventInfo(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void sessionTrackingEventAdd(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void sessionTrackingEventRemove(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void sessionTrackingEventAddField(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void sessionTrackingEventRemoveField(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void sessionTrackingListAttributes(
      const AnalyticsSession& session,
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

  RefPtr<AnalyticsApp> app_;
  dproc::DispatchService* dproc_;
  EventIngress* ingress_;
  String cachedir_;
  AnalyticsAuth* auth_;
  csql::Runtime* sql_;
  zbase::TSDBService* tsdb_;
  ConfigDirectory* customer_dir_;

  LogfileAPIServlet logfile_api_;
  EventsAPIServlet events_api_;
  MapReduceAPIServlet mapreduce_api_;
  DocumentDBServlet documents_api_;
};

}
