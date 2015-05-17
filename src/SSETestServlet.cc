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
      http::HTTPRequestStream* req_stream,
      http::HTTPResponseStream* res_stream) {
  Buffer pong;
  pong.append("pong: ");
  req_stream->readBody([&pong] (const void* data, size_t size) {
    pong.append(data, size);
  });
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
