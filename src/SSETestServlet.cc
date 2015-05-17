/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <unistd.h>
#include <fnord-base/inspect.h>
#include "SSETestServlet.h"

using namespace fnord;

namespace cm {

void SSETestServlet::handleHTTPRequest(
      RefPtr<http::HTTPRequestStream> req_stream,
      RefPtr<http::HTTPResponseStream> res_stream) {
  req_stream->readBody();

  Buffer pong;
  pong.append("pong: ");
  pong.append(req_stream->request().body());
  pong.append("\n");

  http::HTTPResponse res;
  res.populateFromRequest(req_stream->request());
  res.setStatus(http::kStatusOK);
  res_stream->startResponse(res);

  for (int i = 0; i < 100; ++i) {
    res_stream->writeBodyChunk(pong);
    usleep(100000);
  }

  res_stream->finishResponse();
}

}
