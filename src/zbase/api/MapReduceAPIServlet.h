/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/stdtypes.h"
#include "stx/http/httpservice.h"
#include "stx/http/HTTPSSEStream.h"
#include "zbase/AnalyticsSession.pb.h"
#include "zbase/api/MapReduceService.h"

using namespace stx;

namespace zbase {

class MapReduceAPIServlet {
public:

  MapReduceAPIServlet(
      MapReduceService* service,
      ConfigDirectory* cdir,
      const String& cachedir);

  void handle(
      const AnalyticsSession& session,
      RefPtr<stx::http::HTTPRequestStream> req_stream,
      RefPtr<stx::http::HTTPResponseStream> res_stream);

protected:

  void executeMapReduceScript(
      const AnalyticsSession& session,
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void executeMapPartitionTask(
      const AnalyticsSession& session,
      const URI& uri,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  MapReduceService* service_;
  ConfigDirectory* cdir_;
  String cachedir_;
};

}
