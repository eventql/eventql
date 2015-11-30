/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "stx/assets.h"
#include <stx/fnv.h>
#include "stx/protobuf/msg.h"
#include "zbase/metrics/MetricAPIServlet.h"

using namespace stx;

namespace zbase {

MetricAPIServlet::MetricAPIServlet(
    MetricService* service,
    ConfigDirectory* cdir,
    const String& cachedir) :
    service_(service),
    cdir_(cdir),
    cachedir_(cachedir) {}

void MetricAPIServlet::handle(
    const AnalyticsSession& session,
    RefPtr<stx::http::HTTPRequestStream> req_stream,
    RefPtr<stx::http::HTTPResponseStream> res_stream) {
  const auto& req = req_stream->request();
  URI uri(req.uri());

  http::HTTPResponse res;
  res.populateFromRequest(req);

  res.setStatus(http::kStatusNotFound);
  res.addHeader("Content-Type", "text/html; charset=utf-8");
  res.addBody(Assets::getAsset("zbase/webui/404.html"));
  res_stream->writeResponse(res);
}

}
