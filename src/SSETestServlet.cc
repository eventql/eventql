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
      http::HTTPRequest* req,
      http::HTTPResponseStream* res_stream) {
  int i = 0;
  auto write_next_chunk = [i, res_stream] () mutable {
    String chunk = "blah blah blah\n\n";
    fnord::iputs("write chunk... $0", i);

    if (++i < 100) {
      res_stream->writeBodyChunk(Buffer(chunk.data(), chunk.size()));
      usleep(100000);
    } else {
      res_stream->finishResponse();
    }
  };

  http::HTTPResponse res;
  res.populateFromRequest(*req);
  res.setStatus(http::kStatusOK);
  res_stream->onBodyWritten(write_next_chunk);
  res_stream->startResponse(res);
}

}
