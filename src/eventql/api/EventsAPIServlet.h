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
#include "eventql/api/EventsService.h"

using namespace stx;

namespace zbase {

class EventsAPIServlet {
public:

  EventsAPIServlet(
      EventsService* service,
      ConfigDirectory* cdir,
      const String& cachedir);

  void handle(
      const AnalyticsSession& session,
      RefPtr<stx::http::HTTPRequestStream> req_stream,
      RefPtr<stx::http::HTTPResponseStream> res_stream);

protected:

  void scanTable(
      const AnalyticsSession& session,
      const URI& uri,
      http::HTTPRequestStream* req_stream,
      http::HTTPResponseStream* res_stream);

  void scanTablePartition(
      const AnalyticsSession& session,
      const URI& uri,
      http::HTTPRequestStream* req_stream,
      http::HTTPResponseStream* res_stream);

  void insertEvents(
      const AnalyticsSession& session,
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  EventsService* service_;
  ConfigDirectory* cdir_;
  String cachedir_;
};

}
