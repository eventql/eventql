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
#include "dproc/DispatchService.h"
#include "zbase/AnalyticsApp.h"
#include "zbase/ReportParams.pb.h"
#include "zbase/EventIngress.h"
#include "zbase/AnalyticsSession.pb.h"
#include "chartsql/runtime/runtime.h"
#include "zbase/AnalyticsAuth.h"
#include "zbase/ConfigDirectory.h"
#include "tsdb/TSDBService.h"
#include "zbase/api/LogfileAPIServlet.h"
#include "docdb/DocumentDB.h"
#include "docdb/DocumentDBServlet.h"

using namespace stx;

namespace cm {

struct AnalyticsQuery;

class AnalyticsServlet : public stx::http::StreamingHTTPService {
public:
  static const char kSessionCookieKey[];
  static const uint64_t kSessionLifetimeMicros;

  AnalyticsServlet(
      RefPtr<AnalyticsApp> app,
      dproc::DispatchService* dproc,
      EventIngress* ingress,
      const String& cachedir,
      AnalyticsAuth* auth,
      csql::Runtime* sql,
      tsdb::TSDBService* tsdb,
      ConfigDirectory* customer_dir,
      DocumentDB* docdb);

  void handleHTTPRequest(
      RefPtr<stx::http::HTTPRequestStream> req_stream,
      RefPtr<stx::http::HTTPResponseStream> res_stream);

protected:

  void getAuthInfo(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void getPrivateAPIToken(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void executeQuery(
      const AnalyticsSession& session,
      const URI& uri,
      http::HTTPRequestStream* req_stream,
      http::HTTPResponseStream* res_stream);

  void fetchFeed(
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void generateReport(
      const URI& uri,
      http::HTTPRequestStream* req_stream,
      http::HTTPResponseStream* res_stream);

  void downloadReport(
      const URI& uri,
      http::HTTPRequestStream* req_stream,
      http::HTTPResponseStream* res_stream);

  void pushEvents(
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void insertIntoMetric(
      const URI& uri,
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void createTable(
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

  void sessionTrackingListAttributes(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  Option<AnalyticsSession> authenticateRequest(
      const http::HTTPRequest& request) const;

  void performLogin(
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  inline void expectHTTPPost(const http::HTTPRequest& req) {
    if (req.method() != http::HTTPMessage::M_POST) {
      RAISE(kIllegalArgumentError, "expected HTTP POST request");
    }
  }

  RefPtr<AnalyticsApp> app_;
  dproc::DispatchService* dproc_;
  EventIngress* ingress_;
  String cachedir_;
  AnalyticsAuth* auth_;
  csql::Runtime* sql_;
  tsdb::TSDBService* tsdb_;
  ConfigDirectory* customer_dir_;

  LogfileAPIServlet logfile_api_;
  DocumentDBServlet documents_api_;
};

}
