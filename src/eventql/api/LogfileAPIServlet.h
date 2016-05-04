/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "eventql/util/stdtypes.h"
#include "eventql/util/http/httpservice.h"
#include "eventql/util/http/HTTPSSEStream.h"
#include "eventql/AnalyticsSession.pb.h"
#include "eventql/api/LogfileService.h"

using namespace stx;

namespace zbase {

class LogfileAPIServlet {
public:

  LogfileAPIServlet(
      LogfileService* service,
      ConfigDirectory* cdir,
      const String& cachedir);

  void handle(
      const AnalyticsSession& session,
      RefPtr<stx::http::HTTPRequestStream> req_stream,
      RefPtr<stx::http::HTTPResponseStream> res_stream);

protected:

  void listLogfiles(
      const AnalyticsSession& session,
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void fetchLogfileDefinition(
      const AnalyticsSession& session,
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void setLogfileRegex(
      const AnalyticsSession& session,
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void uploadLogfile(
      const AnalyticsSession& session,
      const URI& uri,
      http::HTTPRequestStream* req_stream,
      http::HTTPResponse* res);

  void renderLogfileDefinition(
      const LogfileDefinition* logfile_def,
      json::JSONOutputStream* json);

  LogfileService* service_;
  ConfigDirectory* cdir_;
  String cachedir_;
};

}
