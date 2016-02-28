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
#include "zbase/metrics/MetricService.h"

using namespace stx;

namespace zbase {

class MetricAPIServlet {
public:

  MetricAPIServlet(
      MetricService* service,
      ConfigDirectory* cdir,
      const String& cachedir);

  void handle(
      const AnalyticsSession& session,
      RefPtr<stx::http::HTTPRequestStream> req_stream,
      RefPtr<stx::http::HTTPResponseStream> res_stream);

protected:

  void executeQuery(
      const AnalyticsSession& session,
      const URI& uri,
      http::HTTPRequestStream* req_stream,
      http::HTTPResponseStream* res_stream);

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

  MetricService* service_;
  ConfigDirectory* cdir_;
  String cachedir_;
};

}
