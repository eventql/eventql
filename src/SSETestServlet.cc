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

  http::HTTPSSEStream sse(req_stream, res_stream);
  sse.start();

  for (int i = 0; i < 10; ++i) {
    sse.sendEvent(StringUtil::toString(i), "myid", "myevent");
    usleep(100000);
  }

  sse.finish();
}

}
