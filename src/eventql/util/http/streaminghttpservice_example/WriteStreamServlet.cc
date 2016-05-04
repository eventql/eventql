/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Laura Schlimmer
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "WriteStreamServlet.h"

namespace stx {
namespace http {

void WriteStreamServlet::handleHTTPRequest(
      RefPtr<http::HTTPRequestStream> req_stream,
      RefPtr<http::HTTPResponseStream> res_stream) {


  HTTPResponse resp;
  Buffer chunk;

  chunk.append("Ping Pong\n");
  req_stream->readBody();
  resp.populateFromRequest(req_stream->request());
  resp.setStatus(kStatusOK);
  res_stream->startResponse(resp);

  for (int i = 0; i < 100; ++i) {
    res_stream->writeBodyChunk(chunk);
    usleep(100000);
  }

  res_stream->finishResponse();
}

}
}

