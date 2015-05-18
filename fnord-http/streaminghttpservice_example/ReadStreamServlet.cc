/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Laura Schlimmer
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "src/ReadStreamServlet.h"


namespace fnord {
namespace http {

void ReadStreamServlet::handleHTTPRequest(
      RefPtr<http::HTTPRequestStream> req_stream,
      RefPtr<http::HTTPResponseStream> res_stream) {

  Buffer body;

  auto bodyChunkRead = [body] (const void* data, size_t size) mutable {
    Buffer chunk;
    chunk.append(data, size);
    body.append(data, size);
    fnord::iputs("Request Body Chunk read: $0", chunk.toString());
  };

  req_stream->readBody(bodyChunkRead);
  fnord::iputs("Request Body: $0", body.toString());

}

}
}



