/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "demoQueryServlet.h"

using namespace fnord;
namespace cm {

void demoQueryServlet::handleHTTPRequest(
      RefPtr<http::HTTPRequestStream> req_stream,
      RefPtr<http::HTTPResponseStream> res_stream) {

  http::HTTPSSEStream sse_stream(req_stream, res_stream);
  sse_stream.start();

  for (int i = 0; i < 100; i++) {
    sse_stream.sendEvent("ping pong");
    usleep(1000);
  }

  sse_stream.finish();
}

}
